// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AdminNotificationSystem
{
    using System;
    using System.Timers;
    using Microsoft.WhiteSpaces.AdminNotificationSystem;

    public abstract class NotificationSystem : IPeriodicTrackingSystem
    {
        protected const string DateTimeFormat = "yyyy-MM-d hh:mm:ss tt";

        protected readonly INotificationClient NotificationClient;
        protected readonly Timer NotificationTimer;
        protected readonly string NotificationSystemType;

        protected NotificationSystem(INotificationClient notificationClient, string notificationType)
        {
            if (notificationClient == null)
            {
                throw new ArgumentNullException("notificationClient");
            }

            if (string.IsNullOrWhiteSpace(notificationType))
            {
                throw new ArgumentException("notificationType");
            }

            this.NotificationClient = notificationClient;
            this.NotificationSystemType = notificationType;

            // To avoid an exception that could occur on calling Start/Stop method,
            // use default interval time and notify only once if the interval is not set.
            this.NotificationTimer = new Timer();
        }

        protected NotificationSystem(INotificationClient notificationClient, double notificationInterval, string notificationType)
            : this(notificationClient, notificationType)
        {
            if (notificationInterval == 0 || notificationInterval > int.MaxValue)
            {
                throw new ArgumentOutOfRangeException("notificationInterval");
            }

            this.NotificationTimer.Interval = notificationInterval;
            this.NotificationTimer.AutoReset = true;

            this.NotificationTimer.Elapsed += this.NotificationTimerElapsed;
        }

        public DateTime TrackingTimeStart
        {
            get;
            protected set;
        }

        public bool Enabled
        {
            get;
            protected set;
        }

        public virtual void StartTracking()
        {
            System.Diagnostics.Trace.TraceInformation(string.Format("PeriodicTrackingSystem has started tracking {0} | {1}", this.NotificationSystemType, DateTime.UtcNow.ToString(NotificationSystem.DateTimeFormat)));

            this.Enabled = true;
            this.NotificationTimer.Start();
            this.TrackingTimeStart = DateTime.UtcNow;
        }

        public virtual void StopTracking()
        {
            this.Enabled = false;
            this.NotificationTimer.Stop();

            System.Diagnostics.Trace.TraceInformation(string.Format("PeriodicTrackingSystem has stopped tracking {0} | {1}", this.NotificationSystemType, this.NotificationSystemType, DateTime.UtcNow.ToString(NotificationSystem.DateTimeFormat)));
        }

        public abstract void Notify(DateTime signalTime);

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (this.NotificationTimer != null)
                {
                    this.NotificationTimer.Dispose();
                }
            }
        }   

        private void NotificationTimerElapsed(object sender, ElapsedEventArgs e)
        {
            DateTime elapsedUtcTime = e.SignalTime.ToUniversalTime();

            try
            {
                this.Notify(elapsedUtcTime);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Trace.TraceError(ex.ToString());
                this.StopTracking();
            }
        }         
    }
}
