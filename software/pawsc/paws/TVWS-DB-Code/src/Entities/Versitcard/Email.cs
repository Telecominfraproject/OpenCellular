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
    /// Represents a VCard email instance.
    /// </summary>
    [XmlRoot("email", Namespace = Constants.VCardXmlns)]
    public class Email : TableEntity
    {
        /// <summary>
        /// Gets or sets the alternate Id of the email.
        /// </summary>
        public string AlternateId { get; set; }

        /// <summary>
        /// Gets or sets the PId of the email.
        /// </summary>
        public string PId { get; set; }

        /// <summary>
        /// Gets or sets the Preferred of the email.
        /// </summary>
        public int Pref { get; set; }

        /// <summary>
        /// Gets or sets the Type of the email.
        /// </summary>
        public string[] Type { get; set; }

        /// <summary>
        /// Gets or sets the Address of the email.
        /// </summary>
        [XmlElement("text", Namespace = Constants.VCardXmlns)]
        public string EmailAddress { get; set; }
    }
}
