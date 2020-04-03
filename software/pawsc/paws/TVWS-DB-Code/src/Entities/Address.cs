// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a PAWS address.
    /// </summary>
    public class Address : TableEntity
    {
        /// <summary>
        /// Gets or sets the street address.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameStreet)]
        public string Street { get; set; }

        /// <summary>
        /// Gets or sets the Locality.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameLocality)]
        public string Locality { get; set; }

        /// <summary>
        /// Gets or sets the Region.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameRegion)]
        public string Region { get; set; }

        /// <summary>
        /// Gets or sets the zip code.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameCode)]
        public int Code { get; set; }

        /// <summary>
        /// Gets or sets the Country.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameCountry)]
        public string Country { get; set; }
    }
}
