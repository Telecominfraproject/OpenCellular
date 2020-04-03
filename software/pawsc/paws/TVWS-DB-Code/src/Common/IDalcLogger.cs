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
    public interface IDalcLogger
    {
   /// <summary>
        /// Writes the specified message to an audit table.
        /// </summary>
        /// <param name="regionCode">The region id.</param>
        /// <param name="logLevel">The type of Log Level set in the config.</param>
        /// <param name="logMsgId">Log message Id</param>
        /// <param name="logMessage">The message associated with the log.</param>
        /// <param name="userId">The user Id who initiated the audit.</param>
        /// <param name="transactionId">A unique Id of the audit operation.</param>
        void LogMessage(int regionCode, LoggingLevel logLevel, LoggingMessageId logMsgId, string logMessage, string userId, Guid transactionId);
    }
}
