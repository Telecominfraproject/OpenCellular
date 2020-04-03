// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Practices.Unity;

    /// <summary>
    /// Default Auditor that writes to an Azure table.
    /// </summary>
    public class AzureAuditor : IAuditor
    {
        /// <summary>
        /// private variable to have instance of IDALCAuditor
        /// </summary>
        private IDalcAuditor dalc;

        /// <summary>
        /// private variable to have region code
        /// </summary>
        private int regionCode;

        /// <summary>
        /// private variable to have user id
        /// </summary>
        private string userId;

        /// <summary>
        /// private variable to have transaction ID
        /// </summary>
        private Guid transactionId;

        /// <summary>
        /// Gets or sets the region identifier code.
        /// </summary>
        public int RegionCode 
        {
            get { return this.regionCode; }
            set { this.regionCode = value; }
        }

        /// <summary>
        /// Gets or sets the current user Id associated with the audit (if no user 
        /// associated with audit, then the value can be null).
        /// </summary>
        public string UserId
        { 
            get { return this.userId; }
            set { this.userId = value; }
        }

        /// <summary>
        /// Gets or sets the transaction id associated with the audit.
        /// </summary>
        public Guid TransactionId
        {
            get { return this.transactionId; }
            set { this.transactionId = value; }
        }

        /// <summary>
        /// Gets or sets the DALC Instance.
        /// </summary>
        [Dependency]
        public IDalcAuditor DalcAuditor
        {
            get { return this.dalc; }
            set { this.dalc = value; }
        }

        /// <summary>
        /// Gets or sets ILogger Interface
        /// </summary>
        [Dependency]
        public ILogger Logger { get; set; }

        /// <summary>
        /// Saves the specified audit message.
        /// </summary>
        /// <param name="id">Type of audit being logged.</param>
        /// <param name="status">Indicates if the audit was a success or failure.</param>
        /// <param name="elapsedTime">Amount of time in milliseconds to complete the operation.</param>
        /// <param name="message">Message associated with the audit.</param>
        public void Audit(AuditId id, AuditStatus status, long elapsedTime, string message)
        {
            try
            {
                this.dalc.WriteMessage(this.RegionCode, id, status, message, this.UserId, this.TransactionId, elapsedTime);
            }
            catch (Exception e)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
            }
        }
    }
}
