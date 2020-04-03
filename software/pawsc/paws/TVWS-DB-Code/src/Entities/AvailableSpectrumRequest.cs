// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Xml.Serialization;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents an paws available spectrum request.
    /// </summary>
    [JsonConverter(typeof(AvailableSpectrumConverter))]
    public class AvailableSpectrumRequest : AvailableSpectrumRequestBase
    {
        /// <summary>
        /// Initializes a new instance of the AvailableSpectrumRequest class.
        /// </summary>
        public AvailableSpectrumRequest()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }       
        
        /// <summary>
        /// Gets or sets the UnknownTypes of the paws request.
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
