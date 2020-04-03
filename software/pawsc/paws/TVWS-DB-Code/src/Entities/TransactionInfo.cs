// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Contains information DB Sync Transaction
    /// </summary>
    public class TransactionInfo
    {
        /// <summary>
        /// Gets or sets the whitespace DB admin name (i.e. SPBR, TELC, MSFT, ...).
        /// </summary>
        public string AdminName { get; set; }

        /// <summary>
        /// Gets or sets the last Transaction Id for the specified admin.
        /// </summary>
        public string TransactionId { get; set; }

        /// <summary>
        /// Gets or sets the creation time of the TransactionId (transactionId are typically only valid for a maximum of 72 hours).
        /// </summary>
        public DateTime CreationTime { get; set; }
    }
}
