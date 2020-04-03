// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;

    /// <summary>
    /// Represents Class CDBSIFSTADATA.
    /// </summary>
    public class CDBSIfStaData
    {
        /// <summary>
        /// Gets or sets the Application Id.
        /// </summary>
        /// <value>The Application Id.</value>
        public int ApplicationId { get; set; }

        /// <summary>
        /// Gets or sets the LIC ANT SYS CHANGE
        /// </summary>
        /// <value>The LIC ANT SYS CHANGE.</value>
        public string LicAntSysChange { get; set; }

        /// <summary>
        /// Gets or sets the Antenna System
        /// </summary>
        /// <value>The Antenna System.</value>
        public string AntennaSystem { get; set; }

        /// <summary>
        /// Gets or sets the Application ARN
        /// </summary>
        /// <value>The Application ARN.</value>
        public string RefAppArn { get; set; }

        /// <summary>
        /// Gets or sets the File Prefix
        /// </summary>
        /// <value>The File Prefix.</value>
        public string RefFilePrefix { get; set; }

        /// <summary>
        /// Gets or sets the SYS File Prefix
        /// </summary>
        /// <value>The  SYS File Prefix.</value>
        public string CpAntSysFilePrefix { get; set; }

        /// <summary>
        /// Gets or sets the ANT SYS ARN
        /// </summary>
        /// <value>The ANT SYS ARN.</value>
        public string CpAntSysAppArn { get; set; }

        /// <summary>
        /// Gets or sets the Silent since date
        /// </summary>
        /// <value>The  Silent since date</value>
        public DateTime SilentSinceDate { get; set; }

        /// <summary>
        /// Gets or sets the return_to_air_date.
        /// </summary>
        /// <value>The return_to_air_date.</value>
        public DateTime ReturnToAirDate { get; set; }
    }
}
