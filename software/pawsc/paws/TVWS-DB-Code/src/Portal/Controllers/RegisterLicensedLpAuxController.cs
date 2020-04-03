// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal.Controllers
{
    using System;
    using System.Diagnostics;
    using System.Web.Mvc;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.AzureOAuthProvider;
    using Microsoft.WhiteSpaces.BusinessManager;   
    using mwc = Microsoft.WhiteSpaces.Common;
    
    public class RegisterLicensedLpAuxController : Controller
    {
        private readonly IWhitespacesManager whitespacesManager;

        // Default region is added, as of now registration is supported only for fcc. This need to change.
        private string defaultRegion = "United States";

        public RegisterLicensedLpAuxController(IWhitespacesManager whitespacesManager, IUserManager userManager)
        {
            this.whitespacesManager = whitespacesManager;
        }

        [Dependency]
        public IAuditor RegistrationAuditor { get; set; }

        [Dependency]
        public ILogger RegistrationLogger { get; set; }
        
        public ActionResult Index()
        {
            var userPrincipal = (UserPrincipal)User;
            UserDetails userDetails = userPrincipal.UserManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

            var viewModel = new mwc.LicensedLpAuxRegistration
            {
                Channellist = new ChannelInfo[] { new ChannelInfo { ChannelId = 6 }, new ChannelInfo { ChannelId = 8 }, new ChannelInfo { ChannelId = 9 }, new ChannelInfo { ChannelId = 16 }, new ChannelInfo { ChannelId = 18 }, new ChannelInfo { ChannelId = 19 } },
                Name = userDetails.UserInfo.FirstName + " " + userDetails.UserInfo.LastName,
                Country = userDetails.UserInfo.Country,
                City = userDetails.UserInfo.City,
                Email = userDetails.UserInfo.PreferredEmail,
                Address1 = userDetails.UserInfo.Address1,
                Address2 = userDetails.UserInfo.Address2,
                Phone = userDetails.UserInfo.Phone,
                IsRecurred = false,
                IsReoccurenceDaily = true,
            };

            return this.View(viewModel);
        }

        [HttpPost]
        public ActionResult Register(mwc.LicensedLpAuxRegistration viewModel)
        {
            var userPrincipal = (UserPrincipal)User;
            UserDetails userDetails = userPrincipal.UserManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

            JsonResponse response = new JsonResponse();

            if (ModelState.IsValid)
            {
                response.Message = this.whitespacesManager.RegisterLicensedLpAux(viewModel, userPrincipal.AccessToken);
                response.IsSuccess = true;

                var region = mwc.CommonUtility.GetRegionByName(this.defaultRegion);
                this.RegistrationAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
                this.RegistrationAuditor.TransactionId = this.RegistrationLogger.TransactionId;
                this.RegistrationAuditor.UserId = userDetails.UserInfo.RowKey;

                if (string.Equals(response.Message, Microsoft.WhiteSpaces.Common.Constants.SuccessfullDeviceRegistration, StringComparison.OrdinalIgnoreCase))
                {
                    this.RegistrationAuditor.Audit(AuditId.DeviceRegistration, AuditStatus.Success, default(int), "Licensed LpAux incumbent registration submit by " + userDetails.UserInfo.UserName + "is successful");
                    this.RegistrationLogger.Log(TraceEventType.Information, LoggingMessageId.PortalLicensedLpAuxRegistration, "Licensed LpAux incumbent Incumbent registration submit by " + userDetails.UserInfo.UserName + "is successful");
                }
                else
                {
                    this.RegistrationAuditor.Audit(AuditId.DeviceRegistration, AuditStatus.Failure, default(int), "Licensed LpAux incumbent registration submit by " + userDetails.UserInfo.UserName + "is failed");
                    this.RegistrationLogger.Log(TraceEventType.Error, LoggingMessageId.PortalLicensedLpAuxRegistration, "Licensed LpAux incumbent Incumbent registration submit by " + userDetails.UserInfo.UserName + "is failed due to error " + response.Message);
                }
            }

            return this.Json(response);
        }

        [HttpPost]
        public JsonResult GetChannelList(double latitude, double longitude)
        {
            ChannelInfo[] channelList = this.whitespacesManager.GetChannelList(IncumbentType.LPAux.ToString(), latitude, longitude);

            if (channelList == null || channelList.Length <= 0)
            {
                var region = mwc.CommonUtility.GetRegionByName(this.defaultRegion);
                this.RegistrationAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
                this.RegistrationAuditor.TransactionId = this.RegistrationLogger.TransactionId;
                this.RegistrationAuditor.Audit(AuditId.ChannelList, AuditStatus.Failure, default(int), "No channels found for the location " + latitude + ',' + longitude);
                this.RegistrationLogger.Log(TraceEventType.Error, LoggingMessageId.PortalLicensedLpAuxRegistration, "No channels found for the location " + latitude + ',' + longitude);
            }

            return this.Json(channelList, JsonRequestBehavior.AllowGet);
        }
    }
}
