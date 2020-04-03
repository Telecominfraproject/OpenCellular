// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System.Collections.Generic;
    using System.Xml.Serialization;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents an init request.
    /// </summary>
    public class InitRequest : InitRequestBase
    {
        /// <summary>
        /// Initializes a new instance of the InitRequest class.
        /// </summary>
        public InitRequest()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }       
        
        /// <summary>
        /// Gets or sets the unknown types of an init request.
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; } 
    }
}
