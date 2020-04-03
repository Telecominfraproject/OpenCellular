// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal.Controllers
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
    using Microsoft.WhiteSpaces.BusinessManager;
    using Microsoft.WhiteSpaces.Common;
    using System.Threading.Tasks;

    public class RegisterMVPDController : Controller
    {
        private readonly IWhitespacesManager whitespacesManager;

        // Default region is added, as of now registration is supported only for fcc. This need to change.
        private string defaultRegion = "United States";

        public RegisterMVPDController(IWhitespacesManager whitespacesManager, IUserManager userManager)
        {
            this.whitespacesManager = whitespacesManager;
        }

        [Dependency]
        public IAuditor RegistrationAuditor { get; set; }

        [Dependency]
        public ILogger RegistrationLogger { get; set; }

        // GET: /RegisterMVPD/
        public ActionResult Index()
        {
            var userPrincipal = (UserPrincipal)User;
            UserDetails userDetails = userPrincipal.UserManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

            var viewModel = new MVPDRegisterViewModel
            {
                Name = userDetails.UserInfo.FirstName + " " + userDetails.UserInfo.LastName,
                Country = userDetails.UserInfo.Country,
                City = userDetails.UserInfo.City,
                State = userDetails.UserInfo.State,
                Email = userDetails.UserInfo.PreferredEmail,
                Address1 = userDetails.UserInfo.Address1,
                Address2 = userDetails.UserInfo.Address2,
                Phone = userDetails.UserInfo.Phone
            };

            return this.View(viewModel);
        }

        public ActionResult Register(MVPDRegisterViewModel viewModel)
        {
            JsonResponse response = new JsonResponse();

            var userPrincipal = (UserPrincipal)User;
            UserDetails userDetails = userPrincipal.UserManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

            if (ModelState.IsValid)
            {
                MvpdRegistrationInfo info = new MvpdRegistrationInfo
                {
                    CallSign = viewModel.CallSign,
                    Channel = Convert.ToInt32(viewModel.Channel),
                    Name = viewModel.FriendlyName,
                    RecieveLocation = new Whitespace.Entities.Location
                    {
                        Latitude = Convert.ToDouble(viewModel.MVPDLocationLatittude),
                        Longitude = Convert.ToDouble(viewModel.MVPDLocationLongitude),
                    },
                    TransmitLocation = new Whitespace.Entities.Location
                    {
                        Latitude = Convert.ToDouble(viewModel.TransmitterLatittude),
                        Longitude = Convert.ToDouble(viewModel.TransmitterLongitude),
                    },
                    RegistrantInfo = new Whitespace.Entities.Versitcard.VCard
                    {
                        Org = new Whitespace.Entities.Versitcard.Organization { OrganizationName = viewModel.CableCompanyName },
                        Address = new Whitespace.Entities.Versitcard.Address
                        {
                            Street = viewModel.Address1,
                            Region = viewModel.City,
                            Country = viewModel.Country
                        }
                    },

                    Contact = new Whitespace.Entities.Versitcard.VCard
                   {
                       Title = new Whitespace.Entities.Versitcard.Title { Text = viewModel.FriendlyName },
                       Address = new Whitespace.Entities.Versitcard.Address
                       {
                           Street = viewModel.Address1,
                           Region = viewModel.City,
                           Country = viewModel.Country
                       },
                       Telephone = new Whitespace.Entities.Versitcard.Telephone[] { new Whitespace.Entities.Versitcard.Telephone { TelephoneNumber = viewModel.Phone } },
                       Email = new Whitespace.Entities.Versitcard.Email[] { new Whitespace.Entities.Versitcard.Email { EmailAddress = viewModel.Email } }
                   }
                };

                response.Message = this.whitespacesManager.RegisterMVPD(info, userPrincipal.AccessToken);
                response.IsSuccess = true;

                var region = CommonUtility.GetRegionByName(this.defaultRegion);
                this.RegistrationAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
                this.RegistrationAuditor.TransactionId = this.RegistrationLogger.TransactionId;
                this.RegistrationAuditor.UserId = userDetails.UserInfo.RowKey;

                if (string.Equals(response.Message, Microsoft.WhiteSpaces.Common.Constants.SuccessfullDeviceRegistration, StringComparison.OrdinalIgnoreCase))
                {
                    this.RegistrationAuditor.Audit(AuditId.DeviceRegistration, AuditStatus.Success, default(int), "MVPD Incumbent registration submit by " + userDetails.UserInfo.UserName + "is successful");
                    this.RegistrationLogger.Log(TraceEventType.Information, LoggingMessageId.PortalMvpdRegistration, "MVPD Incumbent registration submit by " + userDetails.UserInfo.UserName + "is successful");
                }
                else
                {
                    this.RegistrationAuditor.Audit(AuditId.DeviceRegistration, AuditStatus.Failure, default(int), "MVPD Incumbent registration submit by " + userDetails.UserInfo.UserName + "is failed");
                    this.RegistrationLogger.Log(TraceEventType.Error, LoggingMessageId.PortalMvpdRegistration, "MVPD Incumbent registration submit by " + userDetails.UserInfo.UserName + "is failed due to error " + response.Message);
                }
            }

            return this.Json(response);
        }

        [HttpPost]
        public JsonResult GetCallSignInfo(string latitude, string longitude)
        {
            var userPrincipal = (UserPrincipal)User;

            var location = new Location
            {
                Latitude = Convert.ToDouble(latitude),
                Longitude = Convert.ToDouble(longitude)
            };

            var tvstations = this.whitespacesManager.GetNearByTvStations(location, userPrincipal.AccessToken);

            if (tvstations != null)
            {
                return this.Json(tvstations, JsonRequestBehavior.AllowGet);
            }
            else
            {
                this.RegistrationLogger.Log(TraceEventType.Error, LoggingMessageId.PortalMvpdRegistration, "No nearby TV stations found for location " + latitude + "," + longitude);

                return null;
            }
        }


        public async Task<JsonResult> GetMVPDCallsignInfo(string callsign)
        {
            var tvstation = await this.whitespacesManager.GetMvpdCallsignInfoAsync(callsign);

            if (tvstation != null)
            {
                return this.Json(tvstation, JsonRequestBehavior.AllowGet);
            }
            else
            {
                this.RegistrationLogger.Log(TraceEventType.Error, LoggingMessageId.PortalMvpdRegistration, "No TV station found with callsign " + callsign);

                return null;
            }
        }

        public JsonResult GetContourData(string callsign, int channel)
        {
            var incumbents = this.whitespacesManager.GetIncumbents(IncumbentType.MVPD.ToString(), this.defaultRegion, new List<int> { channel });
            if (incumbents.Count > 0)
            {
                var requiredIncumbent = incumbents.Where(x => x.CallSign == callsign).FirstOrDefault();

                if (requiredIncumbent != null)
                {
                    return this.Json(requiredIncumbent.ContourPoints, JsonRequestBehavior.AllowGet);
                }
                else
                {
                    this.RegistrationAuditor.Audit(AuditId.CallSignInfo, AuditStatus.Failure, default(int), "No contours found for given channel " + channel + " and callsign" + callsign);
                    this.RegistrationLogger.Log(TraceEventType.Error, LoggingMessageId.PortalMvpdRegistration, "No contours found for given channel " + channel + " and callsign" + callsign);

                    return null;
                }
            }
            else
            {
                var region = CommonUtility.GetRegionByName(this.defaultRegion);
                this.RegistrationAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
                this.RegistrationAuditor.TransactionId = this.RegistrationLogger.TransactionId;
                this.RegistrationAuditor.Audit(AuditId.CallSignInfo, AuditStatus.Failure, default(int), "No call signs found for channel " + channel + " in " + this.defaultRegion);
                this.RegistrationLogger.Log(TraceEventType.Error, LoggingMessageId.PortalMvpdRegistration, "No call signs found for channel " + channel + " in " + this.defaultRegion);
            }

            return null;
        }
    }
}
