// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.EmailNotificationSystem
{
    using System;
    using Microsoft.WhiteSpaces.AdminNotificationSystem;

    public abstract class EmailBodyTemplate
    {
        public EmailBodyTemplate()
        {
        }

        public EmailBodyTemplate(string signature)
        {
            if (string.IsNullOrWhiteSpace(signature))
            {
                throw new ArgumentException("signature");
            }

            this.Signature = signature;
        }

        public string Signature
        {
            get;
            private set;
        }

        public abstract string GetHtmlBody(Notification notification);
    }
}
