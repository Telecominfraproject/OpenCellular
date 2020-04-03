// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Net;
    using System.Web;
    using Microsoft.WindowsAzure.Diagnostics;
    using Microsoft.WindowsAzure.Diagnostics.Management;
    using Microsoft.WindowsAzure.ServiceRuntime;

    public class WebRole : RoleEntryPoint
    {
        public override bool OnStart()
        {
            ServicePointManager.DefaultConnectionLimit = 12;
            TimeSpan timeToTransfer;

            //// IIS Log.
            timeToTransfer = TimeSpan.FromMinutes(5);
            //// LocalResource localResource = RoleEnvironment.GetLocalResource("IISLogs");

            //// DirectoryConfiguration dirConfig = new DirectoryConfiguration();
            //// dirConfig.Container = "wad-iis-logcontainer";
            //// dirConfig.DirectoryQuotaInMB = localResource.MaximumSizeInMegabytes;
            //// dirConfig.Path = localResource.RootPath;

            DiagnosticMonitorConfiguration diagMonitorConfig = DiagnosticMonitor.GetDefaultInitialConfiguration();
            //// diagMonitorConfig.Directories.ScheduledTransferPeriod = timeToTransfer;
            //// diagMonitorConfig.OverallQuotaInMB = 4080;
            //// diagMonitorConfig.Directories.DataSources.Add(dirConfig);

            //// Logging diagnostic information to WADLog storage table.
            timeToTransfer = TimeSpan.FromMinutes(5);
            diagMonitorConfig.Logs.ScheduledTransferPeriod = timeToTransfer;
            diagMonitorConfig.Logs.ScheduledTransferLogLevelFilter = LogLevel.Verbose;

            //// Start diagnostic monitor.
            DiagnosticMonitor diagMonitor = DiagnosticMonitor.Start("Microsoft.WindowsAzure.Plugins.Diagnostics.ConnectionString", diagMonitorConfig);

            return base.OnStart();   
        }
    }
}
