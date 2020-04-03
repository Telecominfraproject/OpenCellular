// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal.Controllers
{
    using System;
    using System.Text;
    using System.Web.Mvc;
    using Microsoft.WhiteSpaces.BusinessManager;
    using Microsoft.WhiteSpaces.Common;

    [AllowAnonymous]
    public class PublicDataController : Controller
    {
        private readonly IPublicDataManager dataManager;
        private readonly IRegionSource regionSource;

        public PublicDataController(IPublicDataManager dataManager, IRegionSource regionSource)
        {
            if (dataManager == null)
            {
                throw new ArgumentNullException("dataManager");
            }

            if (regionSource == null)
            {
                throw new ArgumentException("regionSource");
            }

            this.dataManager = dataManager;
            this.regionSource = regionSource;
        }

        public ActionResult Index()
        {
            this.ViewData["availableRegions"] = this.regionSource.GetAvailableRegions();

            return this.View();
        }

        public FileResult GetPublicDataWithEvents(string entityType)
        {
            var csvString = this.dataManager.GetPublicDataWithEvents(entityType);
            var byteArray = Encoding.ASCII.GetBytes(csvString);

            string fileName = string.Empty;
            switch (entityType)
            {
                case "MVPD": fileName = "cablevideomvpd.csv";
                    break;
                case "TBAS": fileName = "temporarybaslinks.csv";
                    break;
                case "LPAux": fileName = "lowpoweraux.csv";
                    break;
                case "FIXED_TVBD": fileName = "fixedtvbd.csv";
                    break;
                default: fileName = entityType.ToLower() + ".csv";
                    break;
            }

            return this.File(byteArray, System.Net.Mime.MediaTypeNames.Application.Octet, fileName);
        }

        public FileResult GetPublicData(string entityType)
        {
            var csvString = this.dataManager.GetPublicData(entityType);
            var byteArray = Encoding.ASCII.GetBytes(csvString);

            string fileName = string.Empty;
            switch (entityType)
            {
                case "TV_US": fileName = "usstations.csv";
                    break;
                case "TVCA_MX": fileName = "canadamexicostations.csv";
                    break;
                case "TV_TRANSLATOR": fileName = "tvtranslators.csv";
                    break;
                case "BAS": fileName = "baslinks.csv";
                    break;
                case "PLCMRS": fileName = "plmrs-cmrs.csv";
                    break;
                default: fileName = entityType.ToLower() + ".csv";
                    break;
            }

            return this.File(byteArray, System.Net.Mime.MediaTypeNames.Application.Octet, fileName);
        }

        public FileResult GetAuthorizedDeviceModels()
        {
            var csvString = this.dataManager.GetAuthorizedDeviceModels();
            var byteArray = Encoding.ASCII.GetBytes(csvString);

            return this.File(byteArray, System.Net.Mime.MediaTypeNames.Application.Octet, "FCC_WhiteSpace_Authorization.csv");
        }

        public FileResult GetUlsCallSigns()
        {
            var csvString = this.dataManager.GetUlsCallSigns();
            var byteArray = Encoding.ASCII.GetBytes(csvString);

            return this.File(byteArray, System.Net.Mime.MediaTypeNames.Application.Octet, "ulscallsigns.csv");
        }

        public FileResult GetUlsFileNumbers()
        {
            var csvString = this.dataManager.GetUlsFileNumbers();
            var byteArray = Encoding.ASCII.GetBytes(csvString);

            return this.File(byteArray, System.Net.Mime.MediaTypeNames.Application.Octet, "ulsfilenumbers.csv");
        }
    }
}
