// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AdminNotificationSystem
{
    using System;
    using Microsoft.WhiteSpaces.AdminNotificationSystem;

    public interface INotificationClient : IDisposable
    {
        bool NotifyClient(Notification notification);
    }
}
