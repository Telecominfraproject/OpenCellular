// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Sync.Region
{
    using System;
    using System.Configuration;
    using System.Diagnostics;
    using System.IO;
    using System.Net;
    using System.Threading;
    using System.Timers;
    using Microsoft.ApplicationServer.Caching;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.WindowsAzure.Diagnostics;
    using Microsoft.WindowsAzure.ServiceRuntime;

    /// <summary>Worker role responsible for region management synchronization.</summary>
    public class WorkerRole : RoleEntryPoint
    {
        /// <summary>Gets or sets the region synchronize worker.</summary>
        /// <value>The region synchronize worker.</value>
        private IRegionSync RegionSyncDb { get; set; }

        /// <summary>Runs code that is intended to be run for the life of the region synchronization worker.</summary>
        public override void Run()
        {
            try
            {
                this.InitNativeServiceHost();
                IUnityContainer container = Utils.Configuration.CurrentContainer;
                this.RegionSyncDb = container.Resolve<IRegionSync>();

                TimeSpan syncTime = TimeSpan.FromDays(Utils.Configuration["RegionSyncSynchronizationTimer_Day"].ToInt32());

                while (true)
                {
                    this.RegionSyncDb.SyncDB();

                    Thread.Sleep(syncTime);
                }
            }
            catch (Exception)
            {
            }
        }

        /// <summary>
        /// Initializes the native service host.
        /// </summary>
        public void InitNativeServiceHost()
        {
            ProcessStartInfo psi = new ProcessStartInfo(Path.Combine(Utils.GetValidAssemblyPath(), "NativeReference\\NativeCodeHostService.exe"));
            Process hostProcess = Process.Start(psi);
        }

        /// <summary>Runs codes that initializes the region sync worker.</summary>
        /// <returns>True if initialization succeeds; otherwise, false.</returns>
        public override bool OnStart()
        {
            Utils.InitDiagnostics();
            return base.OnStart();
        }
    }
}
