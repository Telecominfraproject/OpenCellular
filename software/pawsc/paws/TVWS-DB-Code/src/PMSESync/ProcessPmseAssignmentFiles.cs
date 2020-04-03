// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.PmseSync
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Security.Cryptography;
    using System.Security.Cryptography.Pkcs;
    using System.Security.Cryptography.X509Certificates;
    using Microsoft.Exchange.WebServices.Data;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Class ProcessPMSEAssignmentFiles.
    /// </summary>
    public class ProcessPmseAssignmentFiles : IPmseAssignment
    {
        /// <summary>
        /// The stop watch
        /// </summary>
        private Stopwatch stopWatch;

        /// <summary>Gets or sets IUserManager Interface</summary>
        [Dependency]
        public IDalcPmseSync PmseSync { get; set; }

        /// <summary>
        /// Gets or sets the common DALC.
        /// </summary>
        /// <value>The common DALC.</value>
        [Dependency]
        public IDalcCommon CommonDalc { get; set; }

        /// <summary>
        /// Gets or sets IAuditor Interface
        /// </summary>
        [Dependency]
        public IAuditor PmseAssignmentAuditor { get; set; }

        /// <summary>
        /// Gets or sets ILogger Interface
        /// </summary>
        [Dependency]
        public ILogger PmseAssignmentLogger { get; set; }

        /// <summary>
        /// Gets or sets the land cover data.
        /// </summary>
        /// <value>The land cover data.</value>
        [Dependency]
        public IClutterDatasetReader TerrainData { get; set; }

        /// <summary>
        /// Synchronizes the database.
        /// </summary>
        public void SyncDB()
        {
            this.PmseAssignmentLogger.Log(TraceEventType.Information, LoggingMessageId.PmseSyncGenericMessage, "Start - Parsing PmseSync the files");

            try
            {
                this.ProcessDirectory();
            }
            catch (Exception ex)
            {
                this.PmseAssignmentAuditor.Audit(AuditId.PmseSync, AuditStatus.Failure, 0, ex.ToString());
            }

            this.PmseAssignmentLogger.Log(TraceEventType.Information, LoggingMessageId.PmseSyncGenericMessage, "End - Parsing PmseSync the files");
        }

        /// <summary>
        /// Processes the directory.
        /// </summary>
        public void ProcessDirectory()
        {
            long elapsedTime = 0;
            //// AuditId auditId = AuditId.PmseSync;
            //// string auditMethodName = "ProcessDirectory";
            ExchangeService service = new ExchangeService(ExchangeVersion.Exchange2010_SP2);
            service.Credentials = new NetworkCredential(Utils.Configuration["Email"], Utils.Configuration["Passcode"]);
            service.Url = new Uri(Utils.Configuration["ExchangeURI"]);

            SearchFilter searchFilter = new SearchFilter.SearchFilterCollection(LogicalOperator.And, new SearchFilter.IsEqualTo(EmailMessageSchema.IsRead, false));

            ItemView view = new ItemView(100);
            FindItemsResults<Item> findResults = service.FindItems(WellKnownFolderName.Inbox, searchFilter, view);

            foreach (Item item in findResults.Items.OrderBy(obj => obj.DateTimeReceived))
            {
                this.stopWatch = new Stopwatch();
                this.stopWatch.Start();
                try
                {
                    item.Load();
                    string replyMessage = string.Empty;
                    bool signedEmail = false;
                    if ((item.ItemClass == "IPM.Note.SMIME.MultipartSigned" && item.LastModifiedName == "admin@jfmg.co.uk") || !Convert.ToBoolean(Utils.Configuration["CheckSignedEmail"]))
                    {
                        if (item.ItemClass == "IPM.Note.SMIME.MultipartSigned")
                        {
                            signedEmail = true;
                        }

                        List<PmseAssignment> listPD = new List<PmseAssignment>();
                        List<DTTDataAvailability> listDA = new List<DTTDataAvailability>();

                        if (item.HasAttachments)
                        {
                            if (item.Attachments.Count == 1)
                            {
                                foreach (Attachment fileAttachment in item.Attachments)
                                {
                                    FileAttachment attachment = fileAttachment as FileAttachment;
                                    if (item.Subject.ToLower().Contains("unscheduled adjustment") || item.Subject.ToLower().Contains("pmse-wsd"))
                                    {
                                        var filePath = Path.Combine(Utils.GetLocalStorePath(), "Temp_File.csv");
                                        attachment.Load(filePath);
                                        string fileName = this.ExtractFileNameFromSubject(item.Subject);
                                        bool result = false;
                                        string[] fileData = this.ParseSignedWSDFile(filePath, signedEmail);

                                        if (item.Subject.ToLower().Contains("unscheduled adjustment"))
                                        {
                                            result = this.ProcessUnscheduledAdjustments(fileData, fileName, out replyMessage);
                                            this.stopWatch.Stop();
                                            elapsedTime = this.stopWatch.ElapsedMilliseconds;

                                            // File.Delete(filePath);
                                            if (result)
                                            {
                                                this.PmseSyncStatus(Constants.PmseSync, Constants.UnscheduledAdjustmentsTableName, this.stopWatch.Elapsed);
                                            }
                                        }
                                        else
                                        {
                                            result = this.ProcessWSDAssignmentFile(fileData, fileName, out replyMessage);
                                            this.stopWatch.Stop();
                                            elapsedTime = this.stopWatch.ElapsedMilliseconds;

                                            // File.Delete(filePath);
                                            if (result)
                                            {
                                                this.PmseSyncStatus(Constants.PMSEAssignmentsTable, Constants.PMSEAssignmentsTable, this.stopWatch.Elapsed);
                                            }
                                        }

                                        string logMessage = string.Format("File {0} data has been successfully synchronized with WSDB. Synchronization Time:{1}", fileName, this.stopWatch.Elapsed);
                                        this.PmseAssignmentLogger.Log(TraceEventType.Information, LoggingMessageId.PmseSyncGenericMessage, logMessage);
                                    }
                                    else
                                    {
                                        replyMessage = Utils.Configuration["mailBodySubjectNotValid"];
                                    }
                                }
                            }
                            else
                            {
                                replyMessage = Utils.Configuration["mailBodyMoreThanOneAttachments"];
                                this.PmseAssignmentLogger.Log(TraceEventType.Information, LoggingMessageId.PmseSyncGenericMessage, replyMessage);
                            }
                        }
                        else
                        {
                            replyMessage = Utils.Configuration["mailBodyAttachmentNotFound"];
                            this.PmseAssignmentLogger.Log(TraceEventType.Information, LoggingMessageId.PmseSyncGenericMessage, replyMessage);
                        }
                    }
                    else
                    {
                        replyMessage = Utils.Configuration["mailBodyUnsignedMail"];
                    }

                    EmailMessage message = EmailMessage.Bind(service, item.Id);
                    ////bool replyToAll = false;
                    ////ResponseMessage responseMessage = message.CreateReply(replyToAll);
                    ////responseMessage.BodyPrefix = replyMessage;
                    ////responseMessage.CcRecipients.Add(message.From.Address);
                    ////responseMessage.SendAndSaveCopy();
                    bool replyToAll = false;
                    ResponseMessage responseMessage = message.CreateReply(replyToAll);
                    responseMessage.ToRecipients.Add(Utils.Configuration["ofcomEmailId"]);
                    responseMessage.BodyPrefix = replyMessage;
                    responseMessage.SendAndSaveCopy();
                    message.IsRead = true;
                    message.Update(ConflictResolutionMode.AutoResolve);
                }
                catch (Exception ex)
                {
                    this.PmseAssignmentLogger.Log(TraceEventType.Error, LoggingMessageId.PmseSyncGenericMessage, "Exception Occured :- " + ex.ToString());
                    continue;
                }
            }

            try
            {
                List<DynamicTableEntity> pmseSyncStatusDetails = this.PmseSync.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.PmseSyncStatusTableName), new { RowKey = Constants.UnscheduledAdjustmentsTableName });

                if (pmseSyncStatusDetails.Count > 0)
                {
                    // var lastSyncTime = pmseSyncStatusDetails[0].Timestamp.AddMinutes(Utils.Configuration["UnscheduledAdjustmentTimeMin"].ToInt32() + Utils.Configuration["AcceptableDelayInUnscheduledAdjustmentMin"].ToInt32());
                    // if (lastSyncTime < DateTimeOffset.Now)
                    // {
                    //    this.PmseAssignmentLogger.Log(TraceEventType.Information, LoggingMessageId.PmseSyncGenericMessage, "Sending mail for unscheduled adjustments");
                    //    EmailMessage message1 = new EmailMessage(service);
                    //    message1.From = Utils.Configuration["Email"];
                    //    message1.ToRecipients.Add(Utils.Configuration["ofcomEmailId"]);
                    //    message1.Subject = Utils.Configuration["mailSubjectUnscheduledMailNotFound"];
                    //    message1.Body = Utils.Configuration["mailBodyUnscheduledMailNotFound"];
                    //    message1.SendAndSaveCopy();
                    //    this.PmseAssignmentLogger.Log(TraceEventType.Information, LoggingMessageId.PmseSyncGenericMessage, "Mail sent for unscheduled adjustments");
                    // }
                }
            }
            catch (Exception ex)
            {
                this.PmseAssignmentLogger.Log(TraceEventType.Error, LoggingMessageId.PmseSyncGenericMessage, string.Format("Some exception occured while sending mail for unscheduled adjustments - {0}", ex.ToString()));
            }
        }

        /// <summary>
        /// Checks the type of the equipment.
        /// </summary>
        /// <param name="equipmentID">The equipment identifier.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool CheckEquipmentType(string equipmentID)
        {
            if ("1,2,4,8,16,32,64".Split(',').Contains(equipmentID))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Checks the situation identifier.
        /// </summary>
        /// <param name="situationID">The situation identifier.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool CheckSituationID(string situationID)
        {
            if ("i,e,a".Split(',').Contains(situationID.ToLower()))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Checks the easting northing.
        /// </summary>
        /// <param name="val">The value.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool CheckRequiredInteger(string val)
        {
            if (val != string.Empty)
            {
                try
                {
                    Convert.ToDouble(val);
                    return true;
                }
                catch
                {
                    return false;
                }
            }

            return false;
        }

        public bool CheckFrequency(string val)
        {
            if (val != string.Empty)
            {
                try
                {
                    var frequency = Convert.ToDouble(val);
                    if (frequency >= 470 && frequency <= 790)
                    {
                        return true;
                    }
                }
                catch
                {
                    return false;
                }
            }

            return false;
        }

        public bool CheckUnscheduledData(string[] values)
        {
            try
            {
                var result = values.Skip(2).Select(obj => Convert.ToInt32(obj)).ToArray();
                return true;
            }
            catch
            {
                return false;
            }
        }

        /// <summary>
        /// Parses the signed file.
        /// </summary>
        /// <param name="filePath">The file path.</param>
        /// /// <param name="signed">the e-mail is signed</param>
        /// <returns>returns System.String[].</returns>
        private string[] ParseSignedWSDFile(string filePath, bool signed)
        {
            var csvLines = File.ReadAllLines(filePath);

            if (signed)
            {
                var mainData = csvLines.SkipWhile(obj => !obj.Trim().StartsWith("Assignment_ID")).TakeWhile(obj => !obj.Contains("NextPart")).ToList();
                List<string> mainLines = new List<string>();

                for (int i = 0; i < mainData.Count() - 1; i += 2)
                {
                    if (mainData[i].Contains("Assignment_ID"))
                    {
                        continue;
                    }

                    mainLines.Add((mainData[i] + mainData[i + 1]).Replace("=", string.Empty));
                }

                return mainLines.ToArray();
            }
            else
            {
                //// Email is not signed
                return csvLines;
            }
        }

        /// <summary>
        /// Extracts the file name from subject.
        /// </summary>
        /// <param name="subject">The subject.</param>
        /// <returns>returns System.String.</returns>
        private string ExtractFileNameFromSubject(string subject)
        {
            var firstIndex = subject.IndexOf('<');
            var lastIndex = subject.LastIndexOf('>');
            var fileName = subject.Substring(firstIndex + 1, (lastIndex - firstIndex) - 1);
            return fileName;
        }

        /// <summary>
        /// Processes the file.
        /// </summary>
        /// <param name="fileContent">The path.</param>
        /// <param name="fileName">Name of the file.</param>
        /// <param name="replyMessage">The reply message.</param>
        /// <returns>returns boolean.</returns>
        private bool ProcessWSDAssignmentFile(string[] fileContent, string fileName, out string replyMessage)
        {
            AuditId auditId = AuditId.PmseSync;
            string methodName = "ProcessWSDAssignmentFile";
            try
            {
                // Begin Log transaction
                this.PmseAssignmentLogger.Log(TraceEventType.Information, LoggingMessageId.PmseSyncGenericMessage, "Enter " + methodName + " for file : " + fileName);

                List<PmseAssignment> lstPmseAssignments = new List<PmseAssignment>();
                OSGLocation assignmentLocation;

                foreach (var line in fileContent)
                {
                    string[] values = line.Split(',');
                    if (values[0] == "Assignment_ID")
                    {
                        continue;
                    }

                    try
                    {
                        PmseAssignment pmseAssignment = new PmseAssignment();
                        if (!this.CheckEquipmentType(values[1]) || !this.CheckSituationID(values[9]) || values[0] == string.Empty || !this.CheckRequiredInteger(values[2]) || !this.CheckRequiredInteger(values[3]) || !this.CheckFrequency(values[5]) || !this.CheckRequiredInteger(values[6]) || values[7] == string.Empty || values[8] == string.Empty)
                        {
                            lstPmseAssignments = null;
                            break;
                        }
                        else
                        {
                            assignmentLocation = Conversion.RoundTo10(values[2].ToDouble(), values[3].ToDouble());
                            pmseAssignment.OriginalEasting = values[2].ToDouble();
                            pmseAssignment.OriginalNorthing = values[3].ToDouble();
                            pmseAssignment.PartitionKey = fileName;
                            pmseAssignment.RowKey = values[0];
                            pmseAssignment.Assignment_ID = values[0];
                            pmseAssignment.Equipment_Type_ID = values[1];
                            pmseAssignment.Easting = assignmentLocation.Easting;
                            pmseAssignment.Northing = assignmentLocation.Northing;
                            pmseAssignment.AntennaHeightMetres = values[4].ToDouble();
                            pmseAssignment.FrequencyMHz = values[5].ToDouble();
                            pmseAssignment.BandwidthMHz = values[6].ToDouble();
                            pmseAssignment.Start = values[7];
                            pmseAssignment.Finish = values[8];
                            pmseAssignment.SituationID = values[9];
                            pmseAssignment.Channel = Conversion.DTTFrequencyToChannel(pmseAssignment.FrequencyMHz);
                            pmseAssignment.ClutterValue = (int)this.TerrainData.CalculateClutter(pmseAssignment.Easting, pmseAssignment.Northing);
                            lstPmseAssignments.Add(pmseAssignment);
                        }
                    }
                    catch (Exception)
                    {
                        this.PmseAssignmentLogger.Log(TraceEventType.Error, LoggingMessageId.PmseSyncGenericMessage, string.Format("Error in File:{0}, Line :{1}", fileName, line));
                        lstPmseAssignments = null;
                    }
                }

                if (lstPmseAssignments != null && lstPmseAssignments.Count > 0 && lstPmseAssignments.Select(obj => obj.RowKey).Distinct().Count() == lstPmseAssignments.Count)
                {
                    List<PmseAssignment> existingPMSEData = this.CommonDalc.FetchEntity<PmseAssignment>(Utils.GetRegionalTableName(Constants.PMSEAssignmentsTable), null);
                    if (existingPMSEData.Count > 0)
                    {
                        this.CommonDalc.DeleteRecords(Constants.PMSEAssignmentsTable, existingPMSEData);
                    }

                    this.PmseSync.InsertFileDataToDatabase(lstPmseAssignments);
                    this.PmseAssignmentLogger.Log(TraceEventType.Information, LoggingMessageId.PmseSyncGenericMessage, "Exit " + methodName + " for file : " + fileName);
                    replyMessage = Utils.Configuration["mailBodySyncSuccessfully"];
                    return true;
                }
                else
                {
                    replyMessage = Utils.Configuration["mailBodyFileNotValid"];
                    this.PmseAssignmentLogger.Log(TraceEventType.Error, LoggingMessageId.PmseSyncGenericMessage, "Validation failed while processing file  " + fileName);
                    this.PmseAssignmentAuditor.Audit(auditId, AuditStatus.Failure, this.stopWatch.ElapsedMilliseconds, methodName + "Validation failed while processing file  " + fileName);
                    return false;
                }
            }
            catch (Exception e)
            {
                replyMessage = Utils.Configuration["mailBodyFailToProcessFile"];
                this.PmseAssignmentLogger.Log(TraceEventType.Error, LoggingMessageId.PmseSyncGenericMessage, "Exception Occured while processing file  " + fileName + " Exception :- " + e.ToString());
                this.PmseAssignmentAuditor.Audit(auditId, AuditStatus.Failure, this.stopWatch.ElapsedMilliseconds, methodName + "Exception Occured while processing file  " + fileName + " Exception :- " + e.ToString());
                return false;
            }
        }

        /// <summary>
        /// Processes the unscheduled adjustments.
        /// </summary>
        /// <param name="dataStream">The data stream.</param>
        /// <param name="fileName">Name of the file.</param>
        /// <param name="replyMessage">The reply message.</param>
        /// <returns>returns List{DTTDataAvailability}.</returns>
        private bool ProcessUnscheduledAdjustments(string[] dataStream, string fileName, out string replyMessage)
        {
            AuditId auditId = AuditId.PmseSync;
            string auditMethodName = "ProcessUnscheduledAdjustments";
            try
            {
                List<DTTDataAvailability> listPD = new List<DTTDataAvailability>();
                foreach (var line in dataStream)
                {
                    var values = line.Split(',');
                    if (values[0].ToLower().Contains("easting"))
                    {
                        continue;
                    }

                    try
                    {
                        if (!this.CheckRequiredInteger(values[0]) || !this.CheckRequiredInteger(values[1]) || !this.CheckUnscheduledData(values))
                        {
                            listPD = null;
                            replyMessage = Utils.Configuration["mailBodyFileNotValid"];
                            break;
                        }
                        else
                        {
                            DTTDataAvailability pd = new DTTDataAvailability();
                            var dttvalues = values.Skip(2).Select(obj => obj.ToInt32()).ToArray();
                            pd.DataRecord = Conversion.IntToByte(dttvalues);
                            var test = Conversion.ByteToInt(pd.DataRecord);
                            pd.Easting = values[0].ToInt32();
                            pd.Northing = values[1].ToInt32();
                            pd.PartitionKey = fileName;
                            pd.RowKey = string.Format("{0}-{1}", pd.Easting, pd.Northing);
                            listPD.Add(pd);
                        }
                    }
                    catch
                    {
                        this.PmseAssignmentLogger.Log(TraceEventType.Error, LoggingMessageId.PmseSyncGenericMessage, string.Format("Error in File:{0}, Line :{1}", fileName, line));
                        listPD = null;
                    }
                }

                if (listPD != null && listPD.Count > 0 && listPD.Select(obj => obj.RowKey).Distinct().Count() == listPD.Count)
                {
                    try
                    {
                        bool result = this.PmseSync.InsertPMSEUnscheduledAdjustment(listPD, Utils.GetRegionalTableName(Constants.UnscheduledAdjustmentsTableName));
                        if (result)
                        {
                            List<DTTDataAvailability> existingPMSEUnscheduledData = this.CommonDalc.FetchEntity<DTTDataAvailability>(Utils.GetRegionalTableName(Constants.UnscheduledAdjustmentsTableName), null);
                            if (existingPMSEUnscheduledData.Count > 0)
                            {
                                existingPMSEUnscheduledData = existingPMSEUnscheduledData.Where(obj => obj.PartitionKey != fileName).ToList();
                                this.CommonDalc.DeleteRecords(Constants.UnscheduledAdjustmentsTableName, existingPMSEUnscheduledData);
                            }

                            replyMessage = Utils.Configuration["mailBodySyncSuccessfully"];
                        }
                        else
                        {
                            replyMessage = Utils.Configuration["mailBodyServerError"];
                        }
                    }
                    catch
                    {
                        replyMessage = Utils.Configuration["mailBodyServerError"];
                    }
                }
                else
                {
                    replyMessage = Utils.Configuration["mailBodyFileNotValid"];
                    return false;
                }

                return true;
            }
            catch (Exception e)
            {
                replyMessage = Utils.Configuration["mailBodyFailToProcessFile"];
                this.PmseAssignmentLogger.Log(TraceEventType.Information, LoggingMessageId.PmseSyncGenericMessage, "Exception Occured while processing file  " + fileName + " Exception :- " + e.ToString());
                this.PmseAssignmentAuditor.Audit(auditId, AuditStatus.Failure, this.stopWatch.ElapsedMilliseconds, auditMethodName + "Exception Occured while processing file  " + fileName + " Exception :- " + e.ToString());
                return false;
            }
        }

        /// <summary>
        /// Updates the data cache status.
        /// </summary>
        /// <param name="partitionKey">The partition key.</param>
        /// <param name="rowKey">The row key.</param>
        /// <param name="timeTaken">The time taken.</param>
        private void PmseSyncStatus(string partitionKey, string rowKey, TimeSpan timeTaken)
        {
            DynamicTableEntity pmseSyncDetail = new DynamicTableEntity();
            pmseSyncDetail.PartitionKey = partitionKey;
            pmseSyncDetail.RowKey = rowKey;
            pmseSyncDetail.Properties.Add("status", EntityProperty.GeneratePropertyForInt(1));
            this.PmseSync.InsertEntity(Utils.GetRegionalTableName(Constants.PmseSyncStatusTableName), pmseSyncDetail);
        }
    }
}
