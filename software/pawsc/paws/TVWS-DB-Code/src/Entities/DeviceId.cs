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
    /// Represents a Paws Device Id.
    /// </summary>
    [XmlRoot("tvbdRegDeviceId", Namespace = Constants.RegistrationRecordEnsembleXmlns)]
    public class DeviceId : TableEntity
    {
        /// <summary>
        /// Gets or sets the series name.
        /// </summary>
        [XmlElement("didSeriesName")]
        public string SeriesName { get; set; }

        /// <summary>
        /// Gets or sets the series value.
        /// </summary>
        [XmlElement("didSeriesValue")]
        public string SeriesValue { get; set; }

        /// <summary>
        /// Gets or sets the serial number.
        /// </summary>
        [XmlElement("didSerialNumber")]
        public string SerialNumber { get; set; }
    }
}
