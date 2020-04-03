// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents Class GeoSpectrumSpec.
    /// </summary>
    public class GeoSpectrumSpec
    {
        /// <summary>
        /// Gets or sets the location.
        /// </summary>
        /// <value>The location.</value>
        [JsonProperty(Constants.PropertyNameLocation)]
        public GeoLocation Location { get; set; }

        /// <summary>
        /// Gets or sets the spectrum specs.
        /// </summary>
        /// <value>The spectrum specs.</value>
        [JsonProperty(Constants.PropertyNameSpectrumSpecs)]
        public SpectrumSpec[] SpectrumSpecs { get; set; }
    }
}
