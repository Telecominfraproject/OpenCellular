// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Xml.Serialization;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a Paws Point instance.
    /// </summary>
    [JsonConverter(typeof(PointConverter))]
    [Serializable]
    public class Point
    {
        /// <summary>
        /// Initializes a new instance of the Point class.
        /// </summary>
        public Point()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }

        /// <summary>
        /// Gets or sets the Latitude.
        /// </summary>
        [Required(ErrorMessage = Constants.ErrorMessageLatitudeRequired)]
        [JsonProperty(Constants.PropertyNameLatitude)]
        public string Latitude { get; set; }

        /// <summary>
        /// Gets or sets the Longitude.
        /// </summary>
        [Required(ErrorMessage = Constants.ErrorMessageLongitudeRequired)]
        [JsonProperty(Constants.PropertyNameLongitude)]
        public string Longitude { get; set; }

        /// <summary>
        /// Gets or sets the UnKnownTypes of a Point.
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
