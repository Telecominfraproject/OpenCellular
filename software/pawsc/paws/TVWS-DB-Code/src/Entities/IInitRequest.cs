// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System.Runtime.Serialization;
    using System.Xml.Serialization;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a paws init request.
    /// </summary>
    public interface IInitRequest
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
    }
}
