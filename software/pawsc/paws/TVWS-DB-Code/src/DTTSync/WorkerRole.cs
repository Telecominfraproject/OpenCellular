// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.DTTSync
{
    using System;
    using System.Net;
    using System.Threading;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.WindowsAzure.Diagnostics;
    using Microsoft.WindowsAzure.ServiceRuntime;

    /// <summary>
    /// Class WorkerRole.
    /// </summary>
    public class WorkerRole : RoleEntryPoint
    {
        /// <summary>
        /// Gets or sets the PMSE synchronize.
        /// </summary>
        /// <value>The PMSE synchronize.</value>
        private IDTTAvailabilitySync DttSync { get; set; }

        /// <summary>
        /// Called by Windows Azure after the role instance has been initialized. This method serves as the
        /// main thread of execution for your role.
        /// </summary>
        public override void Run()
        {
            try
            {
                IUnityContainer container = Utils.Configuration.CurrentContainer;
                this.DttSync = container.Resolve<IDTTAvailabilitySync>();

                TimeSpan syncTime =
                    TimeSpan.FromDays(Utils.Configuration["DttSyncSynchronizationTimer_Day"].ToInt32());

                while (true)
                {
                    this.DttSync.SyncDB();

                    Thread.Sleep(syncTime);
                }
            }
            catch (Exception)
            {
            }
        }

        /// <summary>
        /// Called by Windows Azure to initialize the role instance.
        /// </summary>
        /// <returns>True if initialization succeeds, False if it fails. The default implementation returns True.</returns>
        public override bool OnStart()
        {
            Utils.InitDiagnostics();
            return base.OnStart();
        }
    }
}
