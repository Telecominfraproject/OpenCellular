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
    /// vCard represents personal contact information.  The vCard 
    /// documentation can be found at: http://tools.ietf.org/html/draft-ietf-vcarddav-vcardxml-10
    /// </summary>
    [XmlRoot("properties", Namespace = Constants.VCardXmlns)]
    public class VCard : TableEntity
    {
        /// <summary>
        /// Gets or sets the type of vCard: individual, group, org, location.
        /// </summary>
        [XmlElement("kind")]
        public string Kind { get; set; }

        /// <summary>
        /// Gets or sets the contact's formatted name.
        /// </summary>
        [XmlElement("fn")]
        public FormattedName FN { get; set; }

        /// <summary>
        /// Gets or sets the contact's name.
        /// </summary>
        [XmlElement("name")]
        public Name Name { get; set; }

        /// <summary>
        /// Gets or sets the contact's address.
        /// </summary>
        public Address Address { get; set; }

        /// <summary>
        /// Gets or sets the contact's telephone numbers.
        /// </summary>
        [XmlArrayItem("tel", Namespace = Constants.VCardXmlns, Type = typeof(Telephone))]
        [XmlArray("properties", Namespace = Constants.VCardXmlns)]
        public Telephone[] Telephone { get; set; }

        /// <summary>
        /// Gets or sets the contact's email addresses.
        /// </summary>
        [XmlArrayItem("email", Namespace = Constants.VCardXmlns, Type = typeof(Email))]
        [XmlArray]
        public Email[] Email { get; set; }

        /// <summary>
        /// Gets or sets the contact's time zone.
        /// </summary>
        [XmlElement("tz")]
        public string TimeZone { get; set; }

        /// <summary>
        /// Gets or sets the contact's title.
        /// </summary>
        [XmlElement("title")]
        public Title Title { get; set; }

        /// <summary>
        /// Gets or sets the contact's organization.
        /// </summary>
        [XmlElement("org")]
        public Organization Org { get; set; }
    }
}
