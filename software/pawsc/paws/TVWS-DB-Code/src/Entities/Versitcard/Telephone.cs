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
    /// Gets or sets the VCard owner's telephone information.
    /// </summary>
    [XmlRoot("tel", Namespace = Constants.VCardXmlns)]
    public class Telephone : TableEntity
    {
        /// <summary>
        /// Gets or sets the Alternate Id of the telephone.
        /// </summary>
        public string AlternateId { get; set; }

        /// <summary>
        /// Gets or sets the PId of the telephone.
        /// </summary>
        public string PId { get; set; }

        /// <summary>
        /// Gets or sets the Preference of the telephone.
        /// </summary>
        public int Pref { get; set; }

        /// <summary>
        /// Gets or sets the Type of the telephone.
        /// </summary>
        public string[] Type { get; set; }

        /// <summary>
        /// Gets or sets the MediaType of the telephone.
        /// </summary>
        public string MediaType { get; set; }

        /// <summary>
        /// Gets or sets the Uri of the telephone.
        /// </summary>
        [XmlElement("text", Namespace = Constants.VCardXmlns)]
        public string TelephoneNumber { get; set; }
    }
}
