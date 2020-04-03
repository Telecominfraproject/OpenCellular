// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AdminNotificationSystem
{
    using System;
    using System.Collections.Generic;

    public class AuditLogData : Notification
    {
        private string notificationType;

        public AuditLogData(IEnumerable<AuditLog> auditLogs, string notificationType, DateTime validFrom, DateTime validTo)
            : base(validFrom, validTo)
        {
            this.AuditLogs = auditLogs;
            this.notificationType = notificationType;
        }

        public IEnumerable<AuditLog> AuditLogs
        {
            get;
            private set;
        }

        public override string NotificationType
        {
            get
            {
                return this.notificationType;
            }
        }
    }
}
