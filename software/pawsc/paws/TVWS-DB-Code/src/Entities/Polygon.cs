// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Xml.Serialization;
    using Newtonsoft.Json;
    using Practices.EnterpriseLibrary.Validation.Validators;

    /// <summary>
    ///     Represents a Polygon instance.
    /// </summary>
    [JsonConverter(typeof(PolygonConverter))]
    [Serializable]
    public class Polygon
    {
        /// <summary>
        ///     Initializes a new instance of the Polygon class.
        /// </summary>
        public Polygon()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }

        /// <summary>
        ///     Gets or sets the Exterior.
        /// </summary>
        [ObjectValidator]
        [JsonProperty(Constants.PropertyNameExterior)]
        public Point[] Exterior { get; set; }

        /// <summary>
        ///     Gets or sets the UnKnownTypes of a Polygon.
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
