// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace IncrementalFile
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Diagnostics;
    using System.Linq;
    using System.Net;
    using System.Threading;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure;
    using Microsoft.WindowsAzure.Diagnostics;
    using Microsoft.WindowsAzure.ServiceRuntime;
    using Microsoft.WindowsAzure.Storage;

    /// <summary>
    /// Azure Worker Role
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
        private IDBSyncFileIncr dbsyncFCCfiler = null;

        /// <summary>
        /// Runs code that is intended to be run for the life of the file synchronization worker.
        /// </summary>
        public override void Run()
        {
            IUnityContainer container = Utils.Configuration.CurrentContainer;
            this.logger = container.Resolve<ILogger>();
            this.auditor = container.Resolve<IAuditor>();
            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "DbSync File worker role starting");
            int sleepMilliSeconds = Utils.Configuration[Constants.ConfigSettingDbSyncFileInterval].ToInt32();

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
                        this.logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncFileWorkerGenericMessage, ex.ToString());
                    }

                    if (this.auditor != null)
                    {
                        this.auditor.Audit(AuditId.DBSyncFileGenerated, AuditStatus.Failure, timer.Elapsed.Milliseconds, ex.ToString());
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
            Utils.InitDiagnostics();

            // For information on handling configuration changes
            // see the MSDN topic at http://go.microsoft.com/fwlink/?LinkId=166357.
            return base.OnStart();
        }

        /// <summary>
        /// Starts the DB Sync Polling.
        /// </summary>
        private void GenerateAllRegistrationsFile()
        {
            if (this.dbsyncFCCfiler == null)
            {
                IUnityContainer container = Utils.Configuration.CurrentContainer;
                this.dbsyncFCCfiler = container.Resolve<IDBSyncFileIncr>();
            }

            this.dbsyncFCCfiler.GenerateFile(DbSyncScope.INC);
        }
    }
}
