// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Reflection;
    using System.Text;
    using Microsoft.Whitespace;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a Request class.
    /// </summary>
    [JsonConverter(typeof(RequestConverter))]
    public class Request : BasePawsClass
    {
        /// <summary>
        /// Initializes a new instance of the Request class.
        /// </summary>
        public Request()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }

        /// <summary>
        /// Gets or sets the Method.
        /// </summary>
        [JsonProperty(Constants.PropertyNameMethod)]
        public string Method { get; set; }

        /// <summary>
        /// Gets or sets the parameters.
        /// </summary>
        [JsonProperty(Constants.PropertyNameParams)]
        public Parameters Params { get; set; }

        /// <summary>
        /// Gets or sets the Unknown Types.
        /// </summary>
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
