// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Diagnostics;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Practices.EnterpriseLibrary.Logging;
    using Microsoft.Practices.EnterpriseLibrary.Logging.Configuration;
    using Practices.EnterpriseLibrary.Common.Configuration;

    /// <summary>
    /// Writes the specified message to the log file (uses Logging Application Block).
    /// </summary>
    public class Logger : ILogger
    {
        /// <summary>
        /// Initializes a new instance of the Logger class.
        /// </summary>
        public Logger()
        {
            this.TransactionId = System.Guid.NewGuid();
            this.RegionCode = Utils.CurrentRegionId;
            this.UserId = Utils.UserId;
            var logWriterFactory = new LogWriterFactory((test) => Utils.GetLoggingSettings());
            Microsoft.Practices.EnterpriseLibrary.Logging.Logger.SetLogWriter(logWriterFactory.Create(), false);
        }

        /// <summary>
        /// Gets or sets the current region code.
        /// </summary>
        public int RegionCode { get; set; }

        /// <summary>
        /// Gets or sets the user Id associated with the log message (can be a web user, remote DB Admin, or a worker thread).
        /// </summary>
        public string UserId { get; set; }

        /// <summary>
        /// Gets or sets the transaction associated with the log message.
        /// </summary>
        public Guid TransactionId { get; set; }

        /// <summary>
        /// Writes the specified message to the log file.
        /// </summary>
        /// <param name="severity">The log message level.</param>
        /// <param name="messageId">The message id.</param>
        /// <param name="message">The message being logged.</param>
        public void Log(TraceEventType severity, LoggingMessageId messageId, string message)
        {
            LogEntry entry = new LogEntry()
            {
                Message = message,
                EventId = (int)messageId,
                Severity = severity,
                ActivityId = this.TransactionId,
                Title = this.UserId
            };
            entry.Categories.Add("General");
            entry.ExtendedProperties.Add("TransactionId", this.TransactionId);
            Microsoft.Practices.EnterpriseLibrary.Logging.Logger.Write(entry);
        }

        /// <summary>
        /// Logs the specified message if the current configured logging level is less than the passed in log level.
        /// </summary>
        /// <param name="level">The logging level of the new entry (used to determine if message is to be logged).</param>
        /// <param name="userName">The registered user who made the call.</param>
        /// <param name="msgId">The message Id</param>
        /// <param name="logMessage">The message that is to be logged.</param>
        /// <param name="transactionId">The TransactionId that is to be logged.</param>
        public void Log(LoggingLevel level, string userName, LoggingMessageId msgId, string logMessage, Guid transactionId)
        {
            throw new NotImplementedException();
        }
    }
}
