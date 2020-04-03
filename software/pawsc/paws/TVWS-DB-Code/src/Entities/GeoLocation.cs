// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Xml.Serialization;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a Paws GeoLocation instance.
    /// </summary>
    [JsonConverter(typeof(GeoLocationConverter))]
    [Serializable]
    public class GeoLocation
    {
        /// <summary>
        /// Initializes a new instance of the GeoLocation class.
        /// </summary>
        public GeoLocation()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }

        /// <summary>
        /// Gets or sets the Point of the GeoLocation.
        /// </summary>
        [ObjectValidator]       
        [JsonProperty(Constants.PropertyNamePoint)]
        public Ellipse Point { get; set; }

        /// <summary>
        /// Gets or sets the Region of the GeoLocation.
        /// </summary>
        [ObjectValidator]
        [JsonProperty(Constants.PropertyNameRegion)]
        public Polygon Region { get; set; }

        /// <summary>
        /// Gets or sets the confidence of the GeoLocation.
        /// </summary>
        [JsonProperty(Constants.PropertyNameConfidence)]
        public int Confidence { get; set; }

        /// <summary>
        /// Gets or sets the UnKnownTypes of the GeoLocation instance.
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
