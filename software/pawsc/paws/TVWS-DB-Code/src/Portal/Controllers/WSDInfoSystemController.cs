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
    using Microsoft.WhiteSpaces.BusinessManager;
    using Microsoft.WhiteSpaces.Common;    

    [AllowAnonymous]
    public class WSDInfoSystemController : Controller
    {
        private readonly IWhitespacesManager whitespacesManager;

        private readonly string defaultRegion = "United Kingdom";

        public WSDInfoSystemController(IWhitespacesManager whitespacesManager)
        {
            if (whitespacesManager == null)
            {
                throw new ArgumentNullException("whitespacesManager");
            }
            else
            {
                this.whitespacesManager = whitespacesManager;
            }
        }

        [Dependency]
        public IAuditor WSDAuditor { get; set; }

        [Dependency]
        public ILogger WSDLogger { get; set; }
        
        public ActionResult Index()
        {
            return this.View(new WSDInfoInput());
        }

        [HttpPost]
        public JsonResult SubmitRequest(WSDInfoInput input)
        {
            Parameters requestParams = new Parameters
                       {
                           Version = "1.0",
                           Type = "INTERFERENCE_QUERY_REQ",
                           TimeStamp = DateTime.Now.ToString(),
                           StartTime = input.StartDate,
                           EndTime = input.EndDate,
                           Requestor = new DeviceOwner
                           {
                               Owner = new Vcard
                               {
                                   Organization = new Organization
                                   {
                                       Text = "Ofcom"
                                   },
                                   Email = new Email
                                   {
                                       Text = input.Email
                                   }
                               }
                           },
                           Location = new GeoLocation
                           {
                               Point = new Ellipse
                               {
                                   Center = new Point
                                   {
                                       Latitude = input.Latitude,
                                       Longitude = input.Longitude
                                   },
                                   SemiMajorAxis = (float)Convert.ToDouble(input.Radius),
                                   SemiMinorAxis = 0
                               }
                           },
                       };

            this.whitespacesManager.SubmitPawsInterference(requestParams);

            var region = CommonUtility.GetRegionByName(this.defaultRegion);
            this.WSDAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
            this.WSDAuditor.TransactionId = this.WSDLogger.TransactionId;
            this.WSDAuditor.Audit(AuditId.WSDInfoSystem, AuditStatus.Success, default(int), "Spectrum usage information requested with email " + input.Email);
            this.WSDLogger.Log(TraceEventType.Information, LoggingMessageId.PortalWSDInfo, "Spectrum usage information requested with email " + input.Email);

            ViewBag.Message = "Message sent.";

            return this.Json("Request submitted successfully");
        }
    }
}
