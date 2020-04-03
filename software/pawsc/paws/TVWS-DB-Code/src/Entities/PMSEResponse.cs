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
    /// Represents a Paws Response.
    /// </summary>
    public class PMSEResponse
    {
        /// <summary>
        /// Gets or sets the paws response.
        /// </summary>
        [JsonProperty(Constants.PropertyNameResult)]
        public Result Result { get; set; }

        /// <summary>
        /// Gets or sets the paws response.
        /// </summary>
        [JsonProperty(Constants.PropertyNameError)]
        public Result Error { get; set; }
    }
}
