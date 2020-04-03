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
    /// Represents a Paws Spectrum Schedule.
    /// </summary>
    [JsonConverter(typeof(SpectrumScheduleConverter))]
    public class SpectrumSchedule
    {
        /// <summary>
        /// Gets or sets the scheduled event time.
        /// </summary>
        [Required(ErrorMessage = Constants.ErrorMessageEventTimeRequired)]
        [JsonProperty(PropertyName = Constants.PropertyNameEventTime)]
        public EventTime EventTime { get; set; }

        /// <summary>
        /// Gets or sets the scheduled spectra.
        /// </summary>
        [Required(ErrorMessage = Constants.ErrorMessageSpectraRequired)]
        [JsonProperty(PropertyName = Constants.PropertyNameSpectra)]
        public Spectrum[] Spectra { get; set; }

        /// <summary>
        /// Gets or sets the unknown field types in the device descriptor.
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
