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

    /// <summary>
    /// Represents a VCard formatted name instance.
    /// </summary>
    [XmlRoot("fn", Namespace = Constants.VCardXmlns)]
    public class FormattedName
    {
        /// <summary>
        /// Gets or sets the language of the name.
        /// </summary>
        public string Language { get; set; }

        /// <summary>
        /// Gets or sets the PId of the name.
        /// </summary>
        public string PId { get; set; }

        /// <summary>
        /// Gets or sets the Alternate Id of the name.
        /// </summary>
        public string AlternateId { get; set; }

        /// <summary>
        /// Gets or sets the Type of the name.
        /// </summary>
        public string Type { get; set; }

        /// <summary>
        /// Gets or sets the Preferred of the name.
        /// </summary>
        public int Pref { get; set; }

        /// <summary>
        /// Gets or sets the Preferred of the name.
        /// </summary>
        [XmlElement("text")]
        public string PrefText { get; set; }
    }
}
