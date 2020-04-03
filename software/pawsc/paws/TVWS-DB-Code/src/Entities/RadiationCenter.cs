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
    using Microsoft.WindowsAzure.Storage.Table;
    
    /// <summary>
    /// Represents a antenna radiation center.
    /// </summary>
    [XmlRoot("locRadiationCenter", Namespace = Constants.RegistrationRecordEnsembleXmlns)]
    public class RadiationCenter : TableEntity
    {
        /// <summary>
        /// Gets or sets the radiation center Above Mean Sea Level
        /// </summary>
        [XmlElement("rcAMSL")]
        public double AMSL { get; set; }

        /// <summary>
        /// Gets or sets the Antenna Height Above Average Terrain (HAAT).
        /// </summary>
        [XmlElement("rcHAAT")]
        public double HAAT { get; set; }

        /// <summary>
        /// Gets or sets the Antenna Height above ground.
        /// </summary>
        [XmlElement("rcHAG")]
        public double HAG { get; set; }
    }
}
