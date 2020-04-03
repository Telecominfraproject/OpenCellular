// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.EmailNotificationSystem
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Net.Mail;

    public class Email
    {
        public Email(string from, string[] to, string[] cc, string[] bcc, string subject, EmailBodyTemplate bodyTemplate)
        {
            if (string.IsNullOrWhiteSpace(from))
            {
                throw new ArgumentException("from");
            }

            Email.ValidateMailAddress(from, "from");

            if (to == null)
            {
                throw new ArgumentNullException("to");
            }

            if (!to.Any())
            {
                throw new ArgumentException("List can't be empty", "to");
            }

            Email.ValidateMailAddressCollection(to.ToList(), "to");

            if (cc != null && cc.Any())
            {
                Email.ValidateMailAddressCollection(cc.ToList(), "cc");
            }

            if (bcc != null && cc.Any())
            {
                Email.ValidateMailAddressCollection(bcc.ToList(), "bcc");
            }

            this.From = from;
            this.To = to;
            this.CC = cc;
            this.Bcc = bcc;
            this.Subject = subject;
            this.BodyTemplate = bodyTemplate;
        }

        public string From
        {
            get;
            private set;
        }

        public string[] To
        {
            get;
            private set;
        }

        public string[] CC
        {
            get;
            private set;
        }

        public string[] Bcc
        {
            get;
            private set;
        }

        public string Subject
        {
            get;
            private set;
        }

        public EmailBodyTemplate BodyTemplate
        {
            get;
            private set;
        }

        private static void ValidateMailAddress(string address)
        {
            try
            {
                MailAddress mailAddress = new MailAddress(address);
            }
            catch (Exception)
            {
                throw;
            }
        }

        private static void ValidateMailAddress(string address, string paramName)
        {
            try
            {
                MailAddress mailAddress = new MailAddress(address);
            }
            catch (FormatException ex)
            {
                throw new InvalidArgumentException(ex.Message, paramName, ex.InnerException);
            }
        }

        private static void ValidateMailAddressCollection(List<string> addressCollection, string paramName)
        {
            try
            {
                foreach (string alias in addressCollection)
                {
                    ValidateMailAddress(alias);
                }
            }
            catch (Exception ex)
            {
                throw new InvalidArgumentException(ex.Message, paramName, ex.InnerException);
            }
        }
    }
}
