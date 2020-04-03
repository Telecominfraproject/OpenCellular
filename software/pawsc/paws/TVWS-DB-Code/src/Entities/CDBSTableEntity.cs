// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>Represents Class CDBSTableEntity.</summary>
    public class CDBSTableEntity : TableEntity
    {
        /// <summary>Gets or sets a value indicating whether this instance is new.</summary>
        /// <value><c>true</c> if this instance is new; otherwise, <c>false</c>.</value>
        public bool IsNew { get; set; }

        /// <summary>Gets or sets the ANTENNA_ID</summary>
        /// <value>The ANTENNA_ID.</value>
        public int AntennaId { get; set; }

        /// <summary>Gets or sets the APPLICATION_ID</summary>
        /// <value>The APPLICATION_ID.</value>
        public int ApplicationId { get; set; }

        /// <summary>Gets or sets the FACILITY_ID</summary>
        /// <value>The FACILITY_ID.</value>
        public int FacilityId { get; set; }

        /// <summary>Gets or sets the call sign.</summary>
        /// <value>The call sign.</value>
        public string CallSign { get; set; }
    }
}
