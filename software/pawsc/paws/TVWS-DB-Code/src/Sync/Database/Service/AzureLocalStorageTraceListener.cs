// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Sync.Database.Service
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using Microsoft.WindowsAzure.Diagnostics;
    using Microsoft.WindowsAzure.ServiceRuntime;

    /// <summary>
    /// Default Trace Listener for Azure Local Storage.
    /// </summary>
    public class AzureLocalStorageTraceListener : XmlWriterTraceListener
    {
        /// <summary>
        /// Initializes a new instance of the AzureLocalStorageTraceListener class.
        /// </summary>
        public AzureLocalStorageTraceListener()
            : base(Path.Combine(AzureLocalStorageTraceListener.GetLogDirectory().Path, "Microsoft.Whitespace.Sync.Database.Service.svclog"))
        {
        }

        /// <summary>
        /// Gets the log directory configuration.
        /// </summary>
        /// <returns>Returns the log directory configuration.</returns>
        public static DirectoryConfiguration GetLogDirectory()
        {
            DirectoryConfiguration directory = new DirectoryConfiguration();
            directory.Container = "wad-tracefiles";
            directory.DirectoryQuotaInMB = 10;
            directory.Path = RoleEnvironment.GetLocalResource("Microsoft.Whitespace.Sync.Database.Service.svclog").RootPath;
            return directory;
        }
    }
}
