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
    /// Represents a Paws Ellipse field.
    /// </summary>
    [JsonConverter(typeof(EllipseConverter))]
    [Serializable]
    public class Ellipse
    {
        /// <summary>
        /// Initializes a new instance of the Ellipse class.
        /// </summary>
        public Ellipse()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }

        /// <summary>
        /// Gets or sets the Center of the ellipse.
        /// </summary>
        [ObjectValidator]
        [Required(ErrorMessage = Constants.ErrorMessageCenterRequired)]
        [JsonProperty(Constants.PropertyNameCenter)]
        public Point Center { get; set; }

        /// <summary>
        /// Gets or sets the semi major axis of the ellipse.
        /// </summary>
        [JsonProperty(Constants.PropertyNameSemiMajorAxis)]
        public float? SemiMajorAxis { get; set; }

        /// <summary>
        /// Gets or sets the semi minor axis of the ellipse.
        /// </summary>
        [JsonProperty(Constants.PropertyNameSemiMinorAxis)]
        public float? SemiMinorAxis { get; set; }

        /// <summary>
        /// Gets or sets the orientation of the ellipse.
        /// </summary>
        [JsonProperty(Constants.PropertyNameOrientation)]
        public float? Orientation { get; set; }

        /// <summary>
        /// Gets or sets all the unknown types of the ellipse.
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
