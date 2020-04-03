// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Web.Mvc;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.AzureOAuthProvider;
    using Microsoft.WhiteSpaces.AzureTableAccess;
    using Microsoft.WhiteSpaces.BusinessManager;
    using Microsoft.WhiteSpaces.Common.Enums;
    using Microsoft.WindowsAzure.Storage;

    public class ProfileController : Controller
    {
        private const int PageSize = 5;
        private readonly IUserManager userManager;
        private readonly IUserAdminManager adminManager;
        private readonly IRegionManager regionManager;        

        public ProfileController(IUserManager userManager, IUserAdminManager adminManager, IRegionManager regionManager)
        {
            this.userManager = userManager;
            this.adminManager = adminManager;
            this.regionManager = regionManager;
        }

        [Dependency]
        public IAuditor ProfileAuditor { get; set; }

        [Dependency]
        public ILogger ProfileLogger { get; set; }        

        public PartialViewResult GetUserDetails()
        {
            return this.PartialView("ProfilePartial", this.GetUserProfileModel());
        }

        public PartialViewResult GetRequestView()
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;
            UserDetails userDetails = this.userManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);
            //// string userId = this.userManager.GetUserIdByName(userPrincipal.UserName).ToString();
            string userId = userDetails.UserInfo.RowKey;
            AccessElevationViewModel viewModel = new AccessElevationViewModel()
            {
                UserId = userId
            };

            return this.PartialView("ElivatedAccessPartial", viewModel);
        }

        public ActionResult Index()
        {
            return this.View();
        }

        public JsonResult GetAccessLevels(string userId, string regulatory)
        {
            var accessDetails = this.userManager.GetUserAccessDetails(userId);
            var accessLevel = accessDetails.Where(x => x.Authority.ToString() == regulatory).FirstOrDefault().AccessLevel;

            var roles = Utility.GetUpgradableAccessLevels(Convert.ToInt32(accessLevel), regulatory.ToLower());

            return this.Json(roles, JsonRequestBehavior.AllowGet);
        }

        [HttpPost]
        public ActionResult SaveUserDetails(UserProfileModel profileModel, string buttonType)
        {
            if (buttonType == "Update")
            {
                var userDetails = (UserDetails)TempData["UserDetails"];

                if (userDetails == null)
                {
                    userDetails = this.userManager.GetUserAndAccessDetailsByUserId(profileModel.UserId);
                }

                var userProfile = userDetails.UserInfo;
                var accessDetails = userDetails.AccessInfo;

                ViewBag.Country = new SelectList(Utility.GetCounties(), profileModel.Country);
                ViewBag.TimeZone = new SelectList(Utility.GetTimeZones(), profileModel.TimeZone);
                ViewBag.PhoneCountryCode = new SelectList(Utility.GetCountryPhoneCodes(), userDetails.UserInfo.PhoneCountryCode);

                if (ModelState.IsValid)
                {
                    userProfile.FirstName = profileModel.FirstName;
                    userProfile.LastName = profileModel.LastName;
                    userProfile.PreferredEmail = profileModel.PreferredEmail;
                    userProfile.AccountEmail = profileModel.AccountEmail;
                    userProfile.PhoneCountryCode = profileModel.PhoneCountryCode;
                    userProfile.Phone = profileModel.Phone;
                    userProfile.Address1 = profileModel.Address1;
                    userProfile.Address2 = profileModel.Address2;
                    userProfile.City = profileModel.City;
                    userProfile.State = profileModel.State;
                    userProfile.Country = profileModel.Country;
                    userProfile.TimeZone = profileModel.TimeZone;
                    userProfile.UpdatedTime = DateTime.Now.ToUniversalTime().ToString();
                    userProfile.ZipCode = profileModel.ZipCode;
                    this.userManager.SaveUserProfile(userProfile);

                    this.ProfileAuditor.UserId = profileModel.UserId;
                    this.ProfileAuditor.TransactionId = this.ProfileLogger.TransactionId;
                    this.ProfileAuditor.Audit(AuditId.UpdateUserProfile, AuditStatus.Success, default(int), userProfile.UserName + " has updated his profile");
                    this.ProfileLogger.Log(TraceEventType.Information, LoggingMessageId.PortalUserProfileUpdate, userProfile.UserName + " has updated his profile");

                    profileModel.AccessInfo = userDetails.AccessInfo.ToList();
                    return this.PartialView("ProfilePartial", profileModel);
                }
                else
                {
                    return this.PartialView("EditProfilePartial", profileModel);
                }
            }
            else
            {
                return this.GetUserDetails();
            }
        }

        [HttpPost]
        public ActionResult RequestElevatedAccess(AccessElevationViewModel elevationModel)
        {
            var userDetails = this.userManager.GetUserAndAccessDetailsByUserId(elevationModel.UserId);
            AccessDetails currentAccessLevel = userDetails.AccessInfo.Where(x => x.Authority.ToString() == elevationModel.Regulatory).FirstOrDefault();

            if (ModelState.IsValid)
            {
                if (this.AnyRequestPending(elevationModel.UserId, elevationModel.Regulatory))
                {
                    ViewBag.Message = "Request is pending for access elevation";
                }
                else
                {
                    AccessElevationRequest requestAccess = new AccessElevationRequest
                    {
                        PartitionKey = elevationModel.UserId,
                        Regulatory = elevationModel.Regulatory,
                        CurrentAccessLevel = Convert.ToInt32(currentAccessLevel.AccessLevel),
                        RequestedAccessLevel = Convert.ToInt32(elevationModel.AccessRole),
                        Justification = elevationModel.Justification,
                        RequestStatus = (int)RequestStatus.Pending
                    };

                    this.userManager.RequestAccessElevation(requestAccess);

                    this.ProfileAuditor.UserId = elevationModel.UserId;
                    this.ProfileAuditor.TransactionId = this.ProfileLogger.TransactionId;
                    this.ProfileAuditor.Audit(AuditId.RequestAccessElevation, AuditStatus.Success, default(int), userDetails.UserInfo.UserName + " requested access elevation");
                    this.ProfileLogger.Log(TraceEventType.Information, LoggingMessageId.PortalUserAccessLevelElevationRequest, userDetails.UserInfo.UserName + " requested access elevation");

                    ViewBag.Message = "Request Submitted Successfully";
                }
            }

            AccessElevationViewModel viewModel = new AccessElevationViewModel()
            {
                UserId = elevationModel.UserId
            };

            if (currentAccessLevel != null)
            {
                viewModel.Roles = Utility.GetUpgradableAccessLevels(Convert.ToInt32(currentAccessLevel.AccessLevel), elevationModel.Regulatory.ToLower());
            }

            return this.PartialView("ElivatedAccessPartial", viewModel);
        }

        [HttpPost]
        public PartialViewResult EditProfile()
        {
            var profileModel = this.GetUserProfileModel();
            ViewBag.Country = new SelectList(Utility.GetCounties(), profileModel.Country);
            ViewBag.TimeZone = new SelectList(Utility.GetTimeZones(), profileModel.TimeZone);
            ViewBag.PhoneCountryCode = new SelectList(Utility.GetCountryPhoneCodes(), profileModel.PhoneCountryCode);

            return this.PartialView("EditProfilePartial", profileModel);
        }

        public PartialViewResult GetMvpdRegistrations(int page = 1)
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;
            UserDetails userDetails = this.userManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

            IEnumerable<MVPDRegistration> registrations = this.regionManager.GetMvpdRegistrations(userDetails.MembershipInfo.UserId.ToString());

            if (page < 1)
            {
                page = 1;
            }

            var pagedregistrations = registrations.Skip((page - 1) * PageSize).Take(PageSize);
            ViewBag.mvpdRowCount = registrations.Count();

            return this.PartialView("MvpdRegistrationsPartial", pagedregistrations);
        }

        public PartialViewResult GetLpAuxRegistrations(int page = 1)
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;
            UserDetails userDetails = this.userManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

            IEnumerable<LPAuxRegistration> registrations = this.regionManager.GetLpAuxRegistrations(userDetails.MembershipInfo.UserId.ToString());

            if (page < 1)
            {
                page = 1;
            }

            var pagedregistrations = registrations.Skip((page - 1) * PageSize).Take(PageSize);
            ViewBag.lpauxRowCount = registrations.Count();

            return this.PartialView("LpauxRegistrations", pagedregistrations);
        }

        public PartialViewResult GetTempBasRegistrations(int page = 1)
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;
            UserDetails userDetails = this.userManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

            IEnumerable<TempBASRegistration> registrations = this.regionManager.GetTempBasRegistrations(userDetails.MembershipInfo.UserId.ToString());

            if (page < 1)
            {
                page = 1;
            }

            var pagedregistrations = registrations.Skip((page - 1) * PageSize).Take(PageSize);
            ViewBag.tbasRowCount = registrations.Count();

            return this.PartialView("TempBasRegistrations", registrations);
        }

        public void DeleteRegistration(string partitionKey, string rowkey, string type, string etag)
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;
            UserDetails userDetails = this.userManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

            RegistrationType registrationType = (RegistrationType)Enum.Parse(typeof(RegistrationType), type);

            this.regionManager.DeleteRegistration(partitionKey, rowkey, etag, registrationType);

            this.ProfileAuditor.UserId = userDetails.UserInfo.RowKey;
            this.ProfileAuditor.TransactionId = this.ProfileLogger.TransactionId;
            this.ProfileAuditor.Audit(AuditId.DeleteRegistration, AuditStatus.Success, default(int), userPrincipal.UserName + " deleted registration of type " + type);           
        }

        private bool AnyRequestPending(string userId, string regulatory)
        {
            try
            {
                IEnumerable<AccessElevationRequest> requests = this.userManager.GetPendingRequestForUser(userId);
                return requests.Where(x => x.Regulatory == regulatory).Any();
            }            
            catch (StorageException) 
            {
                //// At very first time, there won't be any records present, so it will throw exception
                return false;
            }
        }

        private UserProfileModel GetUserProfileModel()
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;

            UserDetails userDetails = this.userManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);
            this.TempData["UserDetails"] = userDetails;

            UserProfileModel profileModel = new UserProfileModel();
            if (userDetails != null)
            {
                profileModel.UserId = userDetails.UserInfo.RowKey;
                profileModel.FirstName = userDetails.UserInfo.FirstName;
                profileModel.LastName = userDetails.UserInfo.LastName;
                profileModel.PreferredEmail = userDetails.UserInfo.PreferredEmail;
                profileModel.PhoneCountryCode = userDetails.UserInfo.PhoneCountryCode;
                profileModel.Phone = userDetails.UserInfo.Phone;
                profileModel.Address1 = userDetails.UserInfo.Address1;
                profileModel.Address2 = userDetails.UserInfo.Address2;
                profileModel.City = userDetails.UserInfo.City;
                profileModel.State = userDetails.UserInfo.State;
                profileModel.Country = userDetails.UserInfo.Country;
                profileModel.ZipCode = userDetails.UserInfo.ZipCode;
                profileModel.AccessInfo = userDetails.AccessInfo.ToList();
                profileModel.AccountEmail = userDetails.UserInfo.AccountEmail;
                profileModel.TimeZone = userDetails.UserInfo.TimeZone;
            }

            return profileModel;
        }
    }
}
