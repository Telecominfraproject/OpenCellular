// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents Class SpectrumSpec.
    /// </summary>
    public class SpectrumSpec
    {       
        /// <summary>
        /// Gets or sets the Paws Result Rule Set Info.
        /// </summary>
        /// <value>The  Rule Set information.</value>
        [JsonProperty(Constants.PropertyNameRulesetInfo)]
        public RulesetInfo RulesetInfo { get; set; }

        /// <summary>
        /// Gets or sets the spectrum schedules.
        /// </summary>
        /// <value>The spectrum schedules.</value>
        [JsonProperty(Constants.PropertyNameSpectrumSchedules)]
        public SpectrumSchedule[] SpectrumSchedules { get; set; }

        /// <summary>
        /// Gets or sets the time range.
        /// </summary>
        /// <value>The time range.</value>
        [JsonProperty(Constants.PropertyNameTimeRange)]
        public EventTime TimeRange { get; set; }

        /// <summary>
        /// Gets or sets the frequency range.
        /// </summary>
        /// <value>The frequency range.</value>
        [JsonProperty(Constants.PropertyNameFrequencyRanges)]
        public FrequencyRange[] FrequencyRange { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether [needs spectrum report].
        /// </summary>
        /// <value><c>true</c> if [needs spectrum report]; otherwise, <c>false</c>.</value>
        [JsonProperty(Constants.PropertyNameNeedsSpectrumReport)]
        public bool NeedsSpectrumReport { get; set; }

        /// <summary>
        /// Gets or sets the maximum total BW HZ.
        /// </summary>
        /// <value>The maximum total BW HZ.</value>
        [JsonProperty(Constants.PropertyNameMaxTotalBwHz)]
        public float? MaxTotalBwHz { get; set; }

        /// <summary>
        /// Gets or sets the maximum contiguous BW HZ.
        /// </summary>
        /// <value>The maximum contiguous BW HZ.</value>
        [JsonProperty(Constants.PropertyNameMaxContiguousBwHz)]
        public float? MaxContiguousBwHz { get; set; }
    }
}
