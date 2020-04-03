// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.PAWS.Service
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Net;
    using Entities;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.WindowsAzure;
    using Microsoft.WindowsAzure.Diagnostics;
    using Microsoft.WindowsAzure.ServiceRuntime;
   
    /// <summary>
    /// Web service responsible for all Paws request.
    /// </summary>
    public class WebRole : RoleEntryPoint
    {
        /// <summary>
        /// Runs codes that initializes the Paws Web service.
        /// </summary>
        /// <returns>True if initialization succeeds; otherwise, false.</returns>
        public override bool OnStart()
        {
            Utils.InitDiagnostics();

            // For information on handling configuration changes
            // see the MSDN topic at http://go.microsoft.com/fwlink/?LinkId=166357.
            return base.OnStart();
        }

        /// <summary>
        /// Runs this instance.
        /// </summary>
        public override void Run()
        {   
            base.Run();
        }
    }
}
