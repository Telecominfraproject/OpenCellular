// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;    
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a Paws rule-set Info.
    /// </summary>
    [JsonConverter(typeof(RulesetInfoConverter))]
    public class RulesetInfo 
    {
        /// <summary>
        /// Gets or sets the Authority.
        /// </summary>
        [JsonProperty(Constants.PropertyNameAuthority)]
        public string Authority { get; set; }

        /// <summary>
        /// Gets or sets the Maximum Location Change.
        /// </summary>
        [JsonProperty(Constants.PropertyNameMaxLocationChange)]
        public double? MaxLocationChange { get; set; }

        /// <summary>
        /// Gets or sets the maximum Polling Seconds.
        /// </summary>
        [JsonProperty(Constants.PropertyNameMaxPollingSecs)]
        public int? MaxPollingSecs { get; set; }

        /// <summary>
        /// Gets or sets the Maximum EIRP.
        /// </summary>
        [JsonProperty(Constants.PropertyNameMaxEirpHz)]
        public double? MaxEirpHz { get; set; }

        /// <summary>
        /// Gets or sets the Maximum Channel Bandwidth.
        /// </summary>
        [JsonProperty(Constants.PropertyNameMaxNominalChannelBwMhz)]
        public double? MaxNominalChannelBwMhz { get; set; }

        /// <summary>
        /// Gets or sets the Maximum Channel Bandwidth.
        /// </summary>
        [JsonProperty(Constants.PropertyNameMaxTotalBwMhz)]
        public double? MaxTotalBwMhz { get; set; }

        /// <summary>
        /// Gets or sets the rule set Ids.
        /// </summary>
        [JsonProperty(Constants.PropertyNameRulesetIds)]
        public string RulesetId { get; set; }
    }
}
