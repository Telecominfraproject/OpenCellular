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
    /// Base class for Paws request/responses.
    /// </summary>
    public abstract class BasePawsClass
    {
        /// <summary>
        /// Gets or sets the JsonRPC.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameJsonRpc)]
        public string JsonRpc { get; set; }

        /// <summary>
        /// Gets or sets the Id of the request/response.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameId)]
        public string Id { get; set; }
    }
}
