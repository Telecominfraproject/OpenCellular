// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Sync.Database.Service
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.WindowsAzure;
    using Microsoft.WindowsAzure.Diagnostics;
    using Microsoft.WindowsAzure.ServiceRuntime;

    /// <summary>
    /// Default Web Role Entry Point for web service.
    /// </summary>
    public class WebRole : RoleEntryPoint
    {
        /// <summary>
        /// Handles the startup of the WCF web service by Azure.
        /// </summary>
        /// <returns>true if initialization succeeds; otherwise, false.</returns>
        public override bool OnStart()
        {
            this.InitDiagnostics();
            return base.OnStart();
        }

        /// <summary>
        /// Initializes the diagnostics.
        /// </summary>
        private void InitDiagnostics()
        {
            // Get default initial configuration.
            var config = DiagnosticMonitor.GetDefaultInitialConfiguration();
            var logLevelTransferTime = RoleEnvironment.GetConfigurationSettingValue("LogLevelTransferTime").ToInt32();
            if (logLevelTransferTime == 0)
            {
                logLevelTransferTime = 5;
            }

            config.Logs.ScheduledTransferPeriod = TimeSpan.FromMinutes(logLevelTransferTime);

            // Start the diagnostic monitor with the modified configuration.
            DiagnosticMonitor.Start("AzureStorageAccountConnectionString", config);
        }
    }
}
