// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Sync.Database.Manager
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Diagnostics;
    using System.IO;
    using System.IO.Compression;
    using System.Linq;
    using System.Net;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;
    using System.Threading.Tasks;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;
    using Microsoft.Whitespace.Entities.Versitcard;
    using Microsoft.WindowsAzure.Storage;
    using Security;
    using Security.Cryptography;

    /// <summary>
    /// Primary public interface for the DB Sync Manager.
    /// </summary>
    public class SyncManager : IDBSyncManager
    {
        /// <summary>
        /// Holds Registration Exist Status
        /// </summary>
        private bool registrationExist = false;

        /// <summary>
        /// private interface for the DALC.
        /// </summary>
        private IDalcDBSync dalcDbSync;

        /// <summary>
        /// holds logger instance
        /// </summary>
        private ILogger logger;

        /// <summary>
        ///  holds the instance of auditor
        /// </summary>
        private IAuditor auditor;

        /// <summary>
        /// RegistrationEntityBuilder class used to construct the registration objects.
        /// </summary>
        private RegistrationEntityBuilder regbuilder = null;

        /// <summary>
        /// Gets or sets the DALC for database sync
        /// </summary>
        [Dependency]
        public IDalcDBSync DalcDBSync
        {
            get { return this.dalcDbSync; }
            set { this.dalcDbSync = value; }
        }

        /// <summary>
        /// Gets or sets the logger instance
        /// </summary>
        [Dependency]
        public ILogger Logger
        {
            get { return this.logger; }
            set { this.logger = value; }
        }

        /// <summary>
        /// Gets or sets the logger instance
        /// </summary>
        [Dependency]
        public IAuditor Auditor
        {
            get { return this.auditor; }
            set { this.auditor = value; }
        }

        /// <summary>
        /// Parses all poll request from external DB Admins. 
        /// </summary>
        /// <param name="nextTransactionID">The poll request data.</param>
        /// <returns>Returns a response stream that is to be sent back to the DB Admins.</returns>
        public ParsePollRequestResult ParsePollRequest(string nextTransactionID)
        {
            // ToDo: save value to azure table.
            return this.GenerateRecordEnsemble(nextTransactionID);
        }

        /// <summary>
        /// Generates RealTimePollRequest for the specified DB Administrator.
        /// </summary>
        /// <param name="stream">A DB Admin name such as SPBR, TELC, MSFT, KEYB, ...</param>
        /// <param name="nextTransactionId">Next transaction Id</param>
        /// <returns>Returns an XML stream of the RealTimePollRequest.</returns>
        public Stream GeneratePollRequest(Stream stream, string nextTransactionId)
        {
            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin - SyncManager - Generate Poll Response");
            StreamWriter streamWriter = new StreamWriter(stream, Encoding.ASCII);
            string dbsyncVersionNo = null;

            // preparing the stram with poll request data.
            StringBuilder soapRequest = new StringBuilder("<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">");
            dbsyncVersionNo = Utils.Configuration["DBSyncVersionNo"];
            soapRequest.Append("<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"><m:RealTimePollRequest xmlns:m=\"http://www.whitespace-db-providers.org/2011//InterDB/ws\"><m:RequestedTransactionID>" + nextTransactionId + "</m:RequestedTransactionID><m:Command>wsdPoll</m:Command><m:XsdVersion>" + dbsyncVersionNo + "</m:XsdVersion></m:RealTimePollRequest></s:Body></s:Envelope>");

            streamWriter.Write(soapRequest.ToString());
            streamWriter.Close();
            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End - SyncManager - Generate Poll Response");
            return stream;
        }

        /// <summary>
        /// Parses the response XML from the fast poll request and updates the local DB.
        /// </summary>
        /// <param name="adminName">The whitespace DB who generated the poll response.</param>
        /// <param name="pollResponseXml">The response from the remote DB Admin that is to be parsed.</param>
        /// <param name="publicKey">public key of the administrator</param>
        public void ParsePollResponse(string adminName, string pollResponseXml, string publicKey)
        {
            // ToDo: save value to azure table.
            // throw new NotImplementedException();
            try
            {
                this.regbuilder = new RegistrationEntityBuilder(this.logger);
                this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin " + "SyncManager.ParsePollResponse");
                this.ParseResponseXML(adminName, pollResponseXml, publicKey);
                this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End " + "SyncManager.ParsePollResponse");
            }
            catch (Exception ex)
            {
                this.logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, ex.ToString());
                throw;
            }
        }

        /// <summary>
        /// Generates an XML file (either generates the full XML or an incremental file).
        /// </summary>
        /// <param name="nextTransactionId">Indicates if a full XML file should be generated 
        /// or just a partial incremental file.</param>
        /// <returns>Returns an XML stream of the file content.</returns>
        public ParsePollRequestResult GenerateRecordEnsemble(string nextTransactionId)
        {
            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin - SyncManager - GenerateRecordEnsemble transaction ID :" + nextTransactionId);
            ParsePollRequestResult pollResult = new ParsePollRequestResult();
            XmlDocument xmldoc = null;

            // get data from azure tables
            NextTransactionID reqTransId = this.dalcDbSync.GetRequestorTransactionId(nextTransactionId);

            // Bad Request 
            if (reqTransId == null)
            {
                pollResult.RTResponseCode = PollStatus.BadRequest;
            }
            else if (reqTransId.Timestamp.AddHours(72) >= DateTime.Now)
            {
                // Transaction ID not older than 72 hour - ok
                try
                {
                    xmldoc = this.BuildRecordEnsumble(DbSyncScope.INC, reqTransId, "External");
                }
                catch
                {
                    throw;
                }

                xmldoc = XmlCryptography.SignXmlDcoument(Utils.GetFilePathForConfigKey("SignCertificate"), xmldoc);
                pollResult.ResponseData = xmldoc.OuterXml;
                XmlNamespaceManager namespaceManager = new XmlNamespaceManager(xmldoc.NameTable);
                namespaceManager.AddNamespace("ren", xmldoc.DocumentElement.NamespaceURI);

                XmlNodeList nodelist = xmldoc.SelectNodes("//ren:Registration", namespaceManager);
                if (nodelist != null && nodelist.Count > 0)
                {
                    this.VerifyRecordEnsembleGenerated(xmldoc);

                    pollResult.RTResponseCode = PollStatus.Success;
                    pollResult.ResponseData = xmldoc.OuterXml;
                    pollResult.NextTransactionId = xmldoc.SelectSingleNode("//ren:NextTransactionID", namespaceManager).InnerXml;
                }
                else
                {
                    pollResult.RTResponseCode = PollStatus.NoNewRecords;
                    pollResult.NextTransactionId = xmldoc.SelectSingleNode("//ren:NextTransactionID", namespaceManager).InnerXml;
                }
            }
            else if (reqTransId.Timestamp.AddHours(72) < DateTime.Now)
            {
                // Transaction ID Stale > 72 hours
                pollResult.RTResponseCode = PollStatus.TransactionIDStale;
            }

            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End - SyncManager - GenerateRecordEnsemble transaction ID :" + nextTransactionId);
            return pollResult;
        }

        /// <summary>
        /// Generates an XML file (either generates the full XML).
        /// </summary>
        /// <param name="scope">Indicates if a full XML file should be generated 
        /// or just a partial incremental file.</param>
        /// <returns>Returns an XML string of the file content.</returns>
        public string GenerateRecordEnsemble(DbSyncScope scope)
        {
            string wsdba = Utils.Configuration[Constants.ConfigSettingWSDBAName];
            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin - SyncManager - GenerateRecordEnsemble scope :" + scope);
            XmlDocument xmlDocAll = new XmlDocument();
            string signedOutputFilePath = null;
            try
            {
                NextTransactionID reqTransId = this.dalcDbSync.GetRequestorTransactionId(wsdba, scope);

                // generating xml Doc
                xmlDocAll = this.BuildRecordEnsumble(scope, reqTransId, wsdba);

                // Registrations count check
                if (this.registrationExist)
                {
                    // Signing the XML
                    xmlDocAll = XmlCryptography.SignXmlDcoument(Utils.GetFilePathForConfigKey("SignCertificate"), xmlDocAll);
                    this.VerifyRecordEnsembleGenerated(xmlDocAll);

                    // <n1:NextTransactionID/>
                    string timeStamp = "F" + DateTime.Now.ToFileTime() + "\\";
                    string signedOutPutFileName = Utils.Configuration["Registrar"] + ".V" + Utils.Configuration["DBSyncVersionNo"] + "." + scope.ToString() + "." + DateTime.Now.Year.ToString() + DateTime.Now.Month.ToString() + DateTime.Now.Day.ToString() + "T" + DateTime.Now.Hour.ToString() + DateTime.Now.Minute.ToString() + DateTime.Now.Second.ToString() + "Z";
                    if (!Directory.Exists(Utils.GetOutputDirPathForConfigKey("SignedOutputFilePath") + timeStamp))
                    {
                        Directory.CreateDirectory(Utils.GetOutputDirPathForConfigKey("SignedOutputFilePath") + timeStamp);
                    }

                    signedOutputFilePath = Utils.GetOutputDirPathForConfigKey("SignedOutputFilePath") + timeStamp + signedOutPutFileName + ".xml";

                    XmlTextWriter writer = null;

                    // Saving signed xml to file
                    using (writer = new XmlTextWriter(signedOutputFilePath, null))
                    {
                        writer.Formatting = Formatting.Indented;
                        xmlDocAll.Save(writer);
                    }

                    // compressing the xml file
                    this.CreateZipFile(Utils.GetOutputDirPathForConfigKey("SignedOutputFilePath") + timeStamp + signedOutPutFileName + ".xml", Utils.GetOutputDirPathForConfigKey("SignedOutputFilePath") + timeStamp + signedOutPutFileName + ".zip", signedOutPutFileName + ".xml");
                    if (scope == DbSyncScope.ALL)
                    {
                        this.dalcDbSync.UploadAllRegistrationsFile(Utils.GetOutputDirPathForConfigKey("SignedOutputFilePath") + timeStamp + signedOutPutFileName + ".zip", Utils.Configuration["ALLFileBlobContainer"], signedOutPutFileName + ".zip");
                    }
                    else
                    {
                        this.dalcDbSync.UploadINCRegistrationsFile(Utils.GetOutputDirPathForConfigKey("SignedOutputFilePath") + timeStamp + signedOutPutFileName + ".zip", Utils.Configuration["INCFileBlobContainer"], signedOutPutFileName + ".zip");
                    }
                }
                else
                {
                    // No Records found case.
                    this.auditor.Audit(AuditId.DBSyncFileGenerated, AuditStatus.Success, 0, "File Service : No Records Found");
                    return "No Records Found";
                }
            }
            catch
            {
                throw;
            }

            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End - SyncManager - GenerateRecordEnsemble scope :" + scope);
            return signedOutputFilePath;
        }

        /// <summary>
        /// Retrieves DBA's information
        /// </summary>
        /// <returns> DB Admin Info Array</returns>
        public DBAdminInfo[] GetDBAdmininfo()
        {
            return this.dalcDbSync.GetDBAdmins();
        }

        /// <summary>
        /// parses the response xml received as poll response.
        /// </summary>
        /// <param name="adminInfo"> Admin Info </param>
        public void UpdateDBAdminInfo(DBAdminInfo adminInfo)
        {
            this.dalcDbSync.UpdateDBAdminInfo(adminInfo);
        }

        /// <summary>
        /// Gets DB sync transaction id for the scope.
        /// </summary>
        /// <param name="scope">scope of the DB Sync</param>
        /// <returns> Next Transaction ID </returns>
        public NextTransactionID GetTransactionIdInfo(DbSyncScope scope)
        {
            return this.dalcDbSync.GetRequestorTransactionId(Utils.Configuration[Constants.ConfigSettingWSDBAName], scope);
        }

        /// <summary>
        /// Validates the xml signature of the xml document.
        /// </summary>
        /// <param name="doc"> xml document containing the xml signature to be validated</param>
        /// <param name="key">public key of the administrator</param>
        /// <returns> Returns boolean </returns>
        public bool IsXMLSignatureValid(XmlDocument doc, string key)
        {
            bool success = false;
            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin - SyncManager - IsXMLSignatureValid - Xml Documents signature verification");
            try
            {
                success = XmlCryptography.IsXmlSignValid(doc, key);
            }
            catch (CryptographicException e)
            {
                this.logger.Log(TraceEventType.Critical, LoggingMessageId.DBSyncPollerGenericMessage, e.ToString());
            }

            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End - SyncManager - IsXMLSignatureValid - Xml Documents signature verification");
            return success;
        }

        /// <summary>
        /// parses the response xml received as poll response.
        /// </summary>
        /// <param name="adminName">Indicates admin name </param>
        /// <param name="pollResponse">response xml string</param>
        /// <param name="key">public key of the administrator</param>
        private void ParseResponseXML(string adminName, string pollResponse, string key)
        {
            string wsdba = null;
            LPAuxRegistration[] lpauxRegistrations = null;
            MVPDRegistration[] mvpdRegistrations = null;
            TempBASRegistration[] tempBasRegistrations = null;
            FixedTVBDRegistration[] fixedTVBDRegistrations = null;
            TVReceiveSiteRegistration[] tvreceiveSiteRegistrtions = null;

            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", Begin - dbSyncManager ParseResponseXML()");
            XmlDocument xmlDoc = new XmlDocument();
            xmlDoc.PreserveWhitespace = true;
            xmlDoc.LoadXml(pollResponse);

            XmlNodeList regNodes = null;
            XmlNamespaceManager namespaceManager = new XmlNamespaceManager(xmlDoc.NameTable);
            namespaceManager.AddNamespace("vcard", Constants.VCardXmlns);
            namespaceManager.AddNamespace("ical", Constants.ICalXmlns);
            namespaceManager.AddNamespace("gml", Constants.GMLXmlns);
            namespaceManager.AddNamespace("ren", xmlDoc.DocumentElement.NamespaceURI);

            if (!this.IsXMLSignatureValid(xmlDoc, key))
            {
                this.logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", SyncManager.ParseResponseXML - XML signature mismatched and not verified with the certificate. please check the xml file received.");
                throw new CryptographicException("Xml signature is not matching.");
            }
            else if (!this.IsXmlValidWithSchema(xmlDoc))
            {
                this.logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", SyncManager.ParseResponseXML - XML file is invalid. please check the xml file received.");
                throw new XmlException("Xml file schema validaion failed.");
            }
            else
            {
                try
                {
                    this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", Begin " + "SyncManager.ParseResponseXML - LP-AuxRegistrations serialization Started");
                    wsdba = xmlDoc.SelectSingleNode("//ren:RegistrationRecordEnsemble/ren:EnsembleDescription/ren:Registrar", namespaceManager).InnerText;

                    // lpAuxRegistrations processing.
                    regNodes = xmlDoc.SelectNodes("//ren:Registration[ren:registrationType='LP-Aux_Registration']", namespaceManager);
                    lpauxRegistrations = new LPAuxRegistration[regNodes.Count];
                    if (regNodes != null && regNodes.Count > 0)
                    {
                        lpauxRegistrations = this.BuildLPAuxRegistrations(regNodes, wsdba, namespaceManager);
                    }

                    this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", End " + "SyncManager.ParseResponseXML - LP-AuxRegistrations serialization Completed");

                    // MVPD Registrations processing
                    this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", Begin " + "SyncManager.ParseResponseXML - MVPD Registration serialization Started");
                    regNodes = null;
                    regNodes = xmlDoc.SelectNodes("//ren:Registration[ren:registrationType='MVPD_Registration']", namespaceManager);
                    mvpdRegistrations = new MVPDRegistration[regNodes.Count];
                    if (regNodes != null && regNodes.Count > 0)
                    {
                        mvpdRegistrations = this.BuildMVPDRegistrations(regNodes, wsdba, namespaceManager);
                    }

                    this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", End " + "SyncManager.ParseResponseXML - MVPD Registration serialization completed");

                    // Temp Bas Registraions Processing
                    this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", Begin " + "SyncManager.ParseResponseXML - Temp Bas Registration serialization Started");
                    regNodes = null;
                    regNodes = xmlDoc.SelectNodes("//ren:Registration[ren:registrationType='Temp_BAS_Registration']", namespaceManager);
                    tempBasRegistrations = new TempBASRegistration[regNodes.Count];
                    if (regNodes != null && regNodes.Count > 0)
                    {
                        tempBasRegistrations = this.BuildTempBASRegistration(regNodes, wsdba, namespaceManager);
                    }

                    this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", End " + "SyncManager.ParseResponseXML - Temp Bas Registration serialization Completed");

                    // Fixed TVBD Registraions Processing
                    this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", Begin " + "SyncManager.ParseResponseXML - Fixed TVBD Registration serialization Started");
                    regNodes = null;
                    regNodes = xmlDoc.SelectNodes("//ren:Registration[ren:registrationType='Fixed_TVBD_Registration']", namespaceManager);
                    fixedTVBDRegistrations = new FixedTVBDRegistration[regNodes.Count];
                    if (regNodes != null && regNodes.Count > 0)
                    {
                        fixedTVBDRegistrations = this.BuildFixedTVBDRegistrations(regNodes, wsdba, namespaceManager);
                    }

                    this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", End" + "SyncManager.ParseResponseXML - Fixed TVBD Registration serialization Completed");

                    // TV Receive Site Registrations proceswsing
                    this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", Begin " + "SyncManager.ParseResponseXML - TV Receive Site Registration serialization Started");
                    regNodes = null;
                    regNodes = xmlDoc.SelectNodes("//ren:Registration[ren:registrationType='TV_Receive_Site_Registration']", namespaceManager);
                    tvreceiveSiteRegistrtions = new TVReceiveSiteRegistration[regNodes.Count];
                    if (regNodes != null && regNodes.Count > 0)
                    {
                        tvreceiveSiteRegistrtions = this.BuildTVSiteReceiveRegistrations(regNodes, wsdba, namespaceManager);
                    }

                    this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", Begin " + "SyncManager.ParseResponseXML - TV Receive Site Registration serialization Completed");
                }
                catch
                {
                    throw;
                }
            }

            try
            {
                this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", Begin " + "dalcDBSync.Update - Registration saving to azure table started");
                this.dalcDbSync.Update(adminName, fixedTVBDRegistrations, lpauxRegistrations, mvpdRegistrations, tvreceiveSiteRegistrtions, tempBasRegistrations, null);
                this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + adminName + ", End " + "dalcDBSync.Update - Registration saving to azure table completed");
            }
            catch (StorageException e)
            {
                this.logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, "dalcDbSync " + "dalcDBSync.Update - Registration saving to azure table got exception :" + e.ToString());
                this.auditor.Audit(AuditId.DBSyncPollRequest, AuditStatus.Failure, 0, "dalcDbSync " + "dalcDBSync.Update - Registration saving to azure table got exception :" + e.ToString());
            }
        }
        
        /// <summary>
        /// Validates the Response XML
        /// </summary>
        /// <param name="doc">xml document object to be validated</param>
        /// <returns> Returns boolean </returns>
        private bool IsXmlValidWithSchema(XmlDocument doc)
        {
            bool success = false;
            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin - SyncManager - IsXmlValidWithSchema - Xml Document's Schema Validation");
            try
            {
                success = XmlSchemaValidator.ValidatePollResponseXMLWithSchema(doc, Utils.GetFilePathForConfigKey("RegistrationsXSDFilePath"));
            }
            catch (Exception e)
            {
                this.logger.Log(TraceEventType.Critical, LoggingMessageId.DBSyncPollerGenericMessage, e.ToString());
            }

            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End - SyncManager - IsXmlValidWithSchema - Xml Document's Schema Validation");
            return success;
        }

        /// <summary>
        ///  Builds LP-AUXRegistrations Array
        /// </summary>
        /// <param name="xmlNodeList"> xml node list</param>
        /// <param name="wsdba">White Space DBA Name</param>
        /// <param name="namespacemanager">XML name space manager</param>
        /// <returns>LPAuxRegistration array</returns>
        private LPAuxRegistration[] BuildLPAuxRegistrations(XmlNodeList xmlNodeList, string wsdba, XmlNamespaceManager namespacemanager)
        {
            LPAuxRegistration[] lpauxRegistrations;
            try
            {
                // Lp-Aux Registrations processing
                this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin " + "SyncManager.ParseResponseXML - LPAuxRegistration serialization ");
                lpauxRegistrations = new LPAuxRegistration[xmlNodeList.Count];
                for (int regindex = 0; regindex < xmlNodeList.Count; regindex++)
                {
                    LPAuxRegistration lpauxRegn = new LPAuxRegistration();
                    this.regbuilder.BuildEntity(lpauxRegn, xmlNodeList[regindex], namespacemanager);
                    lpauxRegn.WSDBA = wsdba;
                    lpauxRegn.Timestamp = DateTime.Now;
                    lpauxRegn.RowKey = lpauxRegn.Disposition.RegId;
                    lpauxRegn.PartitionKey = lpauxRegn.WSDBA;
                    lpauxRegn.SerializeObjectsToJston();
                    lpauxRegistrations[regindex] = lpauxRegn;
                }
            }
            catch
            {
                throw;
            }

            return lpauxRegistrations;
        }

        /// <summary>
        ///  Builds MVPD Registration Array
        /// </summary>
        /// <param name="xmlNodeList"> xml node list</param>
        /// <param name="wsdba">White Space DBA Name</param>
        /// <param name="namespacemanager">XML name space manager</param>
        /// <returns>MVPDRegistration array</returns>
        private MVPDRegistration[] BuildMVPDRegistrations(XmlNodeList xmlNodeList, string wsdba, XmlNamespaceManager namespacemanager)
        {
            MVPDRegistration[] mvpdRegistrations;
            try
            {
                // MVPVD Registrations processing
                this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin " + "SyncManager.ParseResponseXML - MVPDRegistrations serialization Started");
                mvpdRegistrations = new MVPDRegistration[xmlNodeList.Count];
                for (int regindex = 0; regindex < xmlNodeList.Count; regindex++)
                {
                    MVPDRegistration mvpdRegn = new MVPDRegistration();
                    this.regbuilder.BuildEntity(mvpdRegn, xmlNodeList[regindex], namespacemanager);
                    mvpdRegn.WSDBA = wsdba; // xmlNodeList.SelectSingleNode("//ren:RegistrationRecordEnsemble/ren:EnsembleDescription/ren:Registrar", namespaceManager).InnerText;
                    mvpdRegn.Timestamp = DateTime.Now;
                    mvpdRegn.RowKey = mvpdRegn.Disposition.RegId;
                    mvpdRegn.PartitionKey = mvpdRegn.WSDBA;
                    mvpdRegn.SerializeObjectsToJston();
                    mvpdRegistrations[regindex] = mvpdRegn;
                }
            }
            catch
            {
                throw;
            }

            return mvpdRegistrations;
        }

        /// <summary>
        ///  Builds Temp Bas Registration Array
        /// </summary>
        /// <param name="xmlNodeList"> xml node list</param>
        /// <param name="wsdba">White Space DBA Name</param>
        /// <param name="namespacemanager">XML name space manager</param>
        /// <returns>TempoBasRegistration array</returns>
        private TempBASRegistration[] BuildTempBASRegistration(XmlNodeList xmlNodeList, string wsdba, XmlNamespaceManager namespacemanager)
        {
            TempBASRegistration[] tempBasRegistrations;
            try
            {
                // Temp Bas Registrations processing
                this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin " + "SyncManager.ParseResponseXML - TempBasRegistrations serialization Started");
                tempBasRegistrations = new TempBASRegistration[xmlNodeList.Count];
                for (int regindex = 0; regindex < xmlNodeList.Count; regindex++)
                {
                    TempBASRegistration tempBasRegn = new TempBASRegistration();
                    this.regbuilder.BuildEntity(tempBasRegn, xmlNodeList[regindex], namespacemanager);
                    tempBasRegn.WSDBA = wsdba;
                    tempBasRegn.Timestamp = DateTime.Now;
                    tempBasRegn.RowKey = tempBasRegn.Disposition.RegId;
                    tempBasRegn.PartitionKey = tempBasRegn.WSDBA;
                    tempBasRegn.SerializeObjectsToJston();
                    tempBasRegistrations[regindex] = tempBasRegn;
                }
            }
            catch
            {
                throw;
            }

            return tempBasRegistrations;
        }

        /// <summary>
        ///  Builds Fixed TVBD Registration Array
        /// </summary>
        /// <param name="xmlNodeList"> xml node list</param>
        /// <param name="wsdba">White Space DBA Name</param>
        /// <param name="namespacemanager">XML name space manager</param>
        /// <returns>FixedTVBDRegistration array</returns>
        private FixedTVBDRegistration[] BuildFixedTVBDRegistrations(XmlNodeList xmlNodeList, string wsdba, XmlNamespaceManager namespacemanager)
        {
            FixedTVBDRegistration[] fixedTVBDRegistrations;
            try
            {
                // Fixed TVBD Registrations processing
                this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin " + "SyncManager.ParseResponseXML - FixedTVBDRegistration serialization.");
                fixedTVBDRegistrations = new FixedTVBDRegistration[xmlNodeList.Count];
                for (int regindex = 0; regindex < xmlNodeList.Count; regindex++)
                {
                    FixedTVBDRegistration fixedTVBDBasRegn = new FixedTVBDRegistration();
                    this.regbuilder.BuildEntity(fixedTVBDBasRegn, xmlNodeList[regindex], namespacemanager);
                    fixedTVBDBasRegn.WSDBA = wsdba;
                    fixedTVBDBasRegn.Timestamp = DateTime.Now;
                    fixedTVBDBasRegn.RowKey = fixedTVBDBasRegn.Disposition.RegId;
                    fixedTVBDBasRegn.PartitionKey = fixedTVBDBasRegn.WSDBA;
                    fixedTVBDBasRegn.SerializeObjectsToJston();
                    fixedTVBDRegistrations[regindex] = fixedTVBDBasRegn;
                }
            }
            catch
            {
                throw;
            }

            return fixedTVBDRegistrations;
        }

        /// <summary>
        ///  Builds TV Receive Site Registration Array
        /// </summary>
        /// <param name="xmlNodeList"> xml node list</param>
        /// <param name="wsdba">White Space DBA Name</param>
        /// <param name="namespacemanager">XML name space manager</param>
        /// <returns>TVReceiveSiteRegistration array</returns>
        private TVReceiveSiteRegistration[] BuildTVSiteReceiveRegistrations(XmlNodeList xmlNodeList, string wsdba, XmlNamespaceManager namespacemanager)
        {
            TVReceiveSiteRegistration[] tvreceiveSiteRegistrations;
            try
            {
                // Tv Receive Site Registrations processing
                this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin " + "SyncManager.ParseResponseXML - TVReceiveSiteRegistration serialization");
                tvreceiveSiteRegistrations = new TVReceiveSiteRegistration[xmlNodeList.Count];
                for (int regindex = 0; regindex < xmlNodeList.Count; regindex++)
                {
                    TVReceiveSiteRegistration tvreceiveSiteRegn = new TVReceiveSiteRegistration();
                    this.regbuilder.BuildEntity(tvreceiveSiteRegn, xmlNodeList[regindex], namespacemanager);
                    tvreceiveSiteRegn.WSDBA = wsdba;
                    tvreceiveSiteRegn.Timestamp = DateTime.Now;
                    tvreceiveSiteRegn.RowKey = tvreceiveSiteRegn.Disposition.RegId;
                    tvreceiveSiteRegn.PartitionKey = tvreceiveSiteRegn.WSDBA;
                    tvreceiveSiteRegn.SerializeObjectsToJston();
                    tvreceiveSiteRegistrations[regindex] = tvreceiveSiteRegn;
                }
            }
            catch
            {
                throw;
            }

            return tvreceiveSiteRegistrations;
        }

        /// <summary>
        /// Compress the file
        /// </summary>
        /// <param name="filePath">target path</param>
        /// <param name="zipFileName">compressed file path</param>
        /// <param name="fileName">file name</param>
        private void CreateZipFile(string filePath, string zipFileName, string fileName)
        {
            using (ZipArchive archive = ZipFile.Open(zipFileName, ZipArchiveMode.Create))
            {
                archive.CreateEntryFromFile(filePath, fileName);
            }
        }

        /// <summary>
        /// Compress the file
        /// </summary>
        /// <param name="date">date to be converted to Z</param>
        /// <returns>Date Z</returns>
        private string BuildDateZ(DateTime date)
        {
            string month = date.Month.ToString();
            string day = date.Day.ToString();
            string hour = date.Hour.ToString();
            string min = date.Minute.ToString();
            string second = date.Second.ToString();

            if (month.Length == 1)
            {
                month = "0" + month;
            }

            if (day.Length == 1)
            {
                day = "0" + day;
            }

            if (hour.Length == 1)
            {
                hour = "0" + hour;
            }

            if (min.Length == 1)
            {
                min = "0" + min;
            }

            if (second.Length == 1)
            {
                second = "0" + second;
            }

            string datetimez = date.Year.ToString() + "-" + month + "-" + day + "T" + hour + ":" + min + ":" + second + "Z";
            return datetimez;
        }

        /// <summary>
        /// Generate the XML file of Registrations
        /// </summary>
        /// <param name="scope">Scope of the registration (All/Increment)</param>
        /// <param name="requestedTransactionId">Next Transaction ID</param>
        ///  <param name="requester"> DB Sync requester component</param>
        /// <returns>Xml Document</returns>
        private XmlDocument BuildRecordEnsumble(DbSyncScope scope, NextTransactionID requestedTransactionId, string requester)
        {
            FixedTVBDRegistration[] fixedTVBDRegistrations = null;
            LPAuxRegistration[] lpauxRegistrations = null;
            MVPDRegistration[] mvpdRegistrations = null;
            TempBASRegistration[] tempBasRegistrations = null;
            TVReceiveSiteRegistration[] tvreceiveSiteRegistrations = null;
            string wsdbaNameForFile = Utils.Configuration[Constants.ConfigSettingWSDBAName];
            string generationDateTime = null;
            string recordsFrom = null;
            string recordsTo = null;

            DateTime dt = DateTime.Now;
            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + wsdbaNameForFile + ", Begin - dbSyncManager BuildRecordEnsumble() Retrieving data Scope:" + scope + ", RequestedTransactionID:" + requestedTransactionId);
            if (scope == DbSyncScope.ALL)
            {
                // All File
                this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + wsdbaNameForFile + ", Begin - dbSyncManager get all registrations from Azure table");
                fixedTVBDRegistrations = this.dalcDbSync.GetFixedTVBDRegistration(wsdbaNameForFile, null);
                mvpdRegistrations = this.dalcDbSync.GetMVPDRegistration(wsdbaNameForFile, null);
                lpauxRegistrations = this.dalcDbSync.GetLPAuxRegistration(wsdbaNameForFile, null);
                tempBasRegistrations = this.dalcDbSync.GetTempBASRegistration(wsdbaNameForFile, null);
                tvreceiveSiteRegistrations = this.dalcDbSync.GetTVReceiveSiteRegistration(wsdbaNameForFile, null);
            }
            else
            {
                if (requestedTransactionId != null)
                {
                    this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + wsdbaNameForFile + ", Begin - dbSyncManager get Incrimental registrations from Azure table");
                    recordsFrom = this.BuildDateZ(requestedTransactionId.NextTransactionIdDateTime);
                    DateTime dtfrom = Convert.ToDateTime(requestedTransactionId.NextTransactionIdDateTime);
                    fixedTVBDRegistrations = this.dalcDbSync.GetFixedTVBDRegistration(wsdbaNameForFile, dtfrom, dt);
                    mvpdRegistrations = this.dalcDbSync.GetMVPDRegistration(wsdbaNameForFile, dtfrom, dt);
                    lpauxRegistrations = this.dalcDbSync.GetLPAuxRegistration(wsdbaNameForFile, dtfrom, dt);
                    tempBasRegistrations = this.dalcDbSync.GetTempBASRegistration(wsdbaNameForFile, dtfrom, dt);
                    tvreceiveSiteRegistrations = this.dalcDbSync.GetTVReceiveSiteRegistration(wsdbaNameForFile, dtfrom, dt);
                }
            }

            NextTransactionID nextTransactionId = new NextTransactionID();

            //// updating existing transactionid for file incemental update when called from Incremental File
            if ((scope == DbSyncScope.INC) && (requester == wsdbaNameForFile))
            {
                nextTransactionId = requestedTransactionId;
                requestedTransactionId.Timestamp = dt;
                requestedTransactionId.NextTransactionId = Guid.NewGuid().ToString();
                requestedTransactionId.NextTransactionIdDateTime = dt;
                this.dalcDbSync.SaveNextTransactionIdInfo(nextTransactionId);
            }
            else
            {
                ////  create new transaction ID remaining cases like File All, Incremental Web Service.
                nextTransactionId.PartitionKey = scope.ToString();
                nextTransactionId.RowKey = Guid.NewGuid().ToString();
                nextTransactionId.Timestamp = dt;
                nextTransactionId.NextTransactionId = Guid.NewGuid().ToString();
                nextTransactionId.NextTransactionIdDateTime = dt;

                if (requester == wsdbaNameForFile)
                {
                    nextTransactionId.WSDBA = wsdbaNameForFile;
                }
                else
                {
                    nextTransactionId.WSDBA = string.Empty;
                }

                this.dalcDbSync.SaveNextTransactionIdInfo(nextTransactionId);
            }

            // ToDo: save value to azure table.
            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + wsdbaNameForFile + ", Begin - dbSyncManager:BuildRecordEnsumble generating XML file.");

            // loading out bound XML template 
            XmlDocument xmlDocTemplate = new XmlDocument();
            xmlDocTemplate.Load(Utils.GetFilePathForConfigKey("OutBoundXMLTemplate"));

            // Actual File to be generated with default root node
            XmlDocument xmlDocAll = new XmlDocument();
            xmlDocAll.Load(Utils.GetFilePathForConfigKey("RegistrationRecordEnsembleFile"));

            XmlNamespaceManager namespaceManager = new XmlNamespaceManager(xmlDocTemplate.NameTable);
            namespaceManager.AddNamespace("vcard", Constants.VCardXmlns);
            namespaceManager.AddNamespace("ical", Constants.ICalXmlns);
            namespaceManager.AddNamespace("gml", Constants.GMLXmlns);
            namespaceManager.AddNamespace("ren", xmlDocTemplate.DocumentElement.NamespaceURI);
            generationDateTime = this.BuildDateZ(DateTime.Now);

            if (scope == DbSyncScope.ALL)
            {
                recordsFrom = Utils.Configuration["RegistrationsStartDate"];
            }

            recordsTo = generationDateTime;

            // setting the Ensemble details
            XmlNode node = xmlDocAll.SelectSingleNode("//ren:EnsembleDescription", namespaceManager);
            node.SelectSingleNode("//ren:Registrar", namespaceManager).InnerText = Utils.Configuration["Registrar"];
            node.SelectSingleNode("//ren:GenerationDate", namespaceManager).InnerText = generationDateTime;
            node.SelectSingleNode("//ren:Scope", namespaceManager).InnerText = scope.ToString();
            node.SelectSingleNode("//ren:RecordsFrom", namespaceManager).InnerText = recordsFrom;
            node.SelectSingleNode("//ren:RecordsTo", namespaceManager).InnerText = recordsTo;

            // fixed tvbd registrations
            XmlNode xnode = null;
            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + wsdbaNameForFile + ", Begin - dbSyncManager:BuildRecordEnsumble add Fixed_TVBD registration information to XML file.");
            for (int index = 0; index < fixedTVBDRegistrations.Length; index++)
            {
                this.registrationExist = true;
                node = xmlDocTemplate.SelectSingleNode("//ren:Registration[ren:registrationType='Fixed_TVBD_Registration']", namespaceManager).CloneNode(true);
                RegistrationXmlNodeBuilder.BuildFixedTVBDRegistrationXmlNode(node, fixedTVBDRegistrations[index], namespaceManager);
                xnode = xmlDocAll.CreateNode(XmlNodeType.Element, "n1:Registration", xmlDocAll.DocumentElement.NamespaceURI);
                xnode.InnerXml = node.InnerXml;
                xmlDocAll.DocumentElement.AppendChild(xnode);
            }

            // LPAux registrations
            xnode = null;
            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + wsdbaNameForFile + ", Begin - dbSyncManager:BuildRecordEnsumble add LP-Aux registration information to XML file.");
            for (int index = 0; index < lpauxRegistrations.Length; index++)
            {
                this.registrationExist = true;
                node = xmlDocTemplate.SelectSingleNode("//ren:Registration[ren:registrationType='LP-Aux_Registration']", namespaceManager).CloneNode(true);
                RegistrationXmlNodeBuilder.BuildLPAUXRegistrationXmlNode(node, lpauxRegistrations[index], namespaceManager);
                xnode = xmlDocAll.CreateNode(XmlNodeType.Element, "n1:Registration", xmlDocAll.DocumentElement.NamespaceURI);
                xnode.InnerXml = node.InnerXml;
                xmlDocAll.DocumentElement.AppendChild(xnode);
            }

            // MVPD registrations
            xnode = null;
            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + wsdbaNameForFile + ", Begin - dbSyncManager:BuildRecordEnsumble add MVPD registration information to XML file.");
            for (int index = 0; index < mvpdRegistrations.Length; index++)
            {
                this.registrationExist = true;
                node = xmlDocTemplate.SelectSingleNode("//ren:Registration[ren:registrationType='MVPD_Registration']", namespaceManager).CloneNode(true);
                RegistrationXmlNodeBuilder.BuildMVPDRegistrationXmlNode(node, mvpdRegistrations[index], namespaceManager);
                xnode = xmlDocAll.CreateNode(XmlNodeType.Element, "n1:Registration", xmlDocAll.DocumentElement.NamespaceURI);
                xnode.InnerXml = node.InnerXml;
                xmlDocAll.DocumentElement.AppendChild(xnode);
            }

            // Temp Bas registrations
            xnode = null;
            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + wsdbaNameForFile + ", Begin - dbSyncManager:BuildRecordEnsumble add Temp_BAS registration information to XML file.");
            for (int index = 0; index < tempBasRegistrations.Length; index++)
            {
                this.registrationExist = true;
                node = xmlDocTemplate.SelectSingleNode("//ren:Registration[ren:registrationType='Temp_BAS_Registration']", namespaceManager).CloneNode(true);
                RegistrationXmlNodeBuilder.BuildTBasRegistrationXmlNode(node, tempBasRegistrations[index], namespaceManager);
                xnode = xmlDocAll.CreateNode(XmlNodeType.Element, "n1:Registration", xmlDocAll.DocumentElement.NamespaceURI);
                xnode.InnerXml = node.InnerXml;
                xmlDocAll.DocumentElement.AppendChild(xnode);
            }

            // TV Receive Site registrations
            xnode = null;
            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + wsdbaNameForFile + ", Begin - dbSyncManager:BuildRecordEnsumble add TV_Receive_Site registration information to XML file.");
            for (int index = 0; index < tvreceiveSiteRegistrations.Length; index++)
            {
                this.registrationExist = true;
                node = xmlDocTemplate.SelectSingleNode("//ren:Registration[ren:registrationType='TV_Receive_Site_Registration']", namespaceManager).CloneNode(true);
                RegistrationXmlNodeBuilder.BuildTVReceiveSiteRegistrationXmlNode(node, tvreceiveSiteRegistrations[index], namespaceManager);
                xnode = xmlDocAll.CreateNode(XmlNodeType.Element, "n1:Registration", xmlDocAll.DocumentElement.NamespaceURI);
                xnode.InnerXml = node.InnerXml;
                xmlDocAll.DocumentElement.AppendChild(xnode);
            }

            xnode = xmlDocAll.CreateNode(XmlNodeType.Element, "n1:NextTransactionID", xmlDocAll.DocumentElement.NamespaceURI);
            xnode.InnerXml = nextTransactionId.NextTransactionId;
            xmlDocAll.DocumentElement.AppendChild(xnode);
            this.logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:" + wsdbaNameForFile + ", End - dbSyncManager BuildRecordEnsumble()");

            return xmlDocAll;
        }

        /// <summary>
        ///  validates xml file for signature and schema
        /// </summary>
        /// <param name="xmlDoc">xml document to be verified</param>
        private void VerifyRecordEnsembleGenerated(XmlDocument xmlDoc)
        {
            if (!XmlCryptography.IsXmlSignValidByFile(xmlDoc, Utils.GetFilePathForConfigKey("PrivateCertificate")))
            {
                this.logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:, SyncManager.VerifyRecordEnsembleGenerated - XML signature mismatched and not verified with the certificate. please check the xml file received.");
                throw new CryptographicException("Xml signature is not matching.");
            }
            else if (!this.IsXmlValidWithSchema(xmlDoc))
            {
                this.logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncPollerGenericMessage, "WSDBA:, SyncManager.VerifyRecordEnsembleGenerated - XML file is invalid. please check the xml file received.");
                throw new XmlException("Xml file schema validaion failed.");
            }
        }
    }
}
