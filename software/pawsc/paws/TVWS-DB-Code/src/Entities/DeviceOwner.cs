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
    /// Represents a paws device owner.
    /// </summary>
    [JsonConverter(typeof(DeviceOwnerConverter))]
    [Serializable]
    public class DeviceOwner
    {
        /// <summary>
        /// Initializes a new instance of the DeviceOwner class.
        /// </summary>
        public DeviceOwner()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }

        /// <summary>
        /// Gets or sets the device's VCard.
        /// </summary>
        [Required(ErrorMessage = Constants.ErrorMessageOwnerRequired)]
        [ObjectValidator]
        [JsonProperty(Constants.PropertyNameOwner)]
        public Vcard Owner { get; set; }

        /// <summary>
        /// Gets or sets the device's operator.
        /// </summary>
        [ObjectValidator]
        [JsonProperty(Constants.PropertyNameOperator)]
        public Vcard Operator { get; set; }

        /// <summary>
        /// Gets or sets the unknown types of the device owner information.
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
