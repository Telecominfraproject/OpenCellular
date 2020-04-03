// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System.Collections.Generic;
    using System.Xml.Serialization;
    using Newtonsoft.Json;

    /// <summary>
    /// Contains the paws parameters for a register request.
    /// </summary>
    public class RegisterRequest : RegisterRequestBase
    {
        /// <summary>
        /// Initializes a new instance of the RegisterRequest class.
        /// </summary>
        public RegisterRequest()
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
