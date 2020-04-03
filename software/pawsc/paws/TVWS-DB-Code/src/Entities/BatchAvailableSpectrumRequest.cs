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
    /// Represents a paws batch available spectrum request.
    /// </summary>
    [JsonConverter(typeof(BatchAvailableSpectrumConverter))]
    public class BatchAvailableSpectrumRequest : BatchAvailableSpectrumRequestBase
    {
        /// <summary>
        /// Initializes a new instance of the BatchAvailableSpectrumRequest class.
        /// </summary>
        public BatchAvailableSpectrumRequest()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }

        /// <summary>
        /// Gets or sets the UnknownTypes of the BatchAvailableSpectrumRequest.
        /// </summary>
        [XmlIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
