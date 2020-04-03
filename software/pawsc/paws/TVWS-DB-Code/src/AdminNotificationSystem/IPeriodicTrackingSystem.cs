// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AdminNotificationSystem
{
    using System;

    public interface IPeriodicTrackingSystem : IDisposable
    {
        void StartTracking();

        void StopTracking();
    }
}
