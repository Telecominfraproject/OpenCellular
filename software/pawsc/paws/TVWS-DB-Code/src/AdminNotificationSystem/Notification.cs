// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AdminNotificationSystem
{
    using System;

    public abstract class Notification
    {
        public Notification(DateTime validFrom, DateTime validTo)
        {
            if (validFrom > validTo)
            {
                throw new ArgumentOutOfRangeException("validFrom, validTo");
            }

            this.ValidFrom = validFrom;
            this.ValidTo = validTo;
        }

        public abstract string NotificationType { get; }

        public DateTime ValidFrom { get; private set; }

        public DateTime ValidTo { get; private set; }
    }
}
