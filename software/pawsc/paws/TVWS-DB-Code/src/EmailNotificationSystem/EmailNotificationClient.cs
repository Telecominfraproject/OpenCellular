// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.EmailNotificationSystem
{
    using System;
    using System.Linq;
    using System.Net.Mail;
    using Microsoft.WhiteSpaces.AdminNotificationSystem;

    public class EmailNotificationClient : INotificationClient
    {
        private readonly Email email;
        private readonly MailMessage mailMessage;

        public EmailNotificationClient(Email email, int mailPriority)
        {
            if (email == null)
            {
                throw new ArgumentNullException("email");
            }

            this.email = email;
            this.mailMessage = new MailMessage();
            this.mailMessage.Priority = (MailPriority)mailPriority;

            this.InitializeMailMessage();
        }

        public EmailNotificationClient(Email email, int mailPriority, bool isBodyHtml)
            : this(email, mailPriority)
        {
            this.mailMessage.IsBodyHtml = isBodyHtml;
        }

        public bool NotifyClient(Notification notification)
        {
            bool mailNotificatonStatus = false;

            using (SmtpClient smtpClient = new SmtpClient())
            {
                this.mailMessage.Body = this.email.BodyTemplate.GetHtmlBody(notification);

                smtpClient.Send(this.mailMessage);

                mailNotificatonStatus = true;
            }

            return mailNotificatonStatus;
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (this.mailMessage != null)
                {
                    this.mailMessage.Dispose();
                }
            }
        }

        private void InitializeMailMessage()
        {
            this.mailMessage.From = new MailAddress(this.email.From);
            this.email.To.ToList().ForEach((toAlias) => this.mailMessage.To.Add(new MailAddress(toAlias)));

            if (this.email.CC != null && this.email.CC.Any())
            {
                this.email.CC.ToList().ForEach((ccAlias) => this.mailMessage.CC.Add(new MailAddress(ccAlias)));
            }

            if (this.email.Bcc != null && this.email.Bcc.Any())
            {
                this.email.Bcc.ToList().ForEach((bccAlias) => this.mailMessage.Bcc.Add(new MailAddress(bccAlias)));
            }

            this.mailMessage.Subject = this.email.Subject;
        }        
    }
}
