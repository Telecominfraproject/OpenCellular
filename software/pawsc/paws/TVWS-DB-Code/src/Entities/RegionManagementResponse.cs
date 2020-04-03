// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a Paws Response.
    /// </summary>
    public class RegionManagementResponse
    {
        /// <summary>
        /// Gets or sets the paws response.
        /// </summary>
        [JsonProperty(Constants.PropertyNameResult)]
        public Result Result { get; set; }

        /// <summary>
        /// Gets or sets the paws response.
        /// </summary>
        [JsonProperty(Constants.PropertyNameError)]
        public Result Error { get; set; }
    }
}
