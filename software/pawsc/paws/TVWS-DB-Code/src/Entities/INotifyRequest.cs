// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a Paws notify request.
    /// </summary>
    public interface INotifyRequest
    {
        /// <summary>
        /// Gets or sets the device descriptor of a notify request.
        /// </summary>
        [JsonProperty(Constants.PropertyNameDeviceDescriptor)]
        DeviceDescriptor DeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the master device descriptor.
        /// </summary>
        /// <value>The master device descriptor.</value>
        [JsonProperty(Constants.PropertyNameMasterDeviceDescriptors)]
        DeviceDescriptor MasterDeviceDescriptors { get; set; }

        /// <summary>
        /// Gets or sets the location of a notify request.
        /// </summary>
        [JsonProperty(Constants.PropertyNameLocation)]
        GeoLocation Location { get; set; }

        /// <summary>
        /// Gets or sets the master device location.
        /// </summary>
        /// <value>The master device location.</value>
        [JsonProperty(Constants.PropertyNameMasterDeviceLocation)]
        GeoLocation MasterDeviceLocation { get; set; }

        /// <summary>
        /// Gets or sets the spectra of a notify request.
        /// </summary>
        [JsonProperty(Constants.PropertyNameSpectra)]
        Spectrum[] Spectra { get; set; }

        /// <summary>
        /// Gets or sets the antenna characteristics of the paws request.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameAntenna)]
        AntennaCharacteristics Antenna { get; set; }
    }
}
