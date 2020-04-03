// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AdminNotificationSystem
{
    using System;

    public class AuditLog
    {
        public AuditLog(int auditId, string message, string transactionId)
        {
            this.AuditId = auditId;
            this.Message = message;
            this.TransactionId = transactionId;
        }

        public int AuditId { get; private set; }

        public string Message { get; private set; }

        public string TransactionId { get; private set; }
    }
}
