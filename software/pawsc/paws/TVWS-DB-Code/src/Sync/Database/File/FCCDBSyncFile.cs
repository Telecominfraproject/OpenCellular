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
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;
    using Microsoft.Whitespace.Sync;

    /// <summary>
    /// FCC DBSync File
    /// </summary>
    public class FCCDBSyncFile : IDBSyncFile
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
        /// Generates the Registrations File
        /// </summary>
        /// <param name="scope"> scope of the DB Sync</param>
        public void GenerateFile(DbSyncScope scope)
        {
            Thread.CurrentThread.CurrentCulture = new CultureInfo(Utils.Configuration["Culture"]);
            try
            {
                string adminName = Utils.Configuration[Constants.ConfigSettingWSDBAName];
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", Begin - FCCDBSyncFile dbSyncManager GenerateRecordEnsemble()");

                // Calling DB Sync Manager for generating the file
                this.SyncManager.GenerateRecordEnsemble(scope);

                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", End - FCCDBSyncFile dbSyncManager GenerateRecordEnsemble()");
                this.Auditor.Audit(AuditId.DBSyncFileGenerated, AuditStatus.Success, 0, "FCCDBSyncFile dbSyncManager GenerateRecordEnsemble Success");
            }
            catch (Exception e)
            {
                this.Auditor.Audit(AuditId.DBSyncFileGenerated, AuditStatus.Failure, 0, "SyncManager " + "FCCDBSyncFile SyncManager.GenerateRecordEnsemble - got exception :" + e.ToString());
                throw;
            }
        }
    }
}
