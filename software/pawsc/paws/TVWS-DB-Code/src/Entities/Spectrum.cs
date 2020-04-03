// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Xml.Serialization;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;
    
    /// <summary>
    /// Represents a Paws Spectrum instance.
    /// </summary>
    [JsonConverter(typeof(SpectrumConverter))]
    public class Spectrum : TableEntity
    {
        /// <summary>
        /// Initializes a new instance of the Spectrum class.
        /// </summary>
        public Spectrum()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }

        /// <summary>
        /// Gets or sets the spectrum bandwidth.
        /// </summary>
        [Required(ErrorMessage = Constants.ErrorMessageBandwidthRequired)]
        [JsonProperty(Constants.PropertyNameResolutionBwHz)]
        public double ResolutionBwHz { get; set; }

        /// <summary>
        /// Gets or sets the spectrum frequency ranges.
        /// </summary>
        [Required(ErrorMessage = Constants.ErrorMessageSpectrumProfileRequired)]
        [JsonProperty(Constants.PropertyNameProfiles)]
        [ObjectCollectionValidator]
        public SpectrumProfile[] Profiles { get; set; }

        /// <summary>
        /// Gets or sets the profile.
        /// </summary>
        /// <value>The profile.</value>
        public SpectrumProfile Profile { get; set; }

        /// <summary>
        /// Gets or sets the unknown field types in the Spectrum.
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
