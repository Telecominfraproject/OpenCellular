// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Xml.Serialization;
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a Paws Device Descriptor.
    /// </summary>
    [JsonConverter(typeof(DeviceDescriptorConverter))]
    public class DeviceDescriptor : TableEntity
    {
        /// <summary>
        /// Initializes a new instance of the DeviceDescriptor class.
        /// </summary>
        public DeviceDescriptor()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }

        /// <summary>
        /// Gets or sets the Serial Number
        /// </summary>
        [StringLength(64, ErrorMessage = Constants.ErrorMessageSerialNumberLength)]
        [JsonProperty(PropertyName = Constants.PropertyNameSerialNumber)]
        [Required(ErrorMessage = Constants.ErrorMessageSerialNumberRequired)]
        public string SerialNumber { get; set; }

        /// <summary>
        /// Gets or sets the Manufacturer Id.
        /// </summary>
        [StringLength(64, ErrorMessage = Constants.ErrorMessageManufacturerIdLength)]
        [JsonProperty(PropertyName = Constants.PropertyNameManufacturerId)]
        public string ManufacturerId { get; set; }

        /// <summary>
        /// Gets or sets the Model Id.
        /// </summary>
        [StringLength(64, ErrorMessage = Constants.ErrorMessageModelIdLength)]
        [JsonProperty(PropertyName = Constants.PropertyNameModelId)]
        public string ModelId { get; set; }

        /// <summary>
        /// Gets or sets the FCC Id.
        /// </summary>
        [StringLength(19, ErrorMessage = Constants.ErrorMessageFccIdLength)]
        [JsonProperty(PropertyName = Constants.PropertyNameFccId)]
        public string FccId { get; set; }

        /// <summary>
        /// Gets or sets the FCC Device Type.
        /// </summary>       
        [JsonProperty(PropertyName = Constants.PropertyNameFccTvbdDeviceType)]
        public string FccTvbdDeviceType { get; set; }

        /// <summary>
        /// Gets or sets the Rule Set Id.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameRulesetIds)]
        public string[] RulesetIds { get; set; }

        /// <summary>
        /// Gets or sets the Device Type
        /// </summary>          
        [StringLength(1, ErrorMessage = Constants.ErrorMessageEtsiDeviceTypeLength)]
        [JsonProperty(PropertyName = Constants.PropertyNameEtsiDeviceType)]
        public string EtsiEnDeviceType { get; set; }

        /// <summary>
        /// Gets or sets the Device Category
        /// </summary>        
        [JsonProperty(PropertyName = Constants.PropertyNameEtsiDeviceCategory)]
        public string EtsiDeviceCategory { get; set; }

        /// <summary>
        /// Gets or sets the Technology Identifier
        /// </summary> 
        [StringLength(64, ErrorMessage = Constants.ErrorMessageTechnologyIdLength)]
        [JsonProperty(PropertyName = Constants.PropertyNameEtsiEnTechnologyId)]
        public string EtsiEnTechnologyId { get; set; }

        /// <summary>
        /// Gets or sets the Device Emission Class
        /// </summary>          
        [StringLength(1, ErrorMessage = Constants.ErrorMessageEtsiEmissionClass)]
        [JsonProperty(PropertyName = Constants.PropertyNameEtsiEnDeviceEmissionsClass)]
        public string EtsiEnDeviceEmissionsClass { get; set; }

        /// <summary>
        /// Gets or sets the unknown field types in the device descriptor.
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
