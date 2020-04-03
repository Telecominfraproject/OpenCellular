// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a Paws rule-set Info.
    /// </summary>   
    public class RuleSetInfoEntity : TableEntity
    {
        /// <summary>
        /// Gets or sets the Authority.
        /// </summary>        
        public string Authority { get; set; }

        /// <summary>
        /// Gets or sets the Maximum Location Change.
        /// </summary>       
        public double? MaxLocationChange { get; set; }

        /// <summary>
        /// Gets or sets the maximum Polling Seconds.
        /// </summary>        
        public int? MaxPollingSecs { get; set; }

        /// <summary>
        /// Gets or sets the Maximum EIRP.
        /// </summary>       
        public double? MaxEirpHz { get; set; }

        /// <summary>
        /// Gets or sets the Maximum Channel Bandwidth.
        /// </summary>        
        public double? MaxNominalChannelBwMhz { get; set; }

        /// <summary>
        /// Gets or sets the Maximum Channel Bandwidth.
        /// </summary>       
        public double? MaxTotalBwMhz { get; set; }

        /// <summary>
        /// Gets or sets the rule set Ids.
        /// </summary>        
        public string RulesetId { get; set; }
    }
}
