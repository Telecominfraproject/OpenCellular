// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a paws device validity request.
    /// </summary>
    public interface IDeviceValidityRequest
    {
        /// <summary>
        /// Gets or sets the device descriptors.
        /// </summary>
        [JsonProperty(Constants.PropertyNameDeviceDescriptors)]
        DeviceDescriptor[] DevicesDescriptors { get; set; }

        /// <summary>
        /// Gets or sets the device descriptors.
        /// </summary>
        [JsonProperty(Constants.PropertyNameMasterDeviceDescriptors)]
        DeviceDescriptor MasterDeviceDescriptors { get; set; }
    }
}
