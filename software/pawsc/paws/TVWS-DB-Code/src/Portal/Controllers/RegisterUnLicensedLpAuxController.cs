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

    public class RegisterUnLicensedLpAuxController : Controller
    {
        private readonly IWhitespacesManager whitespacesManager;

        // Default region is added, as of now registration is supported only for fcc. This need to change.
        private string defaultRegion = "United States";

        public RegisterUnLicensedLpAuxController(IWhitespacesManager whitespacesManager, IUserManager userManager)
        {
            this.whitespacesManager = whitespacesManager;
        }

        [Dependency]
        public IAuditor RegistrationAuditor { get; set; }

        [Dependency]
        public ILogger RegistrationLogger { get; set; }
        
        public ActionResult Index()
        {
            var viewModel = new mwc.LicensedLpAuxRegistration
            {
                // default values
                IsRecurred = false,
                IsReoccurenceDaily = true,
            };
            return this.View(viewModel);
        }

        [HttpPost]
        public JsonResult GetUlsFileDetails(string ulsFileNumber)
        {
            var lpAuxInfo = this.whitespacesManager.GetULSFileInfo(ulsFileNumber);

            if (lpAuxInfo == null)
            {
                var region = mwc.CommonUtility.GetRegionByName(this.defaultRegion);
                this.RegistrationAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
                this.RegistrationAuditor.TransactionId = this.RegistrationLogger.TransactionId;
                this.RegistrationAuditor.Audit(AuditId.ULSFileInfo, AuditStatus.Failure, default(int), "No information found for the given ULS File Number " + ulsFileNumber);
                this.RegistrationLogger.Log(TraceEventType.Error, LoggingMessageId.PortalUnlicensedLpAuxRegistration, "No information found for the given ULS File Number " + ulsFileNumber);
            }

            return this.Json(lpAuxInfo, JsonRequestBehavior.AllowGet);
        }

        [HttpPost]
        public JsonResult GetChannelList(double latitude, double longitude)
        {
            ChannelInfo[] channelList = this.whitespacesManager.GetChannelList(IncumbentType.UnlicensedLPAux.ToString(), latitude, longitude);

            if (channelList == null || channelList.Length <= 0)
            {
                var region = mwc.CommonUtility.GetRegionByName(this.defaultRegion);
                this.RegistrationAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
                this.RegistrationAuditor.TransactionId = this.RegistrationLogger.TransactionId;
                this.RegistrationAuditor.Audit(AuditId.ChannelList, AuditStatus.Failure, default(int), "No channels found for the location " + latitude + ',' + longitude);
                this.RegistrationLogger.Log(TraceEventType.Error, LoggingMessageId.PortalUnlicensedLpAuxRegistration, "No channels found for the location " + latitude + ',' + longitude);
            }

            return this.Json(channelList, JsonRequestBehavior.AllowGet);
        }

        public ActionResult Register(mwc.LicensedLpAuxRegistration viewModel)
        {
            JsonResponse response = new JsonResponse();
            if (ModelState.IsValid)
            {
                var userPrincipal = (UserPrincipal)User;
                UserDetails userDetails = userPrincipal.UserManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

                response.Message = this.whitespacesManager.RegisterUnLicensedLpAux(viewModel, userPrincipal.AccessToken);
                response.IsSuccess = true;

                var region = mwc.CommonUtility.GetRegionByName(this.defaultRegion);
                this.RegistrationAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
                this.RegistrationAuditor.TransactionId = this.RegistrationLogger.TransactionId;
                this.RegistrationAuditor.UserId = userDetails.UserInfo.RowKey;

                if (string.Equals(response.Message, Microsoft.WhiteSpaces.Common.Constants.SuccessfullDeviceRegistration, StringComparison.OrdinalIgnoreCase))
                {
                    this.RegistrationAuditor.Audit(AuditId.DeviceRegistration, AuditStatus.Success, default(int), "Unlicensed LpAux incumbent registration submit by " + userDetails.UserInfo.UserName + "is successful");
                    this.RegistrationLogger.Log(TraceEventType.Information, LoggingMessageId.PortalUnlicensedLpAuxRegistration, "Unlicensed LpAux incumbent registration submit by " + userDetails.UserInfo.UserName + "is successful");
                }
                else
                {
                    this.RegistrationAuditor.Audit(AuditId.DeviceRegistration, AuditStatus.Failure, default(int), "Unlicensed LpAux incumbent registration submit by " + userDetails.UserInfo.UserName + "is failed");
                    this.RegistrationLogger.Log(TraceEventType.Error, LoggingMessageId.PortalUnlicensedLpAuxRegistration, "Unlicensed LpAux incumbent registration submit by " + userDetails.UserInfo.UserName + "is successful");
                }
            }

            return this.Json(response);
        }
    }
}
