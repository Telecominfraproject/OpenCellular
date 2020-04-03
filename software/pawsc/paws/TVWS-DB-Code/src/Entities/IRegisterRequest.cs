// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using Newtonsoft.Json;

    /// <summary>
    /// Interface defining the register request parameters.
    /// </summary>
    public interface IRegisterRequest
    {
        /// <summary>
        /// Gets or sets the device descriptor.
        /// </summary>
        [JsonProperty(Constants.PropertyNameDeviceDescriptor)]
        DeviceDescriptor DeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the device location.
        /// </summary>
        [JsonProperty(Constants.PropertyNameLocation)]
        GeoLocation Location { get; set; }

        /// <summary>
        /// Gets or sets the device owner.
        /// </summary>
        [JsonProperty(Constants.PropertyNameDeviceOwner)]
        DeviceOwner DeviceOwner { get; set; }

        /// <summary>
        /// Gets or sets the device antenna characteristics.
        /// </summary>
        [JsonProperty(Constants.PropertyNameAntenna)]
        AntennaCharacteristics Antenna { get; set; }

        /// <summary>
        /// Gets or sets the device registration disposition.
        /// </summary>
        /// [JsonProperty(Constants.prop)]
        RegistrationDisposition RegistrationDisposition { get; set; }
    }
}
