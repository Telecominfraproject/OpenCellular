// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Dalc
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.WindowsAzure.Storage.Table;
    
    /// <summary>
    /// This entity is to hold the log information to be saved to azure tables and implements Entity Table of Azure Storage
    /// </summary>
    public class LogTableEntity : TableEntity
    {
        /// <summary>
        /// Holds Log Level
        /// </summary>
        private int logLevel;

        /// <summary>
        /// Holds Log Id
        /// </summary>
        private int logId;

        /// <summary>
        /// Holds User Name
        /// </summary>
        private string userID;

        /// <summary>
        /// Holds Log Message
        /// </summary>
        private string message;

        /// <summary>
        /// Holds transaction Id
        /// </summary>
        private string transactionID;

        /// <summary>
        /// Holds Log Date and Time
        /// </summary>
        private DateTime logDateTime;

        /// <summary>
        /// Gets or sets the Log Level
        /// </summary>
        public int LogLevel
        {
            get { return this.logLevel; }
            set { this.logLevel = value; }
        }

        /// <summary>
        /// Gets or sets the Log ID
        /// </summary>
        public int Id
        {
            get { return this.logId; }
            set { this.logId = value; }
        }

        /// <summary>
        /// Gets or sets the User.
        /// </summary>
        public string UserID
        {
            get { return this.userID; }
            set { this.userID = value; }
        }

        /// <summary>
        /// Gets or sets the message.
        /// </summary>
        public string Message
        {
            get { return this.message; }
            set { this.message = value; }
        }

        /// <summary>
        /// Gets or sets the Log Date Time.
        /// </summary>
        public DateTime LogDateTime
        {
            get { return this.logDateTime; }
            set { this.logDateTime = value; }
        }

        /// <summary>
        /// Gets or sets the transaction id.
        /// </summary>
        public string TransactionID
        {
            get { return this.transactionID; }
            set { this.transactionID = value; }
        }
    }
}
