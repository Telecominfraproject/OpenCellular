// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.ComponentModel.DataAnnotations;
    using System.Web.Mvc;
    
    public class OperationalParametersRequest
    {
        public OperationalParametersRequest()
        {
            Dictionary<string, string> requestitems = new Dictionary<string, string>();
            requestitems.Add("Generic", "Generic");
            requestitems.Add("Specific", "Specific");
            this.RequestTypes = new SelectList(requestitems, "Key", "Value");

            Dictionary<string, string> heightItems = new Dictionary<string, string>();
            heightItems.Add("AGL", "AGL");
            heightItems.Add("ASL", "ASL");
            this.HieghtTypes = new SelectList(heightItems, "Key", "Value");

            Dictionary<string, string> emissionClassItems = new Dictionary<string, string>();
            emissionClassItems.Add("1", "Class 1");
            emissionClassItems.Add("2", "Class 2");
            emissionClassItems.Add("3", "Class 3");
            emissionClassItems.Add("4", "Class 4");
            emissionClassItems.Add("5", "Class 5");
            this.EmissionClasses = new SelectList(emissionClassItems, "Key", "Value");

            Dictionary<string, string> deviceCategoriesItems = new Dictionary<string, string>();
            deviceCategoriesItems.Add("Master", "Master");
            deviceCategoriesItems.Add("Slave", "Slave");
            this.DeviceCategories = new SelectList(deviceCategoriesItems, "Key", "Value");

            Dictionary<string, string> deviceTypeItems = new Dictionary<string, string>();
            deviceTypeItems.Add("A", "Type A");
            deviceTypeItems.Add("B", "Type B");
            this.DeviceTypes = new SelectList(deviceTypeItems, "Key", "Value");

            this.Start_time = DateTime.Now;
        }

        public string UniqueId { get; set; }

        [DisplayName("Request Type")]
        public string Request_type { get; set; }

        [DisplayName("Start Time")]
        public DateTime Start_time { get; set; }

        public string Latitude { get; set; }

        public string Longitude { get; set; }

        [DisplayName("Longitude Uncertainty")]
        public string Longitude_uncertainty { get; set; }

        [DisplayName("Latitude Uncertainty")]
        public string Latitude_uncertainty { get; set; }

        [DisplayName("Antenna Height")]
        public string Antenna_Height { get; set; }

        [DisplayName("Antenna Height Type")]
        public string AntennaHeightType { get; set; }

        [DisplayName("Emission Class")]
        public string Emission_class { get; set; }

        [DisplayName("Device Category")]
        public string Device_category { get; set; }

        [DisplayName("Device Type")]
        public string Device_type { get; set; }

        [DisplayName("Technology ID")]
        public string Technology_ID { get; set; }

        public SelectList RequestTypes { get; set; }

        public SelectList HieghtTypes { get; set; }

        public SelectList EmissionClasses { get; set; }

        public SelectList DeviceCategories { get; set; }

        public SelectList DeviceTypes { get; set; }
    }
}
