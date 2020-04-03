// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Xml.Serialization;

    /// <summary>
    /// Represents a Quadrilateral Area of a registration.
    /// </summary>
    [XmlInclude(typeof(QuadrilateralArea))]
    [XmlRoot("lpauxQuadrilateralArea", Namespace = Constants.VCardXmlns)]

    public class QuadrilateralArea
    {
        /// <summary>
        /// Gets or sets the North-East point of the Quadrilateral Area.
        /// </summary>
        [XmlElement("NE_Point")]
        public Position NEPoint { get; set; }

        /// <summary>
        /// Gets or sets the South-East point of the Quadrilateral Area.
        /// </summary>
        [XmlElement("SE_Point")]
        public Position SEPoint { get; set; }

        /// <summary>
        /// Gets or sets the South-West point of the Quadrilateral Area.
        /// </summary>
        [XmlElement("SW_Point")]
        public Position SWPoint { get; set; }

        /// <summary>
        /// Gets or sets the North-West point of the Quadrilateral Area.
        /// </summary>
        [XmlElement("NE_Point")]
        public Position NWPoint { get; set; }
    }
}
