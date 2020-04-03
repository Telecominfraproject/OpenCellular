// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal.Controllers
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Threading.Tasks;
    using System.Web.Mvc;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.WhiteSpaces.AzureOAuthProvider;
    using Microsoft.WhiteSpaces.AzureTableAccess;
    using Microsoft.WhiteSpaces.BusinessManager;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Enums;

    public class UserManagementController : Controller
    {
        private const int PageSize = 10;
        private static object lockObject = new object();  
        private readonly IUserAdminManager adminManager;
        private readonly IWhitespacesManager whitespacesManager;              

        public UserManagementController(IUserAdminManager userManager, IWhitespacesManager whitespacesManager)
        {
            this.adminManager = userManager;
            this.whitespacesManager = whitespacesManager;
        }

        [Dependency]
        public IAuditor UserManagementAuditor { get; set; }

        [Dependency]
        public ILogger UserManagementLogger { get; set; }        

        // GET: /UserManagement/
        public ActionResult Index()
        {
            return this.View();
        }

        public PartialViewResult GetAllUsersInfo()
        {
            IEnumerable<UserInfo> users = this.adminManager.GetAllUsersInfo();
            return this.PartialView("UsersPartial", users);

            // IEnumerable<IntialUserInfo> userList = this.adminManager.GetIntialUserList(null);
            // return PartialView("UsersPartial", userList);
        }

        public PartialViewResult LoadUserInfo()
        {
            IEnumerable<IntialUserInfo> userList = this.adminManager.GetIntialUserList(null);
            UserSearchViewModel model = new UserSearchViewModel();
            userList = userList.OrderBy(x => x.UserName);
            var pagedUserlist = userList.Skip(0).Take(PageSize);
            ViewBag.rowCount = userList.Count();
            ViewBag.pageIndex = 0;

            model.UserList = pagedUserlist;
            model.Page = 1;
            var roleList = AppConfigRegionSource.GetRoles().Where(x => x.Id == 1 || x.Id == 4 || x.Id == 5).Select(x => x.FriendlyName).ToArray();
            var countryList = userList.Select(x => x.Country).Distinct().ToArray();

            model.RoleList = new List<SelectListItem>();
            foreach (var role in roleList)
            {
                model.RoleList.Add(new SelectListItem { Text = role, Value = role });
            }

            model.CountryList = new List<SelectListItem>();
            foreach (var country in countryList)
            {
                model.CountryList.Add(new SelectListItem { Text = country, Value = country });
            }

            model.Alphabets = new char[] { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' };

            return this.PartialView("UserSearchPartial", model);
        }

        public PartialViewResult SearchUsers(UserSearchViewModel model)
        {
            SearchCriteria criteria = new SearchCriteria();
            if (!string.IsNullOrEmpty(model.SelectedAlphabets[0]))
            {
                criteria.NameStartsWith = model.SelectedAlphabets[0].Split(',');
            }

            if (model.ApprovedSelected)
            {
                criteria.RequestStatus = (int)RequestStatus.Approved;
            }
            else if (model.RejectedSelected)
            {
                criteria.RequestStatus = (int)RequestStatus.Reject;
            }

            criteria.SelectedRole = model.SelectedCity;
            criteria.SelectedCountry = model.SelectedCountry;

            IEnumerable<IntialUserInfo> userList = this.adminManager.GetIntialUserList(criteria);
            userList = userList.OrderBy(x => x.UserName);

            if (model.Page < 1)
            {
                model.Page = 1;
            }

            var pagedUserlist = userList.Skip((model.Page - 1) * PageSize).Take(PageSize);
            ViewBag.rowCount = userList.Count();
            ViewBag.pageIndex = model.Page - 1;

            return this.PartialView("UsersPartial", pagedUserlist);
        }

        public PartialViewResult GetAllUsersSummary()
        {
            IEnumerable<UserInfo> users = this.adminManager.GetAllUsersInfo();

            UserSummaryViewModel viewModel = new UserSummaryViewModel();
            viewModel.UsersCount = users.Count();

            foreach (var user in users)
            {
                if (user.SuperAdmin)
                {
                    viewModel.PortalAdminCount++;
                }
                else
                {
                    var roles = user.RolesInfo;

                    if (Convert.ToInt16(roles["fcc"]) == Convert.ToInt16(roles["ofcom"]))
                    {
                        this.CountRoles(roles["fcc"].ToString(), ref viewModel);
                    }
                    else
                    {
                        this.CountRoles(roles["fcc"].ToString(), ref viewModel);
                        this.CountRoles(roles["ofcom"].ToString(), ref viewModel);
                    }
                }
            }

            viewModel.Countries = users.GroupBy(x => x.Country).Select(x => new
            {
                country = x.Key,
                count = x.Count()
            }).OrderByDescending(x => x.count).Select(x => x.country).Take(5).ToArray();

            viewModel.Cities = users.GroupBy(x => x.City).Select(x => new
            {
                city = x.Key,
                count = x.Count()
            }).OrderByDescending(x => x.count).Select(x => x.city).Take(5).ToArray();

            IEnumerable<AccessRequest> elevationRequests = this.adminManager.GetAllAccessRequests();
            viewModel.RequestsCount = elevationRequests.Count();

            return this.PartialView("SummaryPartial", viewModel);
        }        

        public PartialViewResult GetAllRequestsInfo()
        {
            IEnumerable<AccessRequest> requests = this.adminManager.GetAllAccessRequests();
            UserPrincipal userPrincipal = (UserPrincipal)User;

            Parallel.ForEach(
                requests, 
                request =>
                {
                    request.UserName = userPrincipal.UserManager.GetUserProfileFromUserId(request.UserId).UserName;
                });

            return this.PartialView("RequestsPartial", requests);
        }

        public JsonResult UpdateRequest(string userId, string regulatory, string remarks, bool isAccepted)
        {
            UserPrincipal user = (UserPrincipal)User;
            var requestedUser = user.UserManager.GetUserProfileFromUserId(userId);

            AccessRequest request = new AccessRequest();
            request.UserId = userId;
            request.RequestStatus = isAccepted == true ? (int)RequestStatus.Approved : (int)RequestStatus.Reject;
            request.Remarks = remarks.Trim().Replace('\n', ' ');
            request.Regulatory = regulatory;
            request.ApprovedUser = user.UserName;

            var result = this.adminManager.SaveAccessElevationRequest(request);

            if (result)
            {
                this.UserManagementAuditor.UserId = userId;
                this.UserManagementAuditor.TransactionId = this.UserManagementLogger.TransactionId;

                if (isAccepted)
                {
                    this.UserManagementAuditor.Audit(AuditId.RequestAccessElevation, AuditStatus.Success, default(int), "Access elevation request by " + requestedUser.UserName + " is approved by " + user.UserName);
                    this.UserManagementLogger.Log(TraceEventType.Information, LoggingMessageId.PortalUserAccessLevelElevationRequestApproved, requestedUser.UserName + " is approved by " + user.UserName);
                }
                else
                {
                    this.UserManagementAuditor.Audit(AuditId.RequestAccessElevation, AuditStatus.Success, default(int), "Access elevation request by " + requestedUser.UserName + " is rejected by " + user.UserName);
                    this.UserManagementLogger.Log(TraceEventType.Information, LoggingMessageId.PortalUserAccessLevelElevationRequestApproved, requestedUser.UserName + " is rejected by " + user.UserName);
                }

                return this.Json("Updated successfully");
            }

            this.UserManagementLogger.Log(TraceEventType.Error, LoggingMessageId.PortalUserAccessLevelElevationRequest, "Access elevation request updation failed");
            return this.Json("Updation failed");
        }

        public JsonResult GetUserInfo(string userId)
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;
            UserDetails userDetails = userPrincipal.UserManager.GetUserAndAccessDetailsByUserId(userId);

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

            profileModel.ElevationRequests = this.adminManager.GetElevationRequestsByUser(userId).ToList();

            if (userDetails.UserInfo.IsSuperAdmin)
            {
                foreach (var accesslevel in profileModel.AccessInfo)
                {
                    accesslevel.AccessLevel = AccessLevels.SuperAdmin;
                }
            }

            foreach (var request in profileModel.ElevationRequests)
            {
                if (request.RequestStatus == (int)RequestStatus.Approved)
                {
                    request.RequestedAccessLevel = request.CurrentAccessLevel;
                }
            }

            return this.Json(profileModel);
        }

        [HttpPost]
        public JsonResult UpdateAccessDetails(string userId, string[] roles)
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;
            object lockObject = new object();
            List<string> userRoles = new List<string>();

            Parallel.ForEach(
                roles, 
                role =>
                {
                    Hashtable accessLevels = JsonHelper.DeserializeObject<Hashtable>(role);
                    AuthorityAccess access = new AuthorityAccess();
                    access.PartitionKey = userId;
                    access.RowKey = accessLevels["regulatory"].ToString();
                    access.AccessLevel = Convert.ToInt16(accessLevels["role"]);

                    lock (lockObject)
                    {
                        userRoles.Add(accessLevels["role"].ToString());
                    }

                    userPrincipal.UserManager.SaveUserAccessLevel(access);
                });

            UserProfile userProfile = userPrincipal.UserManager.GetUserProfileFromUserId(userId);

            if (userProfile.IsSuperAdmin)
            {
                // down grade portal admin privilages
                if (!userRoles.Contains(((int)AccessLevels.SuperAdmin).ToString()))
                {
                    userProfile.IsSuperAdmin = false;
                    userPrincipal.UserManager.SaveUserProfile(userProfile);

                    this.UserManagementLogger.Log(TraceEventType.Information, LoggingMessageId.PortalUserAccessLevelUpgraded, userPrincipal.UserName + " upgraded access level of " + userProfile.UserName + " to Portal Admin");
                }
            }
            else
            {
                // Upgrade to portal Admin
                if (userRoles.Contains(((int)AccessLevels.SuperAdmin).ToString()))
                {
                    userProfile.IsSuperAdmin = true;
                    userPrincipal.UserManager.SaveUserProfile(userProfile);

                    this.UserManagementLogger.Log(TraceEventType.Information, LoggingMessageId.PortalUserAccessLevelDowngraded, userPrincipal.UserName + " downgraded access level of " + userProfile.UserName + " from Portal Admin");
                }
            }

            this.UserManagementAuditor.UserId = userId;
            this.UserManagementAuditor.TransactionId = this.UserManagementLogger.TransactionId;
            this.UserManagementAuditor.Audit(AuditId.UpdateAccessLevel, AuditStatus.Success, default(int), "Access level of " + userProfile.UserName + " is changed by " + userPrincipal.UserName);
            this.UserManagementLogger.Log(TraceEventType.Information, LoggingMessageId.PoratlUserAccessLevelChange, "Access level of " + userProfile.UserName + " is changed by " + userPrincipal.UserName);

            var rolesInfo = userPrincipal.UserManager.GetUserAccessDetails(userId);

            return this.Json(rolesInfo, JsonRequestBehavior.AllowGet);
        }

        private void CountRoles(string roleId, ref UserSummaryViewModel viewModel)
        {
            switch (roleId)
            {
                case "1": viewModel.PortalUserCount++;
                    break;
                case "4": viewModel.RegionAdminCount++;
                    break;
                case "5": viewModel.PortalAdminCount++;
                    break;
            }
        }
    }
}
