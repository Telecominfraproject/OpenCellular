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
    internal class AuditTableEntity : TableEntity
    {
        /// <summary>
        /// private variable to have Audit Status
        /// </summary>
        private int status;

        /// <summary>
        /// private variable to have value of Audit ID
        /// </summary>
        private int auditId;
        
        /// <summary>
        /// private variable to have user id
        /// </summary>
        private string userID;

        /// <summary>
        /// private variable to have value of message
        /// </summary>
        private string message;

        /// <summary>
        /// private variable to have value of transaction id
        /// </summary>
        private string transactionID;

        /// <summary>
        /// private variable to have the value of elapsed time
        /// </summary>
        private long elapsedTime;

        /// <summary>
        /// Gets or sets the Log Level
        /// </summary>
        public int AuditId
        {
            get { return this.auditId; }
            set { this.auditId = value; }
        }

        /// <summary>
        /// Gets or sets the Log ID
        /// </summary>
        public int Status
        {
            get { return this.status; }
            set { this.status = value; }
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
        public long ElapsedTime
        {
            get { return this.elapsedTime; }
            set { this.elapsedTime = value; }
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
