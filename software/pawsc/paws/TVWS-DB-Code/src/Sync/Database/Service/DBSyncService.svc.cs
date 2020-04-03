// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Sync.Database.Service
{
    using System;
    using System.Configuration;
    using System.Diagnostics;
    using System.Globalization;
    using System.ServiceModel;
    using System.Threading;
    using System.Xml;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;

    /// <summary>
    /// Defines the main DB Sync service.
    /// </summary>
    [ServiceBehavior(Namespace = "http://www.whitespace-db-providers.org/2011//InterDB/ws")]
    public class DBSyncService : IDbSyncService
    {
        /// <summary>
        /// Gets or sets the Sync Manager
        /// </summary>
        [Dependency]
        public IDBSyncManager DbSyncManager { get; set; }

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
        /// Real time poll a public method to be called for DB sync incremental updated
        /// </summary>
        /// <param name="realtimePollRequest">Value passed in from web service.</param>
        /// <returns>Returns a string format of the passed in value.</returns>
        public RealTimePollOutPut RealTimePoll(RealTimePollInput realtimePollRequest)
        {
            Thread.CurrentThread.CurrentCulture = new CultureInfo(Utils.Configuration["Culture"]);
            this.InitializeObjects();
            
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin RealTimePoll");
            ParsePollRequestResult pollResult = null;
            RealTimePollOutPut realTimeResponse = new RealTimePollOutPut();
            XmlDocument xmlDoc = new XmlDocument();
            try
            {
                realTimeResponse.RequestedTransactionID = realtimePollRequest.RequestedTransactionID;
                realTimeResponse.Command = realtimePollRequest.Command;
                realTimeResponse.XsdVersion = realtimePollRequest.XsdVersion;

                // DB Sync Version Check
                if (realTimeResponse.XsdVersion == Utils.Configuration["DBSyncVersionNo"])
                {
                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin  RealTimePoll DbSyncManager.ParsePollRequest");
                    pollResult = this.DbSyncManager.ParsePollRequest(realtimePollRequest.RequestedTransactionID);
                    if (pollResult != null)
                    {
                        // response code success
                        if (pollResult.RTResponseCode == PollStatus.Success)
                        {
                            xmlDoc.LoadXml(pollResult.ResponseData);
                            XmlNamespaceManager namespaceManager = new XmlNamespaceManager(xmlDoc.NameTable);
                            namespaceManager.AddNamespace("ren", xmlDoc.DocumentElement.NamespaceURI);

                            realTimeResponse.RegistrationRecordEnsemble = xmlDoc.SelectSingleNode("//ren:RegistrationRecordEnsemble", namespaceManager).OuterXml;
                        }

                        realTimeResponse.RTPollStatusCode = Convert.ToInt32(pollResult.RTResponseCode);
                        realTimeResponse.NextTransactionID = pollResult.NextTransactionId;
                    }
                    else
                    {
                        // Bad Request Case
                        this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "RealTimePoll DbSyncManager.ParsePollRequest : BadRequest");
                        realTimeResponse.RequestedTransactionID = realtimePollRequest.RequestedTransactionID;
                        realTimeResponse.RTPollStatusCode = Convert.ToInt32(PollStatus.BadRequest);
                        realTimeResponse.NextTransactionID = realtimePollRequest.RequestedTransactionID;
                    }
                }
                else
                {
                    // DB Sync version not supported
                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "RealTimePoll DbSyncManager.ParsePollRequest : Requested Version not supported");
                    realTimeResponse.RequestedTransactionID = realtimePollRequest.RequestedTransactionID;
                    realTimeResponse.RTPollStatusCode = Convert.ToInt32(PollStatus.RequestedVersionnotsupported);
                    realTimeResponse.NextTransactionID = realtimePollRequest.RequestedTransactionID;
                }
            }
            catch (Exception ex)
            {
                // generate the reponse for the server error
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, "RealTimePoll got an error " + ex.ToString());
                realTimeResponse.RequestedTransactionID = realtimePollRequest.RequestedTransactionID;
                realTimeResponse.RTPollStatusCode = Convert.ToInt32(PollStatus.ServerError);
                realTimeResponse.NextTransactionID = realtimePollRequest.RequestedTransactionID;
                this.Auditor.Audit(AuditId.DBSyncPollRequest, AuditStatus.Failure, 0, "Incremental Polling through web service Requested Transaction ID :" + realtimePollRequest.RequestedTransactionID);
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Exit RealTimePoll");
            this.Auditor.Audit(AuditId.DBSyncPollRequest, AuditStatus.Success, 0, "Incremental Polling through web service Requested Transaction ID :" + realtimePollRequest.RequestedTransactionID);
            return realTimeResponse;
        }

        /// <summary>
        /// Initialize Objects
        /// </summary>       
        private void InitializeObjects()
        {
            IConfiguration configuration = (AzureConfig)Utils.Configuration;
            this.DbSyncManager = configuration.CurrentContainer.Resolve<IDBSyncManager>();
            this.Logger = configuration.CurrentContainer.Resolve<ILogger>();
            this.Auditor = configuration.CurrentContainer.Resolve<IAuditor>();
        }
    }
}
