// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.EmailNotificationSystem
{
    using System;
    using System.IO;
    using System.Linq;
    using System.Text;
    using Microsoft.WhiteSpaces.AdminNotificationSystem;
    using ALMBT = Microsoft.WhiteSpaces.EmailNotificationSystem.AuditLogMailBodyTemplate;

    public class AuditLogMailBodyTemplate : EmailBodyTemplate
    {
        private const string ResourceManifest = "Microsoft.WhiteSpaces.EmailNotificationSystem";
        private const string FileName = "MailBodyTemplates.AuditLogEmailBody.html";

        private int logsCount;

        public AuditLogMailBodyTemplate(int auditLogsCount = 50)
        {
            this.logsCount = auditLogsCount;
        }

        public override string GetHtmlBody(Notification notification)
        {
            string emailBody = string.Empty;
            string resourceManifestPath = string.Format("{0}.{1}", ALMBT.ResourceManifest, ALMBT.FileName);

            using (Stream resourceStream = EmbeddedResourceReader.GetResourceStream(resourceManifestPath))
            {
                StreamReader reader = new StreamReader(resourceStream);
                emailBody = reader.ReadToEnd();

                emailBody = this.UpdateEmailBodyPlaceHolder(emailBody, notification);
            }

            return emailBody;
        }

        private string UpdateEmailBodyPlaceHolder(string htmlBody, Notification notification)
        {
            AuditLogData auditNotification = (AuditLogData)notification;

            StringBuilder auditLogEntries = new StringBuilder();

            for (int i = 0; i < Math.Min(this.logsCount, auditNotification.AuditLogs.Count()); i++)
            {
                auditLogEntries.Append("<tr>");
                auditLogEntries.Append(string.Format("<td>{0}</td>", auditNotification.AuditLogs.ElementAt(i).AuditId));
                auditLogEntries.Append(string.Format("<td>{0}</td>", auditNotification.AuditLogs.ElementAt(i).Message));
                auditLogEntries.Append(string.Format("<td>{0}</td>", auditNotification.AuditLogs.ElementAt(i).TransactionId));
                auditLogEntries.Append("</tr>");
            }

            htmlBody = htmlBody.Replace("{ScanTime}", auditNotification.ValidTo.ToString());
            htmlBody = htmlBody.Replace("{LogEntries}", auditLogEntries.ToString());

            return htmlBody;
        }
    }
}
