// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Linq;
    using System.Text;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents the paws device capabilities.
    /// </summary>
    public class DeviceCapabilities
    {
        /// <summary>
        /// Gets or sets the frequency ranges of the paws device. 
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameFrequencyRanges)]
        public FrequencyRange[] FrequencyRanges { get; set; }
    }
}
