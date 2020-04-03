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
    /// Represents a VCard organization.
    /// </summary>
    [XmlInclude(typeof(Organization))]
    [XmlRoot("org", Namespace = Constants.VCardXmlns)]
    public class Organization : TableEntity
    {
        /// <summary>
        /// Gets or sets the Alternate Id of the organization.
        /// </summary>
        public string AlternateId { get; set; }

        /// <summary>
        /// Gets or sets the PId of the organization.
        /// </summary>
        public string PId { get; set; }

        /// <summary>
        /// Gets or sets the Language of the organization.
        /// </summary>
        public string Language { get; set; }

        /// <summary>
        /// Gets or sets the Preference of the organization.
        /// </summary>
        public int Pref { get; set; }

        /// <summary>
        /// Gets or sets the Type of the organization.
        /// </summary>
        public string[] Type { get; set; }

        /// <summary>
        /// Gets or sets the SortAs of the organization.
        /// </summary>
        public string SortAs { get; set; }

        /// <summary>
        /// Gets or sets the Name of the organization.
        /// </summary>
        [XmlElement("text")]
        public string OrganizationName { get; set; }
    }
}
