// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using Newtonsoft.Json;

    /// <summary>
    /// Represents the properties of a paws Telephone.
    /// </summary>
    public class Telephone
    {
        /// <summary>
        /// Gets or sets the telephone in URI format.
        /// </summary>
        [JsonProperty(Constants.PropertyNameUri)]
        public string Uri { get; set; }
    }
}
