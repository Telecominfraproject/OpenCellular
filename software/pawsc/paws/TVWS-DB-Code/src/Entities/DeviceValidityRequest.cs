// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System.Collections.Generic;
    using System.Xml.Serialization;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a paws device validity request.
    /// </summary>
    public class DeviceValidityRequest : DeviceValidityRequestBase
    {
        /// <summary>
        /// Initializes a new instance of the DeviceValidityRequest class.
        /// </summary>
        public DeviceValidityRequest()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }       
        
        /// <summary>
        /// Gets or sets the unknown types of a validity request.
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
