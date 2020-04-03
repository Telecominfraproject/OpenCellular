// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Xml.Serialization;
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;

    /// <summary>
    /// Class for the Paws Event time.
    /// </summary>
    [JsonConverter(typeof(EventTimeConverter))]
    public class EventTime : TableEntity
    {
        /// <summary>
        /// Gets or sets the start time of the paws event.
        /// </summary>
        [Required(ErrorMessage = Constants.ErrorMessageStartTimeRequired)]
        [JsonProperty(Constants.PropertyNameStartTime)]
        public string StartTime { get; set; }

        /// <summary>
        /// Gets or sets the end time of the paws event.
        /// </summary>
        [Required(ErrorMessage = Constants.ErrorMessageStopTimeRequired)]
        [JsonProperty(Constants.PropertyNameStopTime)]
        public string StopTime { get; set; }

        /// <summary>
        /// Gets or sets the unknown field types in the paws event..
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
