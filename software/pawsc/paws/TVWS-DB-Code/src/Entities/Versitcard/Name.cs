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
    /// Represents a VCard name instance.
    /// </summary>
    [XmlRoot("name", Namespace = Constants.VCardXmlns)]
    public class Name : TableEntity
    {
        /// <summary>
        /// Gets or sets the language of the name.
        /// </summary>
        public string Language { get; set; }

        /// <summary>
        /// Gets or sets the SortAs of the name.
        /// </summary>
        public string SortAs { get; set; }

        /// <summary>
        /// Gets or sets the AlternateId of the name.
        /// </summary>
        public string AlternateId { get; set; }

        /// <summary>
        /// Gets or sets the Surname of the name.
        /// </summary>
        public string Surname { get; set; }

        /// <summary>
        /// Gets or sets the Given of the name.
        /// </summary>
        [XmlElement("text")]
        public string ContactName { get; set; }

        /// <summary>
        /// Gets or sets the Additional of the name.
        /// </summary>
        public string Additional { get; set; }

        /// <summary>
        /// Gets or sets the Prefix of the name.
        /// </summary>
        public string Prefix { get; set; }

        /// <summary>
        /// Gets or sets the Suffix of the name.
        /// </summary>
        public string Suffix { get; set; }
    }
}
