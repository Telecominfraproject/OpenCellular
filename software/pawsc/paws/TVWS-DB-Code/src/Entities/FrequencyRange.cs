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
    /// Contains the Paws Frequency Range fields.
    /// </summary>
    [JsonConverter(typeof(FrequencyRangeConverter))]
    public class FrequencyRange : TableEntity
    {
        /// <summary>
        /// Gets or sets the StartHz.
        /// </summary>
        [Required(ErrorMessage = Constants.ErrorMessageStartHzRequired)]
        [JsonProperty(Constants.PropertyNameStartHz)]
        public double StartHz { get; set; }

        /// <summary>
        /// Gets or sets the StopHz.
        /// </summary>
        [Required(ErrorMessage = Constants.ErrorMessageStopHzRequired)]
        [JsonProperty(Constants.PropertyNameStopHz)]
        public double StopHz { get; set; }

        /// <summary>
        /// Gets or sets the MaxPowerDB.
        /// </summary>
        [JsonProperty(Constants.PropertyNameMaxPowerDBm)]
        public double MaxPowerDBm { get; set; }

        /// <summary>
        /// Gets or sets the ChannelId.
        /// </summary>
        [JsonProperty(Constants.PropertyNameChannelId)]
        public string ChannelId { get; set; }

        /// <summary>
        /// Gets or sets the unknown field types in the Frequency Range.
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
