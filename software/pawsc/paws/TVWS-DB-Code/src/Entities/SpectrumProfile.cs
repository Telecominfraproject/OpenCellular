// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents Class SpectrumProfile.
    /// </summary>
    public class SpectrumProfile : TableEntity
    {
        /// <summary>
        /// Gets or sets the StartHz.
        /// </summary>
        /// <value>The hz.</value>
        [Required(ErrorMessage = Constants.ErrorMessageStartHzRequired)]
        [JsonProperty(Constants.PropertyNameHz)]
        public double Hz { get; set; }

        /// <summary>
        /// Gets or sets the MaxPowerDB.
        /// </summary>
        /// <value>The DBM.</value>
        [Required(ErrorMessage = Constants.ErrorMessageStartHzRequired)]
        [JsonProperty(Constants.PropertyNameDBm)]
        public double DBm { get; set; }

        /// <summary>
        /// Gets or sets the ChannelId.
        /// </summary>
        /// <value>The channel identifier.</value>
        [JsonProperty(Constants.PropertyNameChannelId)]
        public string ChannelId { get; set; }
    }
}
