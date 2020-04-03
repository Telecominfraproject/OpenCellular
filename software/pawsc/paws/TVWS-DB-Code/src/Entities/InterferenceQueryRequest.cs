// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Xml.Serialization;
    using Microsoft.Whitespace.Entities.Versitcard;
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents Interference Query Request Details.
    /// </summary>
    [JsonConverter(typeof(InterferenceQueryConverter))]
    public class InterferenceQueryRequest : InterferenceQueryRequestBase
    {
        /// <summary>
        /// Initializes a new instance of the InterferenceQueryRequest class.
        /// </summary>
        public InterferenceQueryRequest()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }

        /// <summary>
        /// Gets or sets the unknown parameter types.
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
