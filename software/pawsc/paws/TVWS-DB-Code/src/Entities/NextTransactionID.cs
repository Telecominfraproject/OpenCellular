// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Enumeration for DB Sync Scope
    /// </summary>
    public enum DbSyncScope
    {
        /// <summary>
        /// Scope All Registrations
        /// </summary>
        ALL,

        /// <summary>
        ///  Scope Incremental Registrations
        /// </summary>
        INC
    }

    /// <summary>
    /// Represents a Next Transaction ID generated
    /// </summary>
    public class NextTransactionID : TableEntity
    {
        /// <summary>
        /// Gets or sets WSDBA name
        /// </summary>
        public string WSDBA { get; set; }

        /// <summary>
        /// Gets or sets Next Transaction ID
        /// </summary>
        public string NextTransactionId { get; set; }

        /// <summary>
        /// Gets or sets Next Transaction ID's date time
        /// </summary>
        public DateTime NextTransactionIdDateTime { get; set; }
    }
}
