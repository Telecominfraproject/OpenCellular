// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Defines all of the required audit methods used by the data access layer component.
    /// </summary>
    public interface IDalcAuditor
    {
        /// <summary>
        /// Writes the specified message to an audit table.
        /// </summary>
        /// <param name="regionCode">The region id.</param>
        /// <param name="id">The type of audit operation that was just completed.</param>
        /// <param name="status">The status of the audit operation.</param>
        /// <param name="message">The message associated with the audit.</param>
        /// <param name="userId">The user Id who initiated the audit.</param>
        /// <param name="transactionId">A unique Id of the audit operation.</param>
        /// <param name="eleapsedTime">The total elapsed time in milliseconds for the audit operation to complete.</param>
        void WriteMessage(
            int regionCode,
            AuditId id,
            AuditStatus status,
            string message,
            string userId,
            Guid transactionId,
            long eleapsedTime);
    }
}
