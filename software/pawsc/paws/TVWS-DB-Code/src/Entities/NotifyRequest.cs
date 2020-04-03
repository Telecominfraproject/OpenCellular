// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Xml.Serialization;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents the paws notify request parameters.
    /// </summary>
    [JsonConverter(typeof(NotifyConverter))]
    public class NotifyRequest : NotifyRequestBase
    {
        /// <summary>
        /// Initializes a new instance of the NotifyRequest class.
        /// </summary>
        public NotifyRequest()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }        

        /// <summary>
        /// Gets or sets the unknown types of the notify request.
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
