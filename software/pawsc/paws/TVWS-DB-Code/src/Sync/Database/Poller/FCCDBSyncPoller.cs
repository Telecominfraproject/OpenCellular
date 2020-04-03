// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Sync.Database.Poller
{
    using System;
    using System.Configuration;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Threading;
    using System.Web;
    using System.Xml;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;
    using Microsoft.Whitespace.Sync.Database.Manager;

    /// <summary>
    /// Represents Poll Status
    /// </summary>
    public enum PollStatus
    {
        /// <summary>
        /// The Success.
        /// </summary>
        Success = 0,

        /// <summary>
        /// The transaction id stale.
        /// </summary>
        TransactionIDStale = 1,

        /// <summary>
        /// The bad request.
        /// </summary>
        BadRequest = 2,

        /// <summary>
        /// The requested version not supported.
        /// </summary>
        RequestedVersionnotsupported = 3,

        /// <summary>
        /// No new records.
        /// </summary>
        NoNewRecords = 4,

        /// <summary>
        /// The server error.
        /// </summary>
        ServerError = 5
    }

    /// <summary>
    /// Represents US DBSyncPOLLER
    /// </summary>
    public class FCCDBSyncPoller : IDBSyncPoller
    {
        /// <summary>
        /// holds Transactions id
        /// </summary>
        private Guid transactionId = Guid.NewGuid();

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
        /// Method to Poll
        /// </summary>
        public void Poll()
        {
            Thread.CurrentThread.CurrentCulture = new CultureInfo(Utils.Configuration["Culture"]);
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin" + "FCCDBSyncPoller.cs: Methods:Poll");
            DBAdminInfo[] dbadminInfo = this.SyncManager.GetDBAdmininfo();
            XmlDocument xmlDoc = new XmlDocument();
            xmlDoc.PreserveWhitespace = true;
            for (int index = 0; index < dbadminInfo.Length; index++)
            {
                try
                {
                    string responseXml = null;
                    int responseCode = 200;
                    XmlNamespaceManager namespacemanager = new XmlNamespaceManager(xmlDoc.NameTable);
                    namespacemanager.AddNamespace("s", "http://schemas.xmlsoap.org/soap/envelope/");
                    namespacemanager.AddNamespace("m", "http://www.whitespace-db-providers.org/2011//InterDB/ws");

                    if ((dbadminInfo[index].NextTransactionID != null) && (dbadminInfo[index].NextTransactionID != string.Empty))
                    {
                        this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin SendingPollRequest : FCCDBSyncPoller.cs: Methods:Poll - ");
                        responseXml = this.SendPollRequest(dbadminInfo[index]);
                        this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End  SendingPollRequest : FCCDBSyncPoller.cs: Methods:Poll - ");
                        if (responseXml != string.Empty)
                        {
                            responseXml = responseXml.Replace("<?xml version=\"1.0\" encoding=\"UTF-8\"?>", string.Empty);
                            xmlDoc.LoadXml(responseXml);

                            responseCode = Convert.ToInt32(xmlDoc.SelectSingleNode("//m:RT-PollStatusCode", namespacemanager).InnerText);
                            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "FCCDBSyncPoller.cs: Methods:Poll - Response Code : " + responseCode.ToString());

                            if (responseCode == (int)PollStatus.Success)
                            {
                                try
                                {
                                    //// update the Azure registration tables.
                                    this.SyncManager.ParsePollResponse(dbadminInfo[index].Name, xmlDoc.SelectSingleNode("//m:RegistrationRecordEnsemble", namespacemanager).InnerXml, dbadminInfo[index].PublicKey);

                                    // Update Next Trasaction ID
                                    dbadminInfo[index].NextTransactionID = xmlDoc.SelectSingleNode("//m:RequestedTransactionID", namespacemanager).InnerText;
                                    this.SyncManager.UpdateDBAdminInfo(dbadminInfo[index]);
                                }
                                catch (Exception e)
                                {
                                    this.Auditor.Audit(AuditId.DBSyncPollRequest, AuditStatus.Failure, 0, "FCCDBSyncPoller - Poll - XML Parsing Exception :" + e.ToString());
                                    this.Logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, "FCCDBSyncPoller - Poll() - XML Parsing Exception :" + e.ToString());
                                }
                            }
                            else if (responseCode == (int)PollStatus.TransactionIDStale)
                            {
                                this.PollSFTPServer(dbadminInfo[index]);
                            }
                            else if (responseCode == (int)PollStatus.BadRequest)
                            {
                                // Bad Request 
                                this.Auditor.Audit(AuditId.DBSyncPollRequest, AuditStatus.Failure, 0, "WSDBA :" + dbadminInfo[index].Name + ", Poll Url:" + dbadminInfo[index].WebServiceUrl + ", NextransactionId:" + dbadminInfo[index].NextTransactionID + " Caused bad request");
                                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA :" + dbadminInfo[index].Name + ", Poll Url:" + dbadminInfo[index].WebServiceUrl + ", NextransactionId:" + dbadminInfo[index].NextTransactionID + " Caused bad request");
                            }
                            else if (responseCode == (int)PollStatus.RequestedVersionnotsupported)
                            {
                                // Version not supported.
                                this.Auditor.Audit(AuditId.DBSyncPollRequest, AuditStatus.Failure, 0, "WSDBA :" + dbadminInfo[index].Name + ", Poll Url:" + dbadminInfo[index].WebServiceUrl + ", NextransactionId:" + dbadminInfo[index].NextTransactionID + " Requested Version is not supported");
                                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA :" + dbadminInfo[index].Name + ", Poll Url:" + dbadminInfo[index].WebServiceUrl + ", NextransactionId:" + dbadminInfo[index].NextTransactionID + " Requested Version is not supported");
                            }
                            else if (responseCode == (int)PollStatus.NoNewRecords)
                            {
                                // No New records -- post Audit Action
                                this.Auditor.Audit(AuditId.DBSyncPollRequest, AuditStatus.Success, 0, "WSDBA :" + dbadminInfo[index].Name + ", Poll Url:" + dbadminInfo[index].WebServiceUrl + ", NextransacionId:" + dbadminInfo[index].NextTransactionID + " No New Records.");
                                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA :" + dbadminInfo[index].Name + ", Poll Url:" + dbadminInfo[index].WebServiceUrl + ", NextransacionId:" + dbadminInfo[index].NextTransactionID + " No New Records.");
                                dbadminInfo[index].NextTransactionID = xmlDoc.SelectSingleNode("//m:RequestedTransactionID", namespacemanager).InnerText;
                                this.SyncManager.UpdateDBAdminInfo(dbadminInfo[index]);
                            }
                            else if (responseCode == (int)PollStatus.ServerError)
                            {
                                // Server Error Request keep try
                                this.Auditor.Audit(AuditId.DBSyncPollRequest, AuditStatus.Failure, 0, "WSDBA :" + dbadminInfo[index].Name + ", Poll Url:" + dbadminInfo[index].WebServiceUrl + ", NextransacionId:" + dbadminInfo[index].NextTransactionID + " WSDBA's server error.");
                                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA :" + dbadminInfo[index].Name + ", Poll Url:" + dbadminInfo[index].WebServiceUrl + ", NextransacionId:" + dbadminInfo[index].NextTransactionID + " WSDBA's server error.");
                            }
                        }
                    }
                    else
                    {
                        this.PollSFTPServer(dbadminInfo[index]);
                    }
                }
                catch (Exception ex)
                {
                    this.Logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, "Error" + "WSDBA :" + dbadminInfo[index].Name + ex.ToString());
                    this.Auditor.Audit(AuditId.DBSyncPollRequest, AuditStatus.Failure, 0, "WSDBA :" + dbadminInfo[index].Name + ", Poll Url:" + dbadminInfo[index].WebServiceUrl + ", NextransacionId:" + dbadminInfo[index].NextTransactionID + " WSDBA's server error.");
                }
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End" + "FCCDBSyncPoller.cs: Methods:Poll");
        }

        /// <summary>
        /// Polling of SFTP server to get the all registrations file.
        /// </summary>
        /// <param name="adminInfo"> DB Admin info</param>
        private void PollSFTPServer(DBAdminInfo adminInfo)
        {
            // call to sftp  server for getting all regisrations file
            string filename = null;
            SFTPHelper sftpClient = new SFTPHelper(this.Logger);
            XmlNamespaceManager namespacemanager;
            try
            {
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin FCCDBSyncPoller - Poll() - making SFTPClint Call ");
                filename = sftpClient.GetAllXMLfileFromSFTP(adminInfo.FtpServerUrl, adminInfo.SFTPPath, adminInfo.FtpServerUserId, adminInfo.FtpServerPwd, Utils.GetOutputDirPathForConfigKey("FTPFileDownloadPath"));
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End FCCDBSyncPoller - Poll() - making SFTPClint Call ");

                XmlDocument xmlDocAll = new XmlDocument();
                xmlDocAll.PreserveWhitespace = true;
                xmlDocAll.Load(filename);

                namespacemanager = new XmlNamespaceManager(xmlDocAll.NameTable);
                namespacemanager.AddNamespace("m", xmlDocAll.DocumentElement.NamespaceURI);

                this.SyncManager.ParsePollResponse(adminInfo.Name, xmlDocAll.OuterXml, adminInfo.PublicKey);

                // Update Next Trasaction ID
                adminInfo.NextTransactionID = xmlDocAll.SelectSingleNode("//m:NextTransactionID", namespacemanager).InnerText;
                this.SyncManager.UpdateDBAdminInfo(adminInfo);
            }
            catch (Exception e)
            {
                this.Auditor.Audit(AuditId.DBSyncPollRequest, AuditStatus.Failure, 0, "FCCDBSyncPoller - Poll() - SFP file get Exception :" + e.ToString());
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, "FCCDBSyncPoller - Poll() - SFTP file get Exception :" + e.ToString());
            }
        }

        /// <summary>
        /// Send Poll request
        /// </summary>
        /// <param name="dbadmin">DB admin info</param>
        /// <returns>string response</returns>
        private string SendPollRequest(DBAdminInfo dbadmin)
        {
            string reponseXml = string.Empty;
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start();
            Stream requestStream = null;
            HttpWebRequest httpWebRequest = null;
            HttpWebResponse wr = null;
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin FCCDBSyncPoller - SendPollRequest() - Calling WiteSpace DBA web service URL:" + dbadmin.WebServiceUrl);
            try
            {
                httpWebRequest = WebRequest.Create(dbadmin.WebServiceUrl) as HttpWebRequest;
                httpWebRequest.Method = "POST";
                httpWebRequest.ContentType = "text/xml; charset=utf-8"; // "application/xml";
                httpWebRequest.Headers.Add("SOAPAction", "http://www.whitespace-db-providers.org/2011//InterDB/ws/RealTimePoll");

                using (requestStream = httpWebRequest.GetRequestStream())
                {
                    requestStream = this.SyncManager.GeneratePollRequest(requestStream, dbadmin.NextTransactionID);
                    using (wr = (HttpWebResponse)httpWebRequest.GetResponse())
                    {
                        if (wr.StatusCode != HttpStatusCode.OK)
                        {
                            stopWatch.Stop();
                            this.Auditor.Audit(AuditId.DBSyncPollRequest, AuditStatus.Failure, stopWatch.Elapsed.Ticks, "FCCDBSyncPoller - SendPollRequest - Response Status code :" + wr.StatusCode.ToString());
                            this.Logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, "FCCDBSyncPoller - SendPollRequest - Response Status code :" + wr.StatusCode.ToString());
                        }
                        else
                        {
                            using (StreamReader srd = new StreamReader(wr.GetResponseStream()))
                            {
                                reponseXml = srd.ReadToEnd();
                            }
                        }
                    }
                }
            }
            catch (WebException e)
            {
                stopWatch.Stop();
                this.Auditor.Audit(AuditId.DBSyncPollRequest, AuditStatus.Failure, stopWatch.Elapsed.Ticks, e.ToString());
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, "FCCDBSyncPoller - SendPollRequest - " + e.ToString());
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End   FCCDBSyncPoller - SendPollRequest() - Calling WiteSpace DBA web service URL");

            reponseXml = HttpUtility.HtmlDecode(reponseXml);
            return reponseXml;
        }
    }
}
