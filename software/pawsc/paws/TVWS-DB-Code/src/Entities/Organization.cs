// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a paws organization.
    /// </summary>
    public class Organization
    {
        /// <summary>
        /// Gets or sets the organization of the paws device.
        /// </summary>
        [JsonProperty(Constants.PropertyNameText)]
        public string Text { get; set; }
    }
}
