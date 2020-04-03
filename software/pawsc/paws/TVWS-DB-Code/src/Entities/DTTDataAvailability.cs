// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.IO.Compression;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Class DTTDataAvailability.
    /// </summary>
    public class DTTDataAvailability : TableEntity
    {
        /// <summary>
        /// Gets or sets the easting.
        /// </summary>
        /// <value>The easting.</value>
        public int Easting { get; set; }

        /// <summary>
        /// Gets or sets the northing.
        /// </summary>
        /// <value>The northing.</value>
        public int Northing { get; set; }

        /// <summary>
        /// Gets or sets the data record.
        /// </summary>
        /// <value>The data record.</value>
        public byte[] DataRecord { get; set; }

        /// <summary>
        /// Gets or sets the unscheduled data.
        /// </summary>
        /// <value>The unscheduled data.</value>
        public int[] UnscheduledData { get; set; }
    }
}
