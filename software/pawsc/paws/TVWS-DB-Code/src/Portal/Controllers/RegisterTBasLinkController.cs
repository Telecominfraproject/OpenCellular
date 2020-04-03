// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal.Controllers
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Text;   
    using System.Web.Mvc;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.AzureOAuthProvider;
    using Microsoft.WhiteSpaces.BusinessManager;
    using Microsoft.WhiteSpaces.Common;

    public class RegisterTBasLinkController : Controller
    {
        private readonly IWhitespacesManager whitespacesManager;

        // Default region is added, as of now registration is supported only for fcc. This need to change.
        private string defaultRegion = "United States";

        public RegisterTBasLinkController(IWhitespacesManager whitespacesManager, IUserManager userManager)
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

            var viewModel = new TBasLinkRegistration
            {
                Name = userDetails.UserInfo.FirstName + " " + userDetails.UserInfo.LastName,
                Country = userDetails.UserInfo.Country,
                City = userDetails.UserInfo.City,
                Email = userDetails.UserInfo.PreferredEmail,
                Address1 = userDetails.UserInfo.Address1,
                Address2 = userDetails.UserInfo.Address2,
                State = userDetails.UserInfo.State,
                Phone = userDetails.UserInfo.Phone
            };

            return this.View(viewModel);
        }

        [HttpPost]
        public ActionResult Register(TBasLinkRegistration viewModel)
        {
            JsonResponse response = new JsonResponse();

            if (ModelState.IsValid)
            {
                var userPrincipal = (UserPrincipal)User;
                UserDetails userDetails = userPrincipal.UserManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

                response.Message = this.whitespacesManager.RegisterTBasLinks(viewModel, userPrincipal.AccessToken);
                response.IsSuccess = true;

                var region = CommonUtility.GetRegionByName(this.defaultRegion);
                this.RegistrationAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
                this.RegistrationAuditor.TransactionId = this.RegistrationLogger.TransactionId;
                this.RegistrationAuditor.UserId = userDetails.UserInfo.RowKey;

                if (string.Equals(response.Message, Microsoft.WhiteSpaces.Common.Constants.SuccessfullDeviceRegistration, StringComparison.OrdinalIgnoreCase))
                {
                    this.RegistrationAuditor.Audit(AuditId.DeviceRegistration, AuditStatus.Success, default(int), "Temp BAS incumbent registration submit by " + userDetails.UserInfo.UserName + "is successful");
                    this.RegistrationLogger.Log(TraceEventType.Information, LoggingMessageId.PortalTempBasRegistration, "Temp BAS incumbent Incumbent registration submit by " + userDetails.UserInfo.UserName + "is successful");
                }
                else
                {
                    this.RegistrationAuditor.Audit(AuditId.DeviceRegistration, AuditStatus.Failure, default(int), "TempBAS   incumbent registration submit by " + userDetails.UserInfo.UserName + "is failed");
                    this.RegistrationLogger.Log(TraceEventType.Error, LoggingMessageId.PortalTempBasRegistration, "TempBAS incumbent Incumbent registration submit by " + userDetails.UserInfo.UserName + "is failed due to error " + response.Message);
                }
            }
            else
            {
                IEnumerable<ModelError> errors = ModelState.Values.SelectMany(x => x.Errors);
                response.IsValidationError = true;
                response.ValidationErrors = errors.Select(x => x.ErrorMessage).ToArray();
            }

            return this.Json(response);
        }

        public JsonResult GetContourData(string callsign, string channels)
        {
            int[] channelList = JsonHelper.DeserializeObject<int[]>(channels);

            var incumbents = this.whitespacesManager.GetIncumbents(IncumbentType.MVPD.ToString(), "United States", channelList);
            if (incumbents.Count > 0)
            {
                var requiredIncumbent = incumbents.Where(x => x.CallSign == callsign).FirstOrDefault();

                if (requiredIncumbent != null)
                {
                    return this.Json(requiredIncumbent.ContourPoints, JsonRequestBehavior.AllowGet);                    
                }
                else
                {
                    StringBuilder channelStringBuilder = new StringBuilder();

                     foreach (int channel in channelList)
                     {
                         channelStringBuilder.Append(channel);

                         if (channel != channelList[channelList.Length - 1])
                         {
                             channelStringBuilder.Append(",");
                         }
                     }

                     this.RegistrationLogger.Log(TraceEventType.Error, LoggingMessageId.PortalMvpdRegistration, "No contours found for given channels " + channelStringBuilder.ToString() + " and callsign" + callsign);
                }
            }          

            return null;
        }
    }
}
