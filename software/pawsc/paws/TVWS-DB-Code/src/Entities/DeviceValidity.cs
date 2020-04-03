// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.ComponentModel.DataAnnotations;
    using Newtonsoft.Json;

    /// <summary>
    /// Class for determining paws device validity.
    /// </summary>
    [JsonConverter(typeof(DeviceValidityConverter))]
    public class DeviceValidity
    {
        /// <summary>
        /// Gets or sets the paws device descriptor.
        /// </summary>
        [JsonProperty(Constants.PropertyNameDeviceDescriptor)]
        [Required(ErrorMessage = Constants.ErrorMessageDeviceValidityDeviceDescriptorRequired)]
        public DeviceDescriptor DeviceDesciptor { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the device is valid.
        /// </summary>
        [JsonProperty(Constants.PropertyNameIsValid)]
        [Required(ErrorMessage = Constants.ErrorMessageDeviceValidityIsValidRequired)]
        public bool IsValid { get; set; }

        /// <summary>
        /// Gets or sets a reason when the device is not valid.
        /// </summary>
        [StringLength(128, ErrorMessage = Constants.ErrorMessageReasonLength)]
        [JsonProperty(Constants.PropertyNameReason)]
        public string Reason { get; set; }
    }
}
