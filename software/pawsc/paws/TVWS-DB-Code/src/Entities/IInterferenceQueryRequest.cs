// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using Newtonsoft.Json;

    /// <summary>
    /// Interface defining the InterferenceQuery request parameters.
    /// </summary>
    public interface IInterferenceQueryRequest
    {
        /// <summary>
        /// Gets or sets the device descriptor.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameTimeStamp)]
        string TimeStamp { get; set; }

        /// <summary>
        /// Gets or sets the device location.
        /// </summary>
        [JsonProperty(Constants.PropertyNameLocation)]
        GeoLocation Location { get; set; }

        /// <summary>
        /// Gets or sets the device owner.
        /// </summary>
        [JsonProperty(Constants.PropertyNameRequestor)]
        DeviceOwner Requestor { get; set; }

        /// <summary>
        /// Gets or sets the Start Time.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameInterferenceQueryStartTime)]
        string StartTime { get; set; }

        /// <summary>
        /// Gets or sets the End Time.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameInterferenceQueryEndTime)]
        string EndTime { get; set; }

        /// <summary>
        /// Gets or sets the Request Type.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameInterferenceQueryRequestType)]
        string RequestType { get; set; }
    }
}
