// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Xml.Serialization;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a paws VCard.
    /// </summary>
    [JsonConverter(typeof(VcardConverter))]
    [Serializable]
    public class Vcard
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Vcard"/> class.
        /// </summary>
        public Vcard()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }

        /// <summary>
        /// Gets or sets the paws virtual card full name.
        /// </summary>
        [JsonProperty(Constants.PropertyNameFullName)]
        public string FullName { get; set; }

        /// <summary>
        /// Gets or sets the paws virtual card Organization.
        /// </summary>
        [ObjectValidator]
        [JsonProperty(Constants.PropertyNameOrganization)]
        public Organization Organization { get; set; }

        /// <summary>
        /// Gets or sets the paws virtual card address.
        /// </summary>
        [ObjectValidator]
        [JsonProperty(Constants.PropertyNameAddress)]
        public Address Address { get; set; }

        /// <summary>
        /// Gets or sets the paws virtual card phone.
        /// </summary>
        [ObjectValidator]
        [JsonProperty(Constants.PropertyNamePhone)]
        public Telephone Phone { get; set; }

        /// <summary>
        /// Gets or sets the paws virtual card email.
        /// </summary>
        [ObjectValidator]
        [JsonProperty(Constants.PropertyNameEmail)]
        public Email Email { get; set; }

        /// <summary>
        /// Gets or sets the paws virtual card unknown parameter types.
        /// </summary>
        [XmlIgnore]
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }
    }
}
