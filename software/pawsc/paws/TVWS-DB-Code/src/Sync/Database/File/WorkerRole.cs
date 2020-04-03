// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Sync.Database.File
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Diagnostics;
    using System.Globalization;
    using System.Linq;
    using System.Net;
    using System.Threading;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.WindowsAzure;
    using Microsoft.WindowsAzure.Diagnostics;
    using Microsoft.WindowsAzure.ServiceRuntime;
    using Microsoft.WindowsAzure.Storage;

    /// <summary>
    /// Worker role responsible for generating synchronization files.
    /// </summary>
    public class WorkerRole : RoleEntryPoint
    {
        /// <summary>
        /// holds the ILogger instance
        /// </summary>
        private ILogger logger;

        /// <summary>
        /// holds the IAuditor instance
        /// </summary>
        private IAuditor auditor;

        /// <summary>
        /// holds the FCC File instance
        /// </summary>
        private IDBSyncFile dbsyncFCCfiler = null;

        /// <summary>
        /// Runs code that is intended to be run for the life of the file synchronization worker.
        /// </summary>
        public override void Run()
        {
            IUnityContainer container = Utils.Configuration.CurrentContainer;
            this.logger = container.Resolve<ILogger>();
            this.auditor = container.Resolve<IAuditor>();

            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "DbSync File worker role starting");
            int sleepMilliSeconds = Utils.Configuration["DbSyncFileInterval_MilliSec"].ToInt32();

            while (true)
            {
                Stopwatch timer = new Stopwatch();

                // invoking the file service.
                try
                {
                    timer.Start();
                    this.GenerateAllRegistrationsFile();
                }
                catch (Exception ex)
                {
                    if (this.logger != null)
                    {
                        this.logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, ex.ToString());
                    }

                    if (this.auditor != null)
                    {
                        this.auditor.Audit(AuditId.DBSyncPollRequest, AuditStatus.Failure, timer.Elapsed.Milliseconds, ex.ToString());
                    }
                }
                finally
                {
                    timer.Stop();
                }

                Thread.Sleep(sleepMilliSeconds);
            }
        }

        /// <summary>
        /// Runs codes that initializes the File Sync worker.
        /// </summary>
        /// <returns>True if initialization succeeds; otherwise, false.</returns>
        public override bool OnStart()
        {
            Thread.CurrentThread.CurrentCulture = new CultureInfo(Microsoft.Whitespace.Common.Utils.Configuration["Culture"]);

            Utils.InitDiagnostics();

            // For information on handling configuration changes see the MSDN topic at http://go.microsoft.com/fwlink/?LinkId=166357.
            return base.OnStart();
        }

        /// <summary>
        /// Starts the DB Sync Polling.
        /// </summary>
        private void GenerateAllRegistrationsFile()
        {
            // Thread.CurrentThread.CurrentCulture = new CultureInfo(Utils.Configuration["Culture"]);
            if (this.dbsyncFCCfiler == null)
            {
                IUnityContainer container = Utils.Configuration.CurrentContainer;
                this.dbsyncFCCfiler = container.Resolve<IDBSyncFile>();
            }

            // calling filer
            this.dbsyncFCCfiler.GenerateFile(Entities.DbSyncScope.ALL);
        }
    }
}
