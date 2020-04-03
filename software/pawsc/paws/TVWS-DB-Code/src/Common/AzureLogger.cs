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
    using Microsoft.Practices.EnterpriseLibrary.Common.Configuration;
    using Microsoft.Practices.EnterpriseLibrary.Logging;
    using Microsoft.Whitespace.Common;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;
    using WindowsAzure.ServiceRuntime;

    /// <summary>
    /// Writes the specified message to the log file (uses Logging Application Block).
    /// </summary>
    public class AzureLogger : ILogger
    {
        /// <summary>The cloud table</summary>
        private CloudTable cloudTable;

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureLogger"/> class.
        /// </summary>
        public AzureLogger()
        {
            this.TransactionId = System.Guid.NewGuid();
            ////this.RegionCode = Utils.CurrentRegionId;
            ////this.UserId = Utils.UserId;
            if (RoleEnvironment.IsAvailable)
            {
                this.cloudTable = this.GetCloudStorageAccount().CreateCloudTableClient().GetTableReference("Log" + RoleEnvironment.CurrentRoleInstance.Role.Name + "v" + DateTime.Now.ToString("MMddhh"));
            }
            else
            {
                this.cloudTable = this.GetCloudStorageAccount().CreateCloudTableClient().GetTableReference("LogRegionSync" + DateTime.Now.ToString("MMddhh"));
            }

            this.cloudTable.CreateIfNotExists();
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
            DynamicTableEntity logEntity = new DynamicTableEntity()
                                       {
                                           PartitionKey = this.TransactionId.ToString(),
                                           RowKey = Guid.NewGuid().ToString(),
                                       };

            logEntity.Properties.Add("TransactionId", EntityProperty.GeneratePropertyForString(this.TransactionId.ToString()));
            logEntity.Properties.Add("LogMessage", EntityProperty.GeneratePropertyForString(message));
            logEntity.Properties.Add("Severity", EntityProperty.GeneratePropertyForString(severity.ToString()));
            logEntity.Properties.Add("MessageId", EntityProperty.GeneratePropertyForString(messageId.ToString()));

            this.cloudTable.Execute(TableOperation.Insert(logEntity));
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
        }

        /// <summary>Return Cloud Storage Account</summary>
        /// <returns>Cloud Storage Account</returns>
        private CloudStorageAccount GetCloudStorageAccount()
        {
            //// creates cloud storage account by reading connection string from app settings
            return CloudStorageAccount.Parse(Utils.Configuration.DbConnectionString);
        }
    }
}
