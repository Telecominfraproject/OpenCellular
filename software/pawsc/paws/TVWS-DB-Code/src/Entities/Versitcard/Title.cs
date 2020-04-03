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
    /// Represents a owner's VCard Title.
    /// </summary>
    [XmlRoot("title", Namespace = Constants.VCardXmlns)]
    public class Title
    {
        /// <summary>
        /// Gets or sets the Alternate Id of the Title.
        /// </summary>
        public string AlternateId { get; set; }

        /// <summary>
        /// Gets or sets the PId of the Title.
        /// </summary>
        public string PId { get; set; }

        /// <summary>
        /// Gets or sets the Language of the Title.
        /// </summary>
        public string Language { get; set; }

        /// <summary>
        /// Gets or sets the Text of the Title.
        /// </summary>
        [XmlElement("text")]
        public string Text { get; set; }
    }
}
