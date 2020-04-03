// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.RegionManagement
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using Common;
    using Common.Utilities;
    using Microsoft.Practices.Unity;
    using WindowsAzure.Diagnostics;
    using WindowsAzure.ServiceRuntime;
   
    /// <summary>
    ///     Web service responsible for handling region management.
    /// </summary>
    public class WebRole : RoleEntryPoint
    {
        /// <summary>
        ///     Runs codes that initializes the region management web service.
        /// </summary>
        /// <returns>True if initialization succeeds; otherwise, false.</returns>
        public override bool OnStart()
        {
            Utils.InitDiagnostics();
            return base.OnStart();
        }

        /// <summary>
        ///     Runs this instance.
        /// </summary>
        public override void Run()
        {
            this.InitNativeServiceHost();           

            base.Run();
        }

        /// <summary>
        /// Initializes the native service host.
        /// </summary>
        private void InitNativeServiceHost()
        {
            ProcessStartInfo psi = new ProcessStartInfo(Path.Combine(Utils.GetValidAssemblyPath(), "NativeReference\\NativeCodeHostService.exe"));
            Process hostProcess = Process.Start(psi);
        }
    }
}
