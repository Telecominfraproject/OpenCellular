// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a paws available spectrum request.
    /// </summary>
    public interface IAvailableSpectrumRequest
    {
        /// <summary>
        /// Gets or sets the device descriptor of the paws request.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameDeviceDescriptor)]
        DeviceDescriptor DeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the location of the paws request.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameLocation)]
        GeoLocation Location { get; set; }

        /// <summary>
        /// Gets or sets the master device location.
        /// </summary>
        /// <value>The master device location.</value>
        [JsonProperty(PropertyName = Constants.PropertyNameMasterDeviceLocation)]
        GeoLocation MasterDeviceLocation { get; set; }

        /// <summary>
        /// Gets or sets the antenna characteristics of the paws request.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameAntenna)]
        AntennaCharacteristics Antenna { get; set; }

        /// <summary>
        /// Gets or sets the owner of the paws request.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameOwner)]
        DeviceOwner Owner { get; set; }

        /// <summary>
        /// Gets or sets the Capabilities of the paws request device.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameCapabilities)]
        DeviceCapabilities Capabilities { get; set; }

        /// <summary>
        /// Gets or sets the master device descriptors of the paws request.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameMasterDeviceDescriptors)]
        DeviceDescriptor MasterDeviceDescriptors { get; set; }

        /// <summary>
        /// Gets or sets the request type of the paws request.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameRequestType)]
        string RequestType { get; set; }

        /// <summary>
        /// Gets or sets the device owner.
        /// </summary>
        /// <value>The device owner.</value>
        [JsonProperty(PropertyName = Constants.PropertyNameDeviceOwner)]
        DeviceOwner DeviceOwner { get; set; }
    }
}
