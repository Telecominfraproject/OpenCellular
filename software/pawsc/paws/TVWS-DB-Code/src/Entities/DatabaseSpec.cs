// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using Newtonsoft.Json;

    /// <summary>
    /// Contains the Paws Database spec.
    /// </summary>
    [JsonConverter(typeof(DatabaseSpecConverter))]
    public class DatabaseSpec
    {
        /// <summary>
        /// Gets or sets the name.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameName)]
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the Uri.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameUri)]
        public string Uri { get; set; }
    }
}
