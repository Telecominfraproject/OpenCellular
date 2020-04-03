// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities.Versitcard
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Xml.Serialization;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents a VCard Address.
    /// </summary>
    [XmlInclude(typeof(Address))]
    [XmlRoot("adr", Namespace = Constants.VCardXmlns)]
    public class Address : TableEntity
    {
        /// <summary>
        /// Gets or sets the PO Box of the address.
        /// </summary>
        [XmlElement("pobox")]
        public string POBox { get; set; }

        /// <summary>
        /// Gets or sets the Ext of the address.
        /// </summary>
        [XmlElement("ext")]
        public string Ext { get; set; }

        /// <summary>
        /// Gets or sets the Street of the address.
        /// </summary>
        [XmlElement("street")]
        public string Street { get; set; }

        /// <summary>
        /// Gets or sets the Locality of the address.
        /// </summary>
        [XmlElement("locality")]
        public string Locality { get; set; }

        /// <summary>
        /// Gets or sets the Region of the address.
        /// </summary>
        [XmlElement("region")]
        public string Region { get; set; }

        /// <summary>
        /// Gets or sets the Zip Code of the address.
        /// </summary>
        [XmlElement("code")]
        public string Code { get; set; }

        /// <summary>
        /// Gets or sets the Country of the address.
        /// </summary>
        [XmlElement("country")]
        public string Country { get; set; }
    }
}
