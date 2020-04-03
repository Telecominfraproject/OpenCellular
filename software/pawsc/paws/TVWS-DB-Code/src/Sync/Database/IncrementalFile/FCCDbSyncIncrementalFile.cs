// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Sync.Database.IncrementalFile
{
    using System;
    using System.Configuration;
    using System.Diagnostics;
    using System.Globalization;
    using System.Threading;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;

    /// <summary>
    /// Incremental File generations
    /// </summary>
    public class FCCDbSyncIncrementalFile : IDBSyncFileIncr
    {
        /// <summary>
        /// Gets or sets the Sync Manager
        /// </summary>
        [Dependency]
        public IDBSyncManager SyncManager { get; set; }

        /// <summary>
        /// Gets or sets the Logger
        /// </summary>
        [Dependency]
        public ILogger Logger { get; set; }

        /// <summary>
        /// Gets or sets the Auditor
        /// </summary>
        [Dependency]
        public IAuditor Auditor { get; set; }

        /// <summary>
        /// Generates files
        /// </summary>
        /// <param name="scope">scope of the DB Sync update</param>
        public void GenerateFile(DbSyncScope scope)
        {
            Thread.CurrentThread.CurrentCulture = new CultureInfo(Utils.Configuration["Culture"]);
            try
            {
                string adminName = Utils.Configuration[Constants.ConfigSettingWSDBAName];
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", Begin - FCCDbSyncIncrementalFile dbSyncManager GenerateRecordEnsemble()");

                // Calling DB Sync Manager for generating the file
                this.SyncManager.GenerateRecordEnsemble(scope);

                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", End - FCCDbSyncIncrementalFile dbSyncManager GenerateRecordEnsemble()");
                this.Auditor.Audit(AuditId.DBSyncFileGenerated, AuditStatus.Success, 0, "FCCDbSyncIncrementalFile dbSyncManager GenerateRecordEnsemble Success");
            }
            catch (Exception e)
            {
                this.Auditor.Audit(AuditId.DBSyncFileGenerated, AuditStatus.Failure, 0, "SyncManager " + "FCCDbSyncIncrementalFile SyncManager.GenerateRecordEnsemble - got exception :" + e.ToString());
                throw;
            }
        }
    }
}
