// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Dalc
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// The Primary Data Access Layer Component.
    /// </summary>
    public class AzureDalc : IDalcCommon, IDalcAuditor, IDalcLogger, IDalcDBSync, IDalcIncumbent, IDalcPaws, IDalcRegionSync, IDalcUserManagement, IDalcTerrain, IDalcPMSE, IDalcPmseSync, IDalcDTTAvailabilitySync, IDalcServiceCacheHelper
    {
        /// <summary>
        /// Constant Variable for Audit Table name
        /// </summary>
        private const string AzureAuditTableName = "Audit";

        /// <summary>
        /// Constant Variable for Log Table name
        /// </summary>
        private const string AzureLogTableName = "Log";

        /// <summary>
        /// Constant Variable for Azure Storage Helper
        /// </summary>
        private AzureStorageHelper azureTableOperation;

        /// <summary>
        /// Gets or sets ILogger Interface
        /// </summary>
        [Dependency]
        public ILogger Logger { get; set; }

        /// <summary>
        /// Gets AzureStorageHelper
        /// </summary>
        private AzureStorageHelper AzureStorage
        {
            get
            {
                if (this.azureTableOperation == null)
                {
                    this.azureTableOperation = new AzureStorageHelper(this.Logger);
                }

                return this.azureTableOperation;
            }
        }

        /// <summary>
        /// Writes the specified message to an audit table.
        /// </summary>
        /// <param name="regionCode">The region id.</param>
        /// <param name="id">The type of audit operation that was just completed.</param>
        /// <param name="status">The status of the audit operation.</param>
        /// <param name="message">The message associated with the audit.</param>
        /// <param name="userId">The user Id who initiated the audit.</param>
        /// <param name="transactionId">A unique Id of the audit operation.</param>
        /// <param name="eleapsedTime">The total elapsed time in milliseconds for the audit operation to complete.</param>
        public void WriteMessage(int regionCode, AuditId id, AuditStatus status, string message, string userId, Guid transactionId, long eleapsedTime)
        {
            AuditTableEntity auditTable = new AuditTableEntity();
            auditTable.PartitionKey = regionCode.ToString() + "-" + DateTime.Now.ToString("yyyyMMdd");
            auditTable.RowKey = Guid.NewGuid().ToString();
            auditTable.Timestamp = DateTime.Now;
            auditTable.AuditId = Convert.ToInt32(id);
            auditTable.Status = Convert.ToInt32(status);
            auditTable.ElapsedTime = eleapsedTime;
            auditTable.TransactionID = this.Logger.TransactionId.ToString();
            auditTable.UserID = userId;
            ////auditTable.RegionCode = regionCode;
            auditTable.Message = message;
            try
            {
                this.AzureStorage.InsertCloudStorageEntity(auditTable, AzureAuditTableName);
            }
            catch
            {
                throw;
            }
            finally
            {
                auditTable = null;
            }
        }

        /// <summary>
        /// Writes the specified message to an audit table.
        /// </summary>
        /// <param name="regionCode">The region id.</param>
        /// <param name="logLevel">The type of Log Level set in the config.</param>
        /// <param name="logMsgId">Log message Id</param>
        /// <param name="logMessage">The message associated with the log.</param>
        /// <param name="userId">The user Id who initiated the audit.</param>
        /// <param name="transactionId">A unique Id of the audit operation.</param>
        public void LogMessage(int regionCode, LoggingLevel logLevel, LoggingMessageId logMsgId, string logMessage, string userId, Guid transactionId)
        {
            LogTableEntity logEntity = new LogTableEntity();
            logEntity.PartitionKey = regionCode.ToString() + "-" + DateTime.Now.Year.ToString() + DateTime.Now.Month.ToString() + DateTime.Now.Day.ToString();
            logEntity.RowKey = Guid.NewGuid().ToString();
            logEntity.Timestamp = DateTime.Now;
            logEntity.Id = Convert.ToInt32(logMsgId);
            logEntity.LogDateTime = DateTime.Now;
            logEntity.LogLevel = Convert.ToInt32(logLevel);
            logEntity.TransactionID = this.Logger.TransactionId.ToString();
            logEntity.UserID = userId;
            logEntity.Message = logMessage;
            try
            {
                this.AzureStorage.InsertCloudStorageEntity(logEntity, AzureLogTableName);
            }
            catch
            {
                throw;
            }
            finally
            {
                logEntity = null;
            }
        }

        /// <summary>
        /// Returns all of the fixed TV DB Registrations.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="nextTransactionID">Specifies the TransactionId that is being used for 
        /// filtering the query.</param>
        /// <returns>Returns an array of Fixed TVBD Registrations.</returns>
        public FixedTVBDRegistration[] GetFixedTVBDRegistration(string adminName, string nextTransactionID)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.GetFixedTVBDRegistration");
            TableQuery<FixedTVBDRegistration> query;
            query = new TableQuery<FixedTVBDRegistration>().Where(TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, adminName));
            return this.AzureStorage.FetchEntity<FixedTVBDRegistration>(Utils.CurrentRegionName + Utils.Configuration.CurrentRegionId + Constants.FixedTVBDRegistrationTablename, query).ToArray();
        }

        /// <summary>
        /// Returns all of the fixed TV DB Registrations for the given time
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="fromDate">from date</param>
        /// <param name="toDate">to date</param>
        /// <returns>Fixed TVBD Registrations</returns>
        public FixedTVBDRegistration[] GetFixedTVBDRegistration(string adminName, DateTime fromDate, DateTime toDate)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.GetFixedTVBDRegistration");
            TableQuery<FixedTVBDRegistration> query;
            string filterconditionFromDate = TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.GreaterThanOrEqual, fromDate);
            string filterconditionToDate = TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.LessThanOrEqual, toDate);
            string filterPartitionKey = TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, adminName);
            string finalCondition = TableQuery.CombineFilters(filterconditionFromDate, TableOperators.And, filterconditionToDate);
            finalCondition = TableQuery.CombineFilters(finalCondition, TableOperators.And, filterPartitionKey);
            query = new TableQuery<FixedTVBDRegistration>().Where(finalCondition);
            return this.AzureStorage.FetchEntity<FixedTVBDRegistration>(Utils.CurrentRegionName + Utils.Configuration.CurrentRegionId + Constants.FixedTVBDRegistrationTablename, query).ToArray();
        }

        /// <summary>
        /// Returns all of the LP-Aux Registrations for the given time.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="fromDate">from date</param>
        /// <param name="toDate">to date</param>
        /// <returns>LP Aux registration</returns>
        public LPAuxRegistration[] GetLPAuxRegistration(string adminName, DateTime fromDate, DateTime toDate)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.GetLPAuxRegistration");
            TableQuery<LPAuxRegistration> query;
            string filterconditionFromDate = TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.GreaterThanOrEqual, fromDate);
            string filterconditionToDate = TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.LessThanOrEqual, toDate);
            string filterPartitionKey = TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, adminName);
            string finalCondition = TableQuery.CombineFilters(filterconditionFromDate, TableOperators.And, filterconditionToDate);
            finalCondition = TableQuery.CombineFilters(finalCondition, TableOperators.And, filterPartitionKey);
            query = new TableQuery<LPAuxRegistration>().Where(finalCondition);
            return this.AzureStorage.FetchEntity<LPAuxRegistration>(Utils.GetRegionalTableName(Constants.LPAuxRegistrationTable), query).ToArray();
        }

        /// <summary>
        /// Returns all of the LP Aux Registrations.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="transactionId">Specifies the TransactionId that is being used for 
        /// filtering the query.</param>
        /// <returns>Returns an array of LP Aux Registrations.</returns>
        public LPAuxRegistration[] GetLPAuxRegistration(string adminName = null, string transactionId = null)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.GetLPAuxRegistration");
            TableQuery<LPAuxRegistration> query;
            query = new TableQuery<LPAuxRegistration>().Where(TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, adminName));
            return this.AzureStorage.FetchEntity<LPAuxRegistration>(Utils.GetRegionalTableName(Constants.LPAuxRegistrationTable), query).ToArray();
        }

        /// <summary>
        /// Returns all of the MVPD Registrations for the given time
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="fromDate">from date</param>
        /// <param name="toDate">to date</param>
        /// <returns>MVPD registrations</returns>
        public MVPDRegistration[] GetMVPDRegistration(string adminName, DateTime fromDate, DateTime toDate)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.GetMVPDRegistration");
            TableQuery<MVPDRegistration> query;
            string filterconditionFromDate = TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.GreaterThanOrEqual, fromDate);
            string filterconditionToDate = TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.LessThanOrEqual, toDate);
            string filterPartitionKey = TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, adminName);
            string finalCondition = TableQuery.CombineFilters(filterconditionFromDate, TableOperators.And, filterconditionToDate);
            finalCondition = TableQuery.CombineFilters(finalCondition, TableOperators.And, filterPartitionKey);
            query = new TableQuery<MVPDRegistration>().Where(finalCondition);
            return this.AzureStorage.FetchEntity<MVPDRegistration>(Utils.CurrentRegionName + Utils.Configuration.CurrentRegionId + Constants.MVPDRegistrationTableName, query).ToArray();
        }

        /// <summary>
        /// Returns all of the fixed MVPD Registrations.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="transactionId">Specifies the TransactionId that is being used for 
        /// filtering the query.</param>
        /// <returns>Returns an array of MVPD Registrations.</returns>
        public MVPDRegistration[] GetMVPDRegistration(string adminName = null, string transactionId = null)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.GetMVPDRegistration");
            TableQuery<MVPDRegistration> query;
            query = new TableQuery<MVPDRegistration>().Where(TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, adminName));
            return this.AzureStorage.FetchEntity<MVPDRegistration>(Utils.CurrentRegionName + Utils.CurrentRegionId + Constants.MVPDRegistrationTableName, query).ToArray();
        }

        /// <summary>
        /// Returns all of the TV Receive Site Registrations.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="transactionId">Specifies the TransactionId that is being used for 
        /// filtering the query.</param>
        /// <returns>Returns an array of TV Receive Site Registrations.</returns>
        public TVReceiveSiteRegistration[] GetTVReceiveSiteRegistration(string adminName = null, string transactionId = null)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.GetTVReceiveSiteRegistration");
            TableQuery<TVReceiveSiteRegistration> query;
            query = new TableQuery<TVReceiveSiteRegistration>().Where(TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, adminName));
            return this.AzureStorage.FetchEntity<TVReceiveSiteRegistration>(Utils.CurrentRegionName + Utils.CurrentRegionId + Constants.TVReceiveSiteRegistrationTablename, query).ToArray();
        }

        /// <summary>
        /// Returns all of the TV Receive Site Registrations for the given time
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="fromDate">from date</param>
        /// <param name="toDate">to date</param>
        /// <returns>TV Receive Site Registrations Array</returns>
        public TVReceiveSiteRegistration[] GetTVReceiveSiteRegistration(string adminName, DateTime fromDate, DateTime toDate)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.GetTVReceiveSiteRegistration");
            TableQuery<TVReceiveSiteRegistration> query;
            string filterconditionFromDate = TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.GreaterThanOrEqual, fromDate);
            string filterconditionToDate = TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.LessThanOrEqual, toDate);
            string filterPartitionKey = TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, adminName);
            string finalCondition = TableQuery.CombineFilters(filterconditionFromDate, TableOperators.And, filterconditionToDate);
            finalCondition = TableQuery.CombineFilters(finalCondition, TableOperators.And, filterPartitionKey);
            query = new TableQuery<TVReceiveSiteRegistration>().Where(finalCondition);
            return this.AzureStorage.FetchEntity<TVReceiveSiteRegistration>(Utils.CurrentRegionName + Utils.Configuration.CurrentRegionId + Constants.TVReceiveSiteRegistrationTablename, query).ToArray();
        }

        /// <summary>
        /// Returns all of the Temp Bas Registrations.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="fromDate">from date</param>
        /// <param name="toDate">to date</param>
        /// <returns>Temp Bas Registrations Array</returns>
        public TempBASRegistration[] GetTempBASRegistration(string adminName, DateTime fromDate, DateTime toDate)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.GetTempBASRegistration");
            TableQuery<TempBASRegistration> query;
            string filterconditionFromDate = TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.GreaterThanOrEqual, fromDate);
            string filterconditionToDate = TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.LessThanOrEqual, toDate);
            string filterPartitionKey = TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, adminName);
            string finalCondition = TableQuery.CombineFilters(filterconditionFromDate, TableOperators.And, filterconditionToDate);
            finalCondition = TableQuery.CombineFilters(finalCondition, TableOperators.And, filterPartitionKey);
            query = new TableQuery<TempBASRegistration>().Where(finalCondition);
            return this.AzureStorage.FetchEntity<TempBASRegistration>(Utils.CurrentRegionName + Utils.Configuration.CurrentRegionId + Constants.TempBasRegistrationTableName, query).ToArray();
        }

        /// <summary>
        /// Returns all of the Temp BAS Registrations.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="transactionId">Specifies the TransactionId that is being used for 
        /// filtering the query.</param>
        /// <returns>Returns an array of Temp BAS Registrations.</returns>
        public TempBASRegistration[] GetTempBASRegistration(string adminName = null, string transactionId = null)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.GetTempBASRegistration");
            TableQuery<TempBASRegistration> query;
            query = new TableQuery<TempBASRegistration>().Where(TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, adminName));
            return this.AzureStorage.FetchEntity<TempBASRegistration>(Utils.CurrentRegionName + Utils.CurrentRegionId + Constants.TempBasRegistrationTableName, query).ToArray();
        }

        /// <summary>
        /// Updates all of the registrations in the DB.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.</param>
        /// <param name="fixedRegistrations">Updated Fixed TVBD Registrations.</param>
        /// <param name="licensedPointAuxRegistrations">Updated LP Aux Registrations.</param>
        /// <param name="mvpdRegistrations">Updated MVPD Registrations.</param>
        /// <param name="televisionReceiveSiteRegistrations">Updated Receive Site Registrations.</param>
        /// <param name="tempBASregistrations">Updated Temp BAS Registrations.</param>
        /// <param name="transactionId">The transaction Id of this update.</param>
        public void Update(
            string adminName,
            FixedTVBDRegistration[] fixedRegistrations,
            LPAuxRegistration[] licensedPointAuxRegistrations,
            MVPDRegistration[] mvpdRegistrations,
            TVReceiveSiteRegistration[] televisionReceiveSiteRegistrations,
            TempBASRegistration[] tempBASregistrations,
            string transactionId = null)
        {
            AzureStorageHelper storageHelper = new AzureStorageHelper(this.Logger);
            try
            {
                // MVPD Registrations Batch Saving
                if (mvpdRegistrations.Length > 0)
                {
                    var upadtedRegistrations = mvpdRegistrations.Where(obj => obj.Disposition.Action == 1 || obj.Disposition.Action == 2).ToList();

                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.Update MVPDRegistration");
                    this.UpdateTable(Constants.MVPDRegistrationTableName, upadtedRegistrations);
                    this.DeleteRecords(Constants.MVPDRegistrationTableName, mvpdRegistrations.Where(obj => obj.Disposition.Action == 0).ToList());
                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "End - " + "AzureDalc.Update MVPDRegistration");

                    // update portal contours and portal summary tables
                    var registrationTableName = Utils.GetRegionalTableName(Constants.MVPDRegistrationTableName);
                    var incumbentData = this.FetchEntity<MVPDRegistration>(registrationTableName, null);
                    var portalContours = PortalContourHelper.GetContoursFromMvpd(incumbentData);
                    this.UpdatePortalContoursAndSummary(portalContours, (int)IncumbentType.MVPD);
                }

                // TempBas Registrations Batch Saving
                if (tempBASregistrations.Length > 0)
                {
                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.Update Temp Bas Registration");
                    this.UpdateTable(Constants.TempBasRegistrationTableName, tempBASregistrations.Where(obj => obj.Disposition.Action == 1 || obj.Disposition.Action == 2).ToList());
                    this.DeleteRecords(Constants.TempBasRegistrationTableName, tempBASregistrations.Where(obj => obj.Disposition.Action == 0).ToList());
                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "End - " + "AzureDalc.Update emp Bas Registration");
                }

                // Fixed TVBD Registrations Batch Saving
                if (fixedRegistrations.Length > 0)
                {
                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.Update FixedTVBDRegistration");
                    this.UpdateTable(Constants.FixedTVBDRegistrationTablename, fixedRegistrations.Where(obj => obj.Disposition.Action == 1 || obj.Disposition.Action == 2).ToList());
                    this.DeleteRecords(Constants.FixedTVBDRegistrationTablename, fixedRegistrations.Where(obj => obj.Disposition.Action == 0).ToList());
                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "End - " + "AzureDalc.Update FixedTVBDRegistration");
                }

                // TV Receive Site Registrations Batch Saving
                if (televisionReceiveSiteRegistrations.Length > 0)
                {
                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.Update TVReceiveSiteRegistration");
                    this.UpdateTable(Constants.TVReceiveSiteRegistrationTablename, televisionReceiveSiteRegistrations.Where(obj => obj.Disposition.Action == 1 || obj.Disposition.Action == 2).ToList());
                    this.DeleteRecords(Constants.TVReceiveSiteRegistrationTablename, televisionReceiveSiteRegistrations.Where(obj => obj.Disposition.Action == 0).ToList());
                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "End - " + "AzureDalc.Update TVReceiveSiteRegistration");
                }

                // LP-Aux Registrations Batch Saving
                if (licensedPointAuxRegistrations.Length > 0)
                {
                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.Update Lp-AuxRegistration");
                    this.UpdateTable(Constants.LPAuxRegistrationTable, licensedPointAuxRegistrations.Where(obj => obj.Disposition.Action == 1 || obj.Disposition.Action == 2).ToList());
                    this.DeleteRecords(Constants.LPAuxRegistrationTable, licensedPointAuxRegistrations.Where(obj => obj.Disposition.Action == 0).ToList());
                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "End - " + "AzureDalc.Update Lp-AuxRegistration");
                    this.SaveLPAuxRegistrationDetailsTable(licensedPointAuxRegistrations);
                }
            }
            catch (StorageException ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, "AzureDalc.Update -" + ex.ToString());
                throw;
            }
        }

        /// <summary>
        /// Builds the LP AUX table.
        /// </summary>
        /// <param name="lowPowerAuxRegistrations">The LP AUX registrations.</param>
        public void SaveLPAuxRegistrationDetailsTable(params LPAuxRegistration[] lowPowerAuxRegistrations)
        {
            List<SyncRow> finalList = new List<SyncRow>();

            // Latitude, Longitude, NEPointLatitude, NEPointLongitude, SWPointLatitude, SWPointLongitude, Channel, StartTime, EndTime)
            for (int i = 0; i < lowPowerAuxRegistrations.Length; i++)
            {
                // for single point entities
                if (lowPowerAuxRegistrations[i].PointsArea != null && lowPowerAuxRegistrations[i].PointsArea.Any())
                {
                    for (int j = 0; j < lowPowerAuxRegistrations[i].PointsArea.Length; j++)
                    {
                        if (lowPowerAuxRegistrations[i].Event != null && lowPowerAuxRegistrations[i].Event.Times != null && lowPowerAuxRegistrations[i].Event.Times.Any())
                        {
                            for (int k = 0; k < lowPowerAuxRegistrations[i].Event.Times.Length; k++)
                            {
                                SyncRow curRow = new SyncRow();
                                curRow.Add("Latitude", EntityProperty.GeneratePropertyForDouble(lowPowerAuxRegistrations[i].PointsArea[j].Latitude));
                                curRow.Add("Longitude", EntityProperty.GeneratePropertyForDouble(lowPowerAuxRegistrations[i].PointsArea[j].Longitude));
                                curRow.Add("Channel", EntityProperty.GeneratePropertyForDouble(lowPowerAuxRegistrations[i].CallSign.Channel));
                                curRow.Add("StartTime", EntityProperty.GeneratePropertyForDateTimeOffset(lowPowerAuxRegistrations[i].Event.Times[k].Start.ToDateTime()));
                                curRow.Add("EndTime", EntityProperty.GeneratePropertyForDateTimeOffset(lowPowerAuxRegistrations[i].Event.Times[k].End.ToDateTime()));
                                curRow.Add("RowKey", EntityProperty.GeneratePropertyForString(Guid.NewGuid().ToString()));

                                finalList.Add(curRow);
                            }
                        }
                    }
                }
                else if (lowPowerAuxRegistrations[i].QuadrilateralArea != null && lowPowerAuxRegistrations[i].QuadrilateralArea.Any())
                {
                    for (int j = 0; j < lowPowerAuxRegistrations[i].QuadrilateralArea.Length; j++)
                    {
                        if (lowPowerAuxRegistrations[i].Event != null && lowPowerAuxRegistrations[i].Event.Times != null && lowPowerAuxRegistrations[i].Event.Times.Any())
                        {
                            for (int k = 0; k < lowPowerAuxRegistrations[i].Event.Times.Length; k++)
                            {
                                SyncRow curRow = new SyncRow();
                                curRow.Add("NEPointLatitude", EntityProperty.GeneratePropertyForDouble(lowPowerAuxRegistrations[i].QuadrilateralArea[j].NEPoint.Latitude));
                                curRow.Add("NEPointLongitude", EntityProperty.GeneratePropertyForDouble(lowPowerAuxRegistrations[i].QuadrilateralArea[j].NEPoint.Longitude));
                                curRow.Add("SWPointLatitude", EntityProperty.GeneratePropertyForDouble(lowPowerAuxRegistrations[i].QuadrilateralArea[j].SWPoint.Latitude));
                                curRow.Add("SWPointLongitude", EntityProperty.GeneratePropertyForDouble(lowPowerAuxRegistrations[i].QuadrilateralArea[j].SWPoint.Longitude));
                                curRow.Add("Channel", EntityProperty.GeneratePropertyForDouble(lowPowerAuxRegistrations[i].CallSign.Channel));
                                curRow.Add("StartTime", EntityProperty.GeneratePropertyForDateTimeOffset(lowPowerAuxRegistrations[i].Event.Times[k].Start.ToDateTime()));
                                curRow.Add("EndTime", EntityProperty.GeneratePropertyForDateTimeOffset(lowPowerAuxRegistrations[i].Event.Times[k].End.ToDateTime()));
                                curRow.Add("RowKey", EntityProperty.GeneratePropertyForString(Guid.NewGuid().ToString()));

                                finalList.Add(curRow);
                            }
                        }
                    }
                }
            }

            this.UpdateTable(Constants.LPAuxRegDetails, finalList, "LPAUX");
        }

        /// <summary>
        /// Updates next transaction ID
        /// </summary>
        /// <param name="nextTransactionID"> next transaction ID table entity</param>
        public void SaveNextTransactionIdInfo(NextTransactionID nextTransactionID)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.SaveNextTransactionIdInfo");
            this.AzureStorage.InsertEntity(Utils.CurrentRegionName + Utils.CurrentRegionId + Constants.NextTransactionIdTableName, nextTransactionID);
        }

        /// <summary>
        /// Returns the last transaction Id used for the specified DB Admin.
        /// </summary>
        /// <param name="adminName">DB Admin name (SPBR, TELC, MSFT, ...)</param>
        /// <param name="scope">DB Sync Scope</param>
        /// <returns>Returns the last transaction Id.</returns>
        public NextTransactionID GetRequestorTransactionId(string adminName, DbSyncScope scope)
        {
            try
            {
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - AzureDalc.GetRequestorTransactionId");
                List<NextTransactionID> transactions = null;
                TableQuery<NextTransactionID> query;
                string filtercondition = null;
                string filterPartitionKey = null;

                filtercondition = TableQuery.GenerateFilterCondition("WSDBA", QueryComparisons.Equal, adminName);
                filterPartitionKey = TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, scope.ToString());
                query = new TableQuery<NextTransactionID>().Where(TableQuery.CombineFilters(filtercondition, TableOperators.And, filterPartitionKey));

                transactions = this.AzureStorage.FetchEntity<NextTransactionID>(Utils.CurrentRegionName + Utils.Configuration.CurrentRegionId + Constants.NextTransactionIdTablename, query);

                if (transactions != null && transactions.Count > 0)
                {
                    return transactions[0];
                }
                else
                {
                    if (scope == DbSyncScope.INC)
                    {
                        filtercondition = TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.GreaterThanOrEqual, DateTime.Now.AddDays(-1));
                        filterPartitionKey = TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, DbSyncScope.ALL.ToString());
                        query = new TableQuery<NextTransactionID>().Where(TableQuery.CombineFilters(filtercondition, TableOperators.And, filterPartitionKey));
                    }

                    transactions = this.AzureStorage.FetchEntity<NextTransactionID>(Utils.CurrentRegionName + Utils.Configuration.CurrentRegionId + Constants.NextTransactionIdTablename, query);
                    if (transactions != null && transactions.Count > 0)
                    {
                        NextTransactionID nt = new NextTransactionID();

                        nt.NextTransactionId = transactions[0].NextTransactionId;
                        nt.PartitionKey = DbSyncScope.INC.ToString();
                        nt.RowKey = Guid.NewGuid().ToString();
                        nt.Timestamp = transactions[0].Timestamp;
                        nt.WSDBA = transactions[0].WSDBA;
                        nt.NextTransactionIdDateTime = transactions[0].NextTransactionIdDateTime;
                        return nt;
                    }
                    else
                    {
                        return null;
                    }
                }
            }
            catch (Exception e)
            {
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "AzureDalc.GetRequestorTransactionId -" + e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Returns the last transaction Id used for the specified DB Admin.
        /// </summary>
        /// <param name="requestedTransactionId">Requested Transaction ID</param>
        /// <returns>Returns the last transaction Id.</returns>
        public NextTransactionID GetRequestorTransactionId(string requestedTransactionId)
        {
            try
            {
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - AzureDalc.GetRequestorTransactionId");
                List<NextTransactionID> transactions = null;
                TableQuery<NextTransactionID> query;
                string filtercondition = TableQuery.GenerateFilterCondition("NextTransactionId", QueryComparisons.Equal, requestedTransactionId);
                query = new TableQuery<NextTransactionID>().Where(filtercondition);
                transactions = this.AzureStorage.FetchEntity<NextTransactionID>(Utils.CurrentRegionName + Utils.Configuration.CurrentRegionId + Constants.NextTransactionIdTablename, query);

                if (transactions != null && transactions.Count > 0)
                {
                    return transactions[0];
                }
                else
                {
                    return null;
                }
            }
            catch (Exception e)
            {
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "AzureDalc.GetRequestorTransactionId -" + e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Returns transaction info for the Sync Poller worker.
        /// </summary>
        /// <param name="adminName">DB Admin name (SPBR, TELC, MSFT, ...)</param>
        /// <returns>Returns the transaction info of the whitespace DBA.</returns>
        public TransactionInfo GetPollerTransactionInfo(string adminName)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Returns all of the known DB Admins.
        /// </summary>
        /// <returns>All of the registered DB Admins</returns>
        public DBAdminInfo[] GetDBAdmins()
        {
            try
            {
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin -  Dalc GetDBAdmins");
                return this.AzureStorage.GetWSDBAPollInfo(Utils.CurrentRegionName + Utils.CurrentRegionId + Constants.DBAdminInfoTable);
            }
            catch (Exception e)
            {
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Saves the specified spectrum usage.
        /// </summary>
        /// <param name="spectrumUsageTable">Spectrum Usage table that is to be updated.</param>
        /// <param name="regEntity">Registration entity that is to be updated.</param>
        /// <returns>Response for spectrum usage notify</returns>
        public int NotifySpectrumUsage(string spectrumUsageTable, object regEntity)
        {
            try
            {
                string logMethodName = "AzureDalc.NotifySpectrumUsage(" + spectrumUsageTable + ", object regEntity)";

                // Begin Log Transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);
                TableEntity tableEntity = (TableEntity)regEntity;
                this.AzureStorage.TransactionId = this.Logger.TransactionId;

                // Route to InsertEntity method in AzureStorageHelper
                this.AzureStorage.InsertEntity(spectrumUsageTable, tableEntity);

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName);
                return 0;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
                return -32000;
            }
        }

        /// <summary>
        /// Returns Device Information of the specified Id.
        /// </summary>
        /// <param name="id">The device Id.</param>
        /// <returns>Device Descriptor information.</returns>
        public DeviceDescriptor GetDeviceInfo(string id)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Returns all of the device descriptors.
        /// </summary>
        /// <returns>All of the device descriptors.</returns>
        public DeviceDescriptor[] GetDevices()
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Updates the specified table.
        /// </summary>
        /// <param name="tableName">Name of the table being updated.</param>
        /// <param name="tableRows">All the rows within the table being updated.</param>
        /// <param name="partitionKey">The partition key.</param>
        /// <returns>returns success code.</returns>
        public int UpdateTable(string tableName, List<SyncRow> tableRows, string partitionKey)
        {
            string logMethodName = "AzureDalc.UpdateTable(" + tableName + ", SyncTable table)";

            var regionalTableName = Utils.GetRegionalTableName(tableName);

            try
            {
                // Begin Log Transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);

                List<DynamicTableEntity> entities = new List<DynamicTableEntity>();
                foreach (var tableRow in tableRows)
                {
                    var obj = new DynamicTableEntity();
                    obj.RowKey = tableRow["RowKey"].StringValue;
                    obj.PartitionKey = partitionKey;
                    tableRow.Columns.Remove("RowKey");
                    obj.Properties = tableRow.Columns;

                    entities.Add(obj);
                }

                tableRows = null;

                List<TableOperation> tableOperations = new List<TableOperation>();

                for (int i = 0; i < entities.Count; i++)
                {
                    tableOperations.Add(TableOperation.InsertOrReplace(entities[i]));
                }

                this.AzureStorage.InsertUpdateBatch(regionalTableName, tableOperations);

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName);

                // returns Success
                return 1;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());

                throw;
            }
        }

        /// <summary>
        /// Updates the specified table.
        /// </summary>
        /// <typeparam name="T">type of entity</typeparam>
        /// <param name="tableName">Name of the table being updated.</param>
        /// <param name="tableRows">All the rows within the table being updated.</param>
        /// <returns>returns success code.</returns>
        public int UpdateTable<T>(string tableName, List<T> tableRows) where T : ITableEntity, new()
        {
            var regionalTableName = Utils.GetRegionalTableName(tableName);

            try
            {
                List<TableOperation> tableOperations = new List<TableOperation>();

                for (int i = 0; i < tableRows.Count; i++)
                {
                    tableOperations.Add(TableOperation.InsertOrReplace(tableRows[i]));
                }

                this.AzureStorage.InsertUpdateBatch(regionalTableName, tableOperations);

                // returns Success
                return 1;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());

                throw;
            }
        }

        /// <summary>
        /// Truncates the records.
        /// </summary>
        /// <typeparam name="T">type of record</typeparam>
        /// <param name="tableName">Name of the table.</param>
        public void TruncateRecords<T>(string tableName) where T : ITableEntity, new()
        {
            var regionalTableName = Utils.GetRegionalTableName(tableName);
            var totalRecords = this.AzureStorage.FetchEntity<T>(regionalTableName);
            this.DeleteRecords(tableName, totalRecords);
        }

        /// <summary>
        /// Deletes the records.
        /// </summary>
        /// <typeparam name="T">type  of entity</typeparam>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="tableRows">The table rows.</param>
        public void DeleteRecords<T>(string tableName, List<T> tableRows) where T : ITableEntity, new()
        {
            var regionalTableName = Utils.GetRegionalTableName(tableName);

            try
            {
                List<TableOperation> tableOperations = new List<TableOperation>();

                for (int i = 0; i < tableRows.Count; i++)
                {
                    tableOperations.Add(TableOperation.Delete(tableRows[i]));
                }

                this.AzureStorage.InsertUpdateBatch(regionalTableName, tableOperations);
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());

                throw;
            }
        }

        /// <summary>
        /// Updated the sync status for the specified database.
        /// </summary>
        /// <param name="name">The database name that has just been synchronized (CDBS, ULS, EAS)</param>
        /// <param name="syncEntityName">Name of the synchronize entity.</param>
        /// <param name="success">Indicates if the database was successfully synchronized.</param>
        public void SetDatabaseSyncStatus(string name, string syncEntityName, bool success)
        {
            string logMethodName = "AzureDalc.SetDatabaseSyncStatus(" + name + ", success " + success + ")";

            var regionalTableName = Utils.GetRegionalTableName(Constants.RegionSyncStatusTableName);

            try
            {
                // Begin Log Transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);

                DynamicTableEntity tableEntity = new DynamicTableEntity();
                tableEntity.RowKey = syncEntityName;
                tableEntity.PartitionKey = name;
                tableEntity.Properties.Add(new KeyValuePair<string, EntityProperty>("Success", EntityProperty.GeneratePropertyForBool(success)));

                this.AzureStorage.InsertUpdateDynamicEntity(regionalTableName, tableEntity);

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName);
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());

                throw;
            }
        }

        /// <summary>
        /// Updates the CDBS data.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="cdbsData">The CDBS data.</param>
        /// <returns>returns status.</returns>
        public int UpdateCDBSData(string tableName, List<CDBSTvEngData> cdbsData)
        {
            return this.UpdateTable<CDBSTvEngData>(tableName, cdbsData);
        }

        /// <summary>
        /// Updates the ULS data.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="ulsData">The ULS data.</param>
        /// <returns>returns status.</returns>
        public int UpdateULSData(string tableName, List<ULSRecord> ulsData)
        {
            return this.UpdateTable<ULSRecord>(tableName, ulsData);
        }

        /// <summary>
        /// Deletes the CDBS data.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="cdbsData">The CDBS data.</param>
        public void DeleteCDBSData(string tableName, List<CDBSTvEngData> cdbsData)
        {
            this.DeleteRecords(tableName, cdbsData);
        }

        /// <summary>
        /// Deletes the ULS data.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="ulsdata">The ULS data.</param>
        public void DeleteULSData(string tableName, List<ULSRecord> ulsdata)
        {
            this.DeleteRecords(tableName, ulsdata);
        }

        /// <summary>
        /// Returns the last time the specified database was synchronized.
        /// </summary>
        /// <param name="name">The database name that is being queried(CDBS, ULS, EAS)</param>
        /// <returns>DateTime of the last successful sync time.</returns>
        public System.DateTime GetLastsuccessfulSyncTime(string name)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Updates the authorized device records.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="deviceRecords">The device records.</param>
        /// <returns>returns status.</returns>
        public int UpdateAuthorizedDeviceRecords(string tableName, List<AuthorizedDeviceRecord> deviceRecords)
        {
            this.TruncateRecords<AuthorizedDeviceRecord>(tableName);
            return this.UpdateTable(tableName, deviceRecords);
        }

        /// <summary>
        /// Adds the specified user into the database.
        /// </summary>
        /// <param name="user">User information that is to be added.</param>
        /// <returns> user id </returns>
        public string AddUser(User user)
        {
            string uniqueID = Guid.NewGuid().ToString();
            string logMethodName = "AzureDalc.AddUser(User user)";

            // Begin Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);
            try
            {
                user.PartitionKey = user.UserId;
                user.RowKey = uniqueID;
                user.CreationDate = System.DateTime.Now;
                this.AzureStorage.InsertUser(Constants.AuthorizedUsersTable, user);

                if (user.Access != null)
                {
                    foreach (RegionAccess access in user.Access)
                    {
                        access.PartitionKey = uniqueID;
                        access.RowKey = Guid.NewGuid().ToString();
                        this.AzureStorage.InsertAccessRegion(Constants.RegionAccessTable, access);
                    }
                }

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName);
                return uniqueID;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Updated the specified user fields in the database.
        /// </summary>
        /// <param name="user">Updated user information.</param>
        /// <returns> user id </returns>
        public string UpdateUser(User user)
        {
            string logMethodName = "AzureDalc.UpdateUser(User user)";

            // Begin Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);
            try
            {
                User userInfo = this.GetUser(user.UserId);
                user.RowKey = userInfo.RowKey;
                user.PartitionKey = userInfo.PartitionKey;
                user.ETag = userInfo.ETag;
                user.CreationDate = userInfo.CreationDate;
                user.CreateBy = userInfo.CreateBy;

                this.AzureStorage.UpdateUser(Constants.AuthorizedUsersTable, user);

                if (user.Access != null)
                {
                    List<TableOperation> tableOperations = new List<TableOperation>();
                    foreach (RegionAccess access in userInfo.Access)
                    {
                        access.Deleted = true;
                        tableOperations.Add(TableOperation.Merge(access));
                    }

                    this.AzureStorage.InsertUpdateBatch(Constants.RegionAccessTable, tableOperations);

                    foreach (RegionAccess access in user.Access)
                    {
                        access.PartitionKey = userInfo.RowKey;
                        access.RowKey = Guid.NewGuid().ToString();
                        this.AzureStorage.InsertAccessRegion(Constants.RegionAccessTable, access);
                    }
                }

                return user.RowKey;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Gets the specified user.
        /// </summary>
        /// <param name="userId">The user Id that is to be retrieved.</param>
        /// <returns>Returns the specified user (or null if no user is found with the specified Id).</returns>
        public User GetUser(string userId)
        {
            string logMethodName = "AzureDalc.GetUser(string userId)";

            // Begin Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);
            try
            {
                User user = null;
                TableQuery<User> query;
                string filtercondition = null;
                string filterPartitionKey = null;

                filtercondition = TableQuery.GenerateFilterCondition("RowKey", QueryComparisons.Equal, userId);
                filterPartitionKey = TableQuery.GenerateFilterConditionForBool("Deleted", QueryComparisons.Equal, false);

                query = new TableQuery<User>().Where(TableQuery.CombineFilters(filtercondition, TableOperators.And, filterPartitionKey));
                IEnumerable<User> entities = this.AzureStorage.FetchEntity<User>(Constants.AuthorizedUsersTable, query).ToArray();

                if (entities.Count() > 0)
                {
                    user = entities.ElementAt(0);
                    user.Access = this.AzureStorage.GetUserAccessLevels(Constants.RegionAccessTable, userId).ToArray();
                }

                return user;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Gets the specified user by Live Id.
        /// </summary>
        /// <param name="liveId">The Live Id that is to be retrieved.</param>
        /// <returns>Returns the specified user (or null if no user is found with the specified Id).</returns>
        public User GetUserByLiveId(string liveId)
        {
            User user = null;
            TableQuery<User> query;
            query = new TableQuery<User>().Where(TableQuery.GenerateFilterCondition("UserId", QueryComparisons.Equal, liveId));
            IEnumerable<User> entities = this.AzureStorage.FetchEntity<User>(Constants.AuthorizedUsersTable, query).ToArray();

            if (entities.Count() > 0)
            {
                user = entities.ElementAt(0);
                user.Access = this.AzureStorage.GetUserAccessLevels(Constants.RegionAccessTable, user.RowKey).ToArray();
            }

            return user;
        }

        /// <summary>
        /// Request for Elevated Access
        /// </summary>
        /// <param name="elevatedAccessRequest">The elevated access request.</param>
        /// <returns>returns String </returns>
        public string RequestElevatedAccess(ElevatedAccessRequest elevatedAccessRequest)
        {
            string logMethodName = "AzureDalc.RequestElevatedAccess(elevatedAccessRequest)";

            // Begin Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);
            try
            {
                elevatedAccessRequest.PartitionKey = elevatedAccessRequest.UserId;
                elevatedAccessRequest.RowKey = Guid.NewGuid().ToString();
                this.AzureStorage.ElevateAccessRequest(Constants.ElevateAccessTable, elevatedAccessRequest);
                return elevatedAccessRequest.UserId;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Gets all of the registered user.
        /// </summary>
        /// <returns>All registered users.</returns>
        public User[] GetUsers()
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Loads the specified terrain file from the storage.
        /// </summary>
        /// <param name="fileName">File name that is to be retrieved from storage.</param>
        /// <param name="dataDirectory">The data directory.</param>
        /// <param name="terrainElementSize">Size of the terrain element.</param>
        /// <returns>returns File data.</returns>
        public object LoadTerrainFile(string fileName, string dataDirectory, int terrainElementSize)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.TerrainCacheTileLoaded, string.Format("Opening Terrain File {0}, DataDir: {1}, Time - {2}", fileName, dataDirectory, DateTime.UtcNow));

            var container = this.AzureStorage.GetCloudBlobContainer(dataDirectory);
            var blockBlob = container.GetBlockBlobReference(fileName);
            byte[] fileContents = new byte[terrainElementSize];
            if (blockBlob.Exists())
            {
                blockBlob.DownloadToByteArray(fileContents, 0);
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.TerrainCacheTileLoaded, "Closed Blob File -> " + DateTime.UtcNow);

            return fileContents;
        }

        /// <summary>
        /// Checks the terrain file.
        /// </summary>
        /// <param name="fileName">Name of the file.</param>
        /// <param name="dataDirectory">The data directory.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool CheckTerrainFileExists(string fileName, string dataDirectory)
        {
            var container = this.AzureStorage.GetCloudBlobContainer(dataDirectory);
            var blockBlob = container.GetBlockBlobReference(fileName);
            if (blockBlob.Exists())
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Loads the terrain file.
        /// </summary>
        /// <param name="fileName">Name of the file.</param>
        /// <param name="dataDirectory">The data directory.</param>
        /// <returns>returns Stream.</returns>
        public Stream LoadTerrainFile(string fileName, string dataDirectory)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.TerrainCacheTileLoaded, string.Format("Opening Terrain File {0}, Time - {1}", fileName, DateTime.UtcNow));

            var container = this.AzureStorage.GetCloudBlobContainer(dataDirectory);
            var blockBlob = container.GetBlockBlobReference(fileName);
            MemoryStream dataStream = new MemoryStream();

            if (blockBlob.Exists())
            {
                blockBlob.DownloadToStream(dataStream);
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.TerrainCacheTileLoaded, "Closed Terrain File -> " + DateTime.UtcNow);

            return dataStream;
        }

        /// <summary>
        /// Streams the BLOB file.
        /// </summary>
        /// <param name="fileName">Name of the file.</param>
        /// <param name="dataDirectory">The data directory.</param>
        /// <param name="timeSpan">The time span.</param>
        /// <returns>returns Stream.</returns>
        public Stream StreamBlobFile(string fileName, string dataDirectory, TimeSpan? timeSpan = null)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, string.Format("Opening Blob File {0}, Time - {1}", fileName, DateTime.UtcNow));

            var container = this.AzureStorage.GetCloudBlobContainer(dataDirectory);
            var blockBlob = container.GetBlockBlobReference(fileName);
            Stream dataStream = null;

            if (blockBlob.Exists())
            {
                if (timeSpan.HasValue)
                {
                    BlobRequestOptions requestOptions = new BlobRequestOptions();
                    requestOptions.ServerTimeout = timeSpan.Value;
                    dataStream = blockBlob.OpenRead(null, requestOptions);
                }
                else
                {
                    dataStream = blockBlob.OpenRead();
                }
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Streamed Blob File -> " + DateTime.UtcNow);

            return dataStream;
        }

        /// <summary>
        /// Loads the DTT synchronize file.
        /// </summary>
        /// <param name="fileName">Name of the file.</param>
        /// <returns>returns List{DTTDataAvailability}.</returns>
        public List<DTTDataAvailability> LoadDttSyncFile(string fileName)
        {
            try
            {
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DttSyncGenericMessage, string.Format("Opening Blob File {0}, Time - {1}", fileName, DateTime.UtcNow));

                string dataDirectory = Utils.Configuration["DttSyncSourceContainer"];

                var container = this.AzureStorage.GetDttSyncCloudBlobContainer(dataDirectory);
                var blockBlob = container.GetBlockBlobReference(fileName);

                List<DTTDataAvailability> listPD = new List<DTTDataAvailability>();
                string[] fileNameSegments = fileName.Split('_');
                string partitionKey = fileName.Replace("_" + fileNameSegments.Last(), string.Empty);
                using (TextReader reader = new StreamReader(blockBlob.OpenRead()))
                {
                    var line = string.Empty;
                    while ((line = reader.ReadLine()) != null)
                    {
                        var values = line.Split(',');
                        if (values[0].ToLower() == "easting")
                        {
                            continue;
                        }

                        DTTDataAvailability pd = new DTTDataAvailability();
                        pd.PartitionKey = partitionKey;
                        pd.RowKey = values[0] + "-" + values[1];
                        pd.Easting = values[0].ToInt32();
                        pd.Northing = values[1].ToInt32();
                        ////pd.DataRecord = line;
                        listPD.Add(pd);
                    }
                }

                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DttSyncGenericMessage, "Closed Blob File -> " + DateTime.UtcNow);
                return listPD;
            }
            catch (Exception e)
            {
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DttSyncGenericMessage, "Exception Occured in file : - " + fileName + " Exception :- " + e.ToString());

                throw;
            }
        }

        /// <summary>
        /// Gets the DTT synchronize file list from BLOB.
        /// </summary>
        /// <returns>returns IListBlobItem[][].</returns>
        public IListBlobItem[] GetDttSyncFileListFromBlob()
        {
            string dataDirectory = Utils.Configuration["DttSyncSourceContainer"];
            var container = this.AzureStorage.GetDttSyncCloudBlobContainer(dataDirectory);

            ////List blobs and directories in this container
            return container.ListBlobs().ToArray();
        }

        /// <summary>
        /// Fetches the registration type.
        /// </summary>
        /// <param name="request">Table entity that is to be fetched.</param>
        /// <returns>Entity Name.</returns>
        public string FetchDeviceRegistrationType(IRegisterRequest request)
        {
            return this.AzureStorage.FetchEntity(Constants.DeviceRegistrationTable, request.DeviceDescriptor.FccTvbdDeviceType).ToString();
        }

        /// <summary>
        /// Fetches the Date or Sequence.
        /// </summary>
        /// <param name="settingsTable">Table type to be fetched.</param>
        /// <param name="keyValue">Value that is to be fetched.</param>
        /// <returns>Entity Name.</returns>
        public object FetchEntity(string settingsTable, string keyValue)
        {
            return this.AzureStorage.FetchEntity(settingsTable, keyValue);
        }

        /// <summary>
        /// Fetches the entity.
        /// </summary>
        /// <typeparam name="T">Table type to be fetched.</typeparam>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns object.</returns>
        public List<T> FetchEntity<T>(string tableName, object parameters) where T : ITableEntity, new()
        {
            var retValue = this.AzureStorage.FetchEntity<T>(tableName, parameters);
            return retValue;
        }

        /// <summary>
        /// Fetches the entity with desired columns and provided equality filters
        /// </summary>
        /// <typeparam name="T">entity type to be fetched</typeparam>
        /// <param name="tableName">table name</param>
        /// <param name="filters">Equality condition filters</param>
        /// <param name="columns">columns of an entity</param>
        /// <returns>list of entities</returns>
        public List<T> FetchEntity<T>(string tableName, Dictionary<string, string> filters, List<string> columns) where T : ITableEntity, new()
        {
            var finalQuery = string.Empty;
            foreach (var filterKey in filters.Keys)
            {
                var filterQuery = TableQuery.GenerateFilterCondition(filterKey, QueryComparisons.Equal, filters[filterKey]);
                if (string.IsNullOrEmpty(finalQuery))
                {
                    finalQuery = filterQuery;
                }
                else
                {
                    finalQuery = TableQuery.CombineFilters(finalQuery, TableOperators.And, filterQuery);
                }
            }

            TableQuery<T> query = new TableQuery<T>().Where(finalQuery);
            query.Select(columns);

            return this.AzureStorage.FetchEntity<T>(tableName, query);
        }

        /// <summary>
        /// Fetches the RuleSetInfoInsertDTTFileData
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <returns>return RuleSetInfo.</returns>
        public List<RuleSetInfoEntity> GetRulesetInfo(string tableName)
        {
            var retData = this.AzureStorage.FetchEntity<RuleSetInfoEntity>(tableName);
            return retData;
        }

        /// <summary>
        /// Determines whether [is device registered] [the specified table name].
        /// </summary>
        /// <typeparam name="T">type of Table to query on</typeparam>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="deviceId">The device id.</param>
        /// <returns><c>true</c> if [is device registered] [the specified table name]; otherwise, <c>false</c>.</returns>
        public bool IsDeviceRegistered<T>(string tableName, string deviceId) where T : ITableEntity, new()
        {
            ////Stopwatch methodTimer = new Stopwatch();
            ////methodTimer.Start();
            var table = this.AzureStorage.GetCloudTable(tableName);
            var retData = (from fixedtvbd in table.CreateQuery<FixedTVBDRegistration>()
                           where fixedtvbd.SerialNumber == deviceId
                           select fixedtvbd).Take(1).FirstOrDefault();

            ////methodTimer.Stop();
            ////this.Logger.Log(TraceEventType.Critical, LoggingMessageId.RegionCalculationGenericMessage, string.Format("{0}, {1}", "IsDeviceRegistered: ", methodTimer.Elapsed.TotalMilliseconds));

            if (retData == null)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Updates the validated device.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="entity">The entity.</param>
        public void UpdateValidatedDevice(string tableName, TableEntity entity)
        {
            this.AzureStorage.InsertEntity(tableName, entity);
        }

        /// <summary>
        /// Retrieves an entity.
        /// </summary>
        /// <param name="table">Table that is to be fetched.</param>
        /// <param name="keyValue">Value that is to be fetched.</param>
        /// <param name="value">Second value that is to be fetched.</param>
        /// <returns>Entity Name.</returns>
        public object RetrieveEntity(string table, string keyValue, string value)
        {
            return this.AzureStorage.RetrieveEntity(table, keyValue, value);
        }

        /// <summary>
        /// Retrieves an entity.
        /// </summary>
        /// <param name="table">Table that is to be fetched.</param>
        /// <param name="value">Second value that is to be fetched.</param>
        /// <returns>Entity Name.</returns>
        public List<TableEntity> RetrieveEntity(string table, string value)
        {
            return this.AzureStorage.RetrieveEntity(table, value);
        }

        /// <summary>
        /// Gets the DTT dataset values.
        /// </summary>
        /// <param name="tableName">The table.</param>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns DTTDataset values.</returns>
        public List<DttData> GetDTTDatasetValues(string tableName, List<OSGLocation> parameters)
        {
            Stopwatch timer = new Stopwatch();
            timer.Start();

            var minEasting = parameters.Min(obj => obj.Easting);
            var minNorthing = parameters.Min(obj => obj.Northing);

            var maxEasting = parameters.Max(obj => obj.Easting);
            var maxNorthing = parameters.Max(obj => obj.Northing);

            var dttTablenameSuffix = Utils.Configuration[Constants.ConfigSettingDTTSyncTableSuffix];

            if (string.IsNullOrEmpty(dttTablenameSuffix))
            {
                dttTablenameSuffix = Constants.ConfigSettingDefaultDTTSyncTableSuffix;
            }

            var dtttableName = tableName + dttTablenameSuffix;

            int squareBoxStartEasting = DTTSquare.ProjectToDTTSquareEasting(minEasting);
            int squareBoxStartNorthing = DTTSquare.ProjectToDTTSquareNorthing(minNorthing);

            int squareBoxEndEasting = DTTSquare.ProjectToDTTSquareEasting(maxEasting);
            int squareBoxEndNorthing = DTTSquare.ProjectToDTTSquareNorthing(maxNorthing);

            string filter = string.Format("(PartitionKey ge '{0}') and (RowKey ge '{1}') and (PartitionKey le '{2}') and (RowKey le '{3}')", squareBoxStartEasting.ToString().PadLeft(7, '0'), squareBoxStartNorthing.ToString().PadLeft(7, '0'), squareBoxEndEasting.ToString().PadLeft(7, '0'), squareBoxEndNorthing.ToString().PadLeft(7, '0'));

            TableQuery<DTTDataAvailability> query1 = (new TableQuery<DTTDataAvailability>()).Where(filter);
            List<DTTDataAvailability> records = this.AzureStorage.FetchEntityFromTerrainData<DTTDataAvailability>(dtttableName, query1);

            List<DttData> values = new List<DttData>();
            foreach (var dttDataAvailabilityBlock in records)
            {
                var dttsquare = Conversion.DecompressDTTData(dttDataAvailabilityBlock);
                values.AddRange(dttsquare.GetDttBatch());
            }

            var datasetValues = (from parameter in parameters
                                 join dttvalue in values on new { parameter.Easting, parameter.Northing } equals new { dttvalue.Easting, dttvalue.Northing }
                                 select dttvalue).ToList();

            ////if (unscheduledAdjustments != null)
            ////{
            ////    var adjustmentData = (from datasetRow in datasetValues
            ////                          join unschedule in unscheduledAdjustments on new { datasetRow.Easting, datasetRow.Northing } equals new { unschedule.Easting, unschedule.Northing }
            ////                          select new { objunscheduledRow = unschedule, objDatasetRow = datasetRow });

            ////    foreach (var adjustment in adjustmentData)
            ////    {
            ////        for (int i = 0; i < 40; i++)
            ////        {
            ////            adjustment.objDatasetRow.DataValues[i + 2] = adjustment.objDatasetRow.DataValues[i + 2] + adjustment.objunscheduledRow.UnscheduledData[i];
            ////        }
            ////    }
            ////}

            timer.Stop();

            ////this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, string.Format("UniqueId : {0}, GetDTTDatasetValues Time: {1}, RecordCount : {2}, TableName : {3}", parameters[0].PartitionKey, timer.Elapsed.ToString(), datasetValues.Count(), tableName));

            return datasetValues;
        }

        /// <summary>
        /// Gets the excluded regions.
        /// </summary>
        /// <returns>returns List excluded Regions.</returns>
        public List<DynamicTableEntity> GetExcludedRegions()
        {
            return this.AzureStorage.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.ExcludedChannels));
        }

        /// <summary>
        /// Fetches the Date.
        /// </summary>
        /// <param name="tableEntity">Table entity that is to be updated.</param>
        public void UpdateSettingsDateSequence(object tableEntity)
        {
            TableEntity tblEntity = (TableEntity)tableEntity;
            this.AzureStorage.InsertEntity(Utils.GetRegionalTableName(Constants.SettingsTable), tblEntity);
        }

        /// <summary>
        /// Updates the specified incumbent.
        /// </summary>
        /// <param name="registrationTable">Registration table that is to be updated.</param>
        /// <param name="regEntity">Registration entity that is to be updated.</param>
        /// <returns>Response after Registration</returns>
        public int RegisterDevice(string registrationTable, object regEntity)
        {
            try
            {
                string logMethodName = "AzureDalc.RegisterDevice(" + registrationTable + ", object regEntity)";

                // Begin Log Transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);
                TableEntity tableEntity = (TableEntity)regEntity;
                this.AzureStorage.TransactionId = this.Logger.TransactionId;

                // Route to InsertEntity method in AzureStorageHelper
                this.AzureStorage.InsertEntity(registrationTable, tableEntity);

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName);
                return 0;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
                return -32000;
            }
        }

        /// <summary>
        /// Updates the specified EntityTable
        /// </summary>
        /// <param name="entity">table entity that is to be updated.</param>
        public void UpdateSettingsTable(TableEntity entity)
        {
            this.AzureStorage.InsertCloudStorageEntity(entity, Constants.SettingsTable);
        }

        /// <summary>
        /// Updates the specified EntityTable
        /// </summary>
        /// <param name="entity">table entity that is to be updated.</param>
        public void LogMessage(TableEntity entity)
        {
            this.AzureStorage.InsertCloudStorageEntity(entity, AzureLogTableName);
        }

        /// <summary>
        /// PAWS Initialize method.
        /// </summary>
        /// <param name="initializationTable">Initialization table that is to be updated.</param>
        /// <param name="regEntity">Initialization entity that is to be updated.</param>
        /// <returns>Response after Initialization</returns>
        public int Initialize(string initializationTable, object regEntity)
        {
            try
            {
                string logMethodName = "AzureDalc.Initialize(object regEntity" + initializationTable + ")";

                // Begin Log Transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);
                TableEntity tableEntity = (TableEntity)regEntity;

                // Route to InsertEntity method in AzureStorageHelper
                this.AzureStorage.InsertEntity(initializationTable, tableEntity);

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName);
                return 0;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
                return -32000;
            }
        }

        /// <summary>
        /// PAWS Device Validation method.
        /// </summary>
        /// <param name="validationTable">Table that is to be fetched.</param>
        /// <param name="partitionKey">Partition Key that is to be fetched.</param>
        /// <param name="rowKey">Row Key that is to be fetched.</param>
        /// <returns>Response after Device Validation</returns>
        public int ValidateDevice(string validationTable, string partitionKey, string rowKey)
        {
            int resp = 0;
            try
            {
                string logMethodName = "AzureDalc.ValidateDevice(object regEntity" + validationTable + ")";

                // Begin Log Transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);

                // Route to RetrieveEntity method in AzureStorageHelper
                resp = Convert.ToInt32(this.AzureStorage.FetchEntity<FixedTVBDRegistration>(validationTable, partitionKey, rowKey));

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName);
                return resp;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
                return -32000;
            }
        }

        /// <summary>
        /// Updates the specified EntityTable
        /// </summary>
        /// <param name="adminInfo">DBAdminInfo Entity</param>
        public void UpdateDBAdminInfo(DBAdminInfo adminInfo)
        {
            try
            {
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - AzureDalc UpdateDBAdminInfo");
                AzureStorageHelper storageHelper = new AzureStorageHelper(this.Logger);
                storageHelper.InsertEntity(Utils.CurrentRegionName + Utils.CurrentRegionId + Constants.DBAdminInfoTable, adminInfo);
            }
            catch (Exception e)
            {
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "AzureDalc UpdateDBAdminInfo got an exception - " + e.ToString());
                throw;
            }
        }

        #region Implementation of IDalcIncumbent

        /// <summary>
        /// Gets all of the registered incumbents at the current region.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="columnNames">The column names.</param>
        /// <param name="filters">The filters.</param>
        /// <param name="rowCount">The row count.</param>
        /// <returns>Returns all of the registered incumbents at the current region.</returns>
        public Incumbent[] GetIncumbents(string tableName, string[] columnNames = null, object filters = null, int rowCount = 0)
        {
            TableQuery<Incumbent> incumbentQuery = new TableQuery<Incumbent>();
            List<Incumbent> incumbents = new List<Incumbent>();

            if (columnNames != null)
            {
                incumbentQuery.SelectColumns = columnNames;
            }

            if (rowCount > 0)
            {
                incumbentQuery.TakeCount = rowCount;
            }

            if (filters != null)
            {
                incumbentQuery.FilterString = this.AzureStorage.ConvertFiltersToTableQuery(filters);
            }

            incumbents = this.AzureStorage.FetchEntity<Incumbent>(tableName, incumbentQuery);

            return incumbents.ToArray();
        }

        /// <summary>
        /// Gets the combined incumbents.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="squareArea">The square area.</param>
        /// <returns>returns Incumbent[].</returns>
        public Incumbent[] GetCombinedIncumbents(string tableName, SquareArea squareArea)
        {
            var qs1 = TableQuery.GenerateFilterConditionForDouble("Latitude", QueryComparisons.LessThanOrEqual, squareArea.TopLeftPoint.Latitude);
            var qs3 = TableQuery.GenerateFilterConditionForDouble("Latitude", QueryComparisons.GreaterThanOrEqual, squareArea.BottomRightPoint.Latitude);

            var qs2 = TableQuery.GenerateFilterConditionForDouble("Longitude", QueryComparisons.GreaterThanOrEqual, squareArea.TopLeftPoint.Longitude);
            var qs4 = TableQuery.GenerateFilterConditionForDouble("Longitude", QueryComparisons.LessThanOrEqual, squareArea.BottomRightPoint.Longitude);

            var finalQuery = TableQuery.CombineFilters(qs4, TableOperators.And, TableQuery.CombineFilters(qs2, TableOperators.And, TableQuery.CombineFilters(qs1, TableOperators.And, qs3)));
            TableQuery<Incumbent> query = (new TableQuery<Incumbent>()).Where(finalQuery);
            var incumbents = this.AzureStorage.FetchEntity<Incumbent>(tableName, query);

            return incumbents.ToArray();
        }

        /// <summary>
        /// Gets the call sign array.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="filters">The filters.</param>
        /// <returns>returns String[][].</returns>
        public string[] GetCallSignArray(string tableName, object filters = null)
        {
            TableQuery<DynamicTableEntity> incumbentQuery = new TableQuery<DynamicTableEntity>();
            List<DynamicTableEntity> incumbents = new List<DynamicTableEntity>();

            incumbentQuery.SelectColumns = new[] { "CallSign" };
            if (filters != null)
            {
                incumbentQuery.FilterString = this.AzureStorage.ConvertFiltersToTableQuery(filters);
            }

            incumbents = this.AzureStorage.FetchEntity<DynamicTableEntity>(tableName, incumbentQuery);

            return incumbents.Select(c => c.Properties["CallSign"].StringValue).Distinct().ToArray();
        }

        /// <summary>
        /// Gets the PMSE assignments.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="searchArea">The search area.</param>
        /// <returns>returns Incumbent[][].</returns>
        public PmseAssignment[] GetPMSEAssignments(string tableName, SquareArea searchArea)
        {
            var qs1 = TableQuery.GenerateFilterConditionForDouble("Latitude", QueryComparisons.LessThanOrEqual, searchArea.TopLeftPoint.Latitude);
            var qs3 = TableQuery.GenerateFilterConditionForDouble("Latitude", QueryComparisons.GreaterThanOrEqual, searchArea.BottomRightPoint.Latitude);

            var qs2 = TableQuery.GenerateFilterConditionForDouble("Longitude", QueryComparisons.GreaterThanOrEqual, searchArea.TopLeftPoint.Longitude);
            var qs4 = TableQuery.GenerateFilterConditionForDouble("Longitude", QueryComparisons.LessThanOrEqual, searchArea.BottomRightPoint.Longitude);

            var finalQuery = TableQuery.CombineFilters(qs4, TableOperators.And, TableQuery.CombineFilters(qs2, TableOperators.And, TableQuery.CombineFilters(qs1, TableOperators.And, qs3)));
            TableQuery<PmseAssignment> query = (new TableQuery<PmseAssignment>()).Where(finalQuery);

            var incumbents = this.AzureStorage.FetchEntity<PmseAssignment>(tableName, query);

            return incumbents.ToArray();
        }

        /// <summary>
        /// Gets the PMSE assignments.
        /// </summary>
        /// <param name="table">The table.</param>
        /// <param name="locations">The locations.</param>
        /// <returns>returns Incumbent[][].</returns>
        public PmseAssignment[] GetPMSEAssignments(string table, List<OSGLocation> locations)
        {
            ////var minrowKey = string.Format("{0}-{1}", queryParams.Min(obj => obj.Easting), queryParams.Min(obj => obj.Northing));
            ////var maxrowkey = string.Format("{0}-{1}", queryParams.Max(obj => obj.Easting), queryParams.Max(obj => obj.Northing));

            var minEastingFilter = TableQuery.GenerateFilterConditionForInt("Easting", QueryComparisons.GreaterThanOrEqual, locations.Min(obj => obj.Easting));
            var maxEastingFilter = TableQuery.GenerateFilterConditionForInt("Easting", QueryComparisons.LessThanOrEqual, locations.Max(obj => obj.Easting));
            var minNorthingFilter = TableQuery.GenerateFilterConditionForInt("Northing", QueryComparisons.GreaterThanOrEqual, locations.Min(obj => obj.Northing));
            var maxNorthingFilter = TableQuery.GenerateFilterConditionForInt("Northing", QueryComparisons.LessThanOrEqual, locations.Max(obj => obj.Northing));

            var filter1 = TableQuery.CombineFilters(minEastingFilter, TableOperators.And, TableQuery.CombineFilters(maxEastingFilter, TableOperators.And, TableQuery.CombineFilters(minNorthingFilter, TableOperators.And, maxNorthingFilter)));

            TableQuery<PmseAssignment> query1 = (new TableQuery<PmseAssignment>()).Where(filter1);

            var incumbents = this.AzureStorage.FetchEntity<PmseAssignment>(table, query1);

            return incumbents.ToArray();
        }

        /// <summary>
        /// Gets the antenna pattern for specific incumbents.
        /// </summary>
        /// <param name="incumbentInfo">The incumbent information.</param>
        /// <returns>Returns the antenna patterns</returns>
        public double[] GetAntennaPatterns(Incumbent incumbentInfo)
        {
            var entity = this.AzureStorage.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.CDBSAntennaPatternTableName), new { AntennaId = incumbentInfo.AntennaId });

            var antennaPattern = entity.FirstOrDefault();

            if (antennaPattern == null)
            {
                return null;
            }

            return JsonSerialization.DeserializeString<double[]>(antennaPattern.Properties["Patterns"].StringValue);
        }

        /// <summary>
        /// Gets the specific incumbent.
        /// </summary>
        /// <param name="id">Id of the incumbent to retrieve.</param>
        /// <returns>Returns the specified incumbent (or null if Id does not exist)</returns>
        public Incumbent GetIncumbent(IncumbentId id)
        {
            return null;
        }

        /// <summary>
        /// Updates the specified incumbent.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="incumbent">Incumbent that is to be updated.</param>
        public void UpdateIncumbent(string tableName, Incumbent incumbent)
        {
            this.AzureStorage.UpdateEntity(Utils.GetRegionalTableName(tableName), incumbent);
        }

        /// <summary>
        /// Updates the specified incumbent.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="propertiesRow">The properties row.</param>
        public void UpdateIncumbent(string tableName, SyncRow propertiesRow)
        {
            string logMethodName = "AzureDalc.UpdateIncumbent(" + tableName + ", SyncRow propertiesRow)";

            var regionalTableName = tableName;

            try
            {
                // Begin Log Transaction
                ////this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);

                DynamicTableEntity entity = new DynamicTableEntity();
                entity.PartitionKey = propertiesRow.PartitionKey;
                entity.RowKey = propertiesRow.RowKey;
                entity.ETag = propertiesRow.ETag;
                foreach (var prop in propertiesRow.Columns)
                {
                    entity.Properties.Add(prop.Key, prop.Value);
                }

                this.AzureStorage.MergeEntity(regionalTableName, TableOperation.Merge(entity));

                // End Log transaction
                ////this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName);
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());

                throw;
            }
        }

        /// <summary>
        /// Gets the incumbents inside square area
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="squareArea">The square area.</param>
        /// <param name="columnNames">The column names.</param>
        /// <param name="filters">The filters.</param>
        /// <returns>returns Incumbents.</returns>
        public Incumbent[] GetIncumbents(string tableName, SquareArea squareArea, string[] columnNames = null, object filters = null)
        {
            var qs1 = TableQuery.GenerateFilterConditionForDouble("Latitude", QueryComparisons.LessThanOrEqual, squareArea.TopLeftPoint.Latitude);
            var qs3 = TableQuery.GenerateFilterConditionForDouble("Latitude", QueryComparisons.GreaterThanOrEqual, squareArea.BottomRightPoint.Latitude);

            var qs2 = TableQuery.GenerateFilterConditionForDouble("Longitude", QueryComparisons.GreaterThanOrEqual, squareArea.TopLeftPoint.Longitude);
            var qs4 = TableQuery.GenerateFilterConditionForDouble("Longitude", QueryComparisons.LessThanOrEqual, squareArea.BottomRightPoint.Longitude);

            var finalQuery = TableQuery.CombineFilters(qs4, TableOperators.And, TableQuery.CombineFilters(qs2, TableOperators.And, TableQuery.CombineFilters(qs1, TableOperators.And, qs3)));
            if (filters != null)
            {
                finalQuery = TableQuery.CombineFilters(finalQuery, TableOperators.And, this.AzureStorage.ConvertFiltersToTableQuery(filters));
            }

            TableQuery<Incumbent> query = (new TableQuery<Incumbent>()).Where(finalQuery);

            if (columnNames != null)
            {
                query.SelectColumns = columnNames;
            }

            var incumbents = this.AzureStorage.FetchEntity<Incumbent>(tableName, query);

            return incumbents.ToArray();
        }

        /// <summary>
        /// Gets the LP AUX entities.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="incumbentInfo">The incumbent information.</param>
        /// <param name="squareArea">The square area.</param>
        /// <param name="startDate">The start date.</param>
        /// <returns>returns Incumbents.</returns>
        public Incumbent[] FilterLPAuxEntities(string tableName, Incumbent incumbentInfo, SquareArea squareArea, DateTime startDate)
        {
            // Latitude, Longitude, NEPointLatitude, NEPointLongitude, SWPointLatitude, SWPointLongitude, Channel, StartTime, EndTime)
            var qs1 = TableQuery.GenerateFilterConditionForDouble("Latitude", QueryComparisons.LessThanOrEqual, squareArea.TopLeftPoint.Latitude);
            var qs3 = TableQuery.GenerateFilterConditionForDouble("Latitude", QueryComparisons.GreaterThanOrEqual, squareArea.BottomRightPoint.Latitude);

            var qs2 = TableQuery.GenerateFilterConditionForDouble("Longitude", QueryComparisons.GreaterThanOrEqual, squareArea.TopLeftPoint.Longitude);
            var qs4 = TableQuery.GenerateFilterConditionForDouble("Longitude", QueryComparisons.LessThanOrEqual, squareArea.BottomRightPoint.Longitude);

            var finalQuery = TableQuery.CombineFilters(qs4, TableOperators.And, TableQuery.CombineFilters(qs2, TableOperators.And, TableQuery.CombineFilters(qs1, TableOperators.And, qs3)));
            var dt1 = TableQuery.GenerateFilterConditionForDate("StartTime", QueryComparisons.GreaterThanOrEqual, startDate);

            finalQuery = TableQuery.CombineFilters(finalQuery, TableOperators.And, dt1);

            TableQuery<Incumbent> query = (new TableQuery<Incumbent>()).Where(finalQuery);

            var incumbents = this.AzureStorage.FetchEntity<Incumbent>(tableName, query);

            var qs5 = TableQuery.GenerateFilterConditionForDouble("NEPointLatitude", QueryComparisons.GreaterThanOrEqual, incumbentInfo.Latitude);
            var qs6 = TableQuery.GenerateFilterConditionForDouble("NEPointLongitude", QueryComparisons.LessThanOrEqual, incumbentInfo.Longitude);
            var qs7 = TableQuery.GenerateFilterConditionForDouble("SWPointLatitude", QueryComparisons.LessThanOrEqual, incumbentInfo.Latitude);
            var qs8 = TableQuery.GenerateFilterConditionForDouble("SWPointLongitude", QueryComparisons.GreaterThanOrEqual, incumbentInfo.Longitude);

            finalQuery = TableQuery.CombineFilters(qs5, TableOperators.And, TableQuery.CombineFilters(qs6, TableOperators.And, TableQuery.CombineFilters(qs7, TableOperators.And, qs8)));
            query = (new TableQuery<Incumbent>()).Where(finalQuery);

            incumbents.AddRange(this.AzureStorage.FetchEntity<Incumbent>(tableName, query));

            return incumbents.ToArray();
        }

        /// <summary>
        /// Gets all stations.
        /// </summary>
        /// <param name="squareArea">The square area.</param>
        /// <returns>returns Incumbents.</returns>
        public Incumbent[] GetAllStations(SquareArea squareArea)
        {
            List<Incumbent> stations = new List<Incumbent>();

            stations.AddRange(this.GetIncumbents(Utils.GetRegionalTableName(Constants.CDBSUSMexicoTVEngDataTableName), squareArea));
            stations.AddRange(this.GetIncumbents(Utils.GetRegionalTableName(Constants.ULSBroadcastAuxiliaryTableName), squareArea));

            return stations.ToArray();
        }

        /// <summary>
        /// Adds the incumbent.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="regEntity">Incumbent to add.</param>
        /// <returns>Returns string</returns>
        public string InsertIncumbentData(string tableName, object regEntity)
        {
            string resp = "Success";
            try
            {
                string logMethodName = "AzureDalc.InsertIncumbentData(string tableName, object regEntity)";

                // Begin Log Transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);

                // Route to InsertEntity method in AzureStorageHelper
                TableEntity tableEntity = (TableEntity)regEntity;
                this.AzureStorage.InsertEntity(tableName, tableEntity);

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName);
                return resp;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Inserts the entity.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="tableEntity">The table entity.</param>
        public void InsertEntity(string tableName, DynamicTableEntity tableEntity)
        {
            this.AzureStorage.InsertUpdateDynamicEntity(tableName, tableEntity);
        }

        /// <summary>
        /// Inserts the batch.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="batchOperation">The batch operation.</param>
        public void InsertBatch(string tableName, TableBatchOperation batchOperation)
        {
            this.AzureStorage.InsertUpdateBatch(Utils.GetRegionalTableName(tableName), batchOperation);
        }

        /// <summary>
        /// Gets the unscheduled adjustments.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <returns>returns adjustments.</returns>
        public List<DttData> GetUnscheduledAdjustments(string tableName)
        {
            var adjustments = this.FetchEntity<DTTDataAvailability>(Utils.GetRegionalTableName(tableName), null);
            List<DttData> unscheduledData = new List<DttData>();
            for (int i = 0; i < adjustments.Count; i++)
            {
                DttData objunscheduleddata = new DttData();
                objunscheduleddata.Easting = adjustments[i].Easting;
                objunscheduleddata.Northing = adjustments[i].Northing;
                objunscheduleddata.DataValues = Conversion.ByteToInt(adjustments[i].DataRecord);
                unscheduledData.Add(objunscheduleddata);
            }

            return unscheduledData;
        }

        /// <summary>
        /// Adds the incumbent.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="entity">Incumbent to delete.</param>
        /// <returns>Returns string</returns>
        public string DeleteIncumbentData(string tableName, string entity)
        {
            string resp = "Success";
            try
            {
                string logMethodName = "AzureDalc.DeleteIncumbentData(tableName, entity).";

                // Begin Log Transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);

                List<TableEntity> entities = this.AzureStorage.RetrieveEntity(tableName, entity);
                if (entities.Count != 0)
                {
                    if (tableName == Utils.CurrentRegionPrefix + Constants.MVPDRegistrationTableName)
                    {
                        // Route to DeleteEntity method in AzureStorageHelper
                        foreach (MVPDRegistration tableEntity in entities)
                        {
                            this.AzureStorage.DeleteEntity(tableName, tableEntity);
                        }
                    }
                    else if (tableName == Utils.GetRegionalTableName(Constants.LPAuxRegistrationTable))
                    {
                        // Route to DeleteEntity method in AzureStorageHelper
                        foreach (LPAuxRegistration tableEntity in entities)
                        {
                            this.AzureStorage.DeleteEntity(tableName, tableEntity);
                        }
                    }
                    else if (tableName == Utils.GetRegionalTableName(Constants.LPAuxRegDetails))
                    {
                        // Route to DeleteEntity method in AzureStorageHelper
                        foreach (LPAuxRegistration tableEntity in entities)
                        {
                            this.AzureStorage.DeleteEntity(tableName, tableEntity);
                        }
                    }
                    else if (tableName == Utils.CurrentRegionPrefix + Constants.TempBasRegistrationTableName)
                    {
                        // Route to DeleteEntity method in AzureStorageHelper
                        foreach (TempBASRegistration tableEntity in entities)
                        {
                            this.AzureStorage.DeleteEntity(tableName, tableEntity);
                        }
                    }
                }
                else
                {
                    resp = "No incumbents found";
                }

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName);
                return resp;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Gets the registered incumbents
        /// </summary>
        /// <typeparam name="T">type of incumbent</typeparam>
        /// <param name="incumbentType">Type of the incumbent.</param>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="id">The user id.</param>
        /// <returns>Returns List</returns>
        public List<T> GetIncumbentsData<T>(IncumbentType incumbentType, string tableName, string id) where T : ITableEntity, new()
        {
            Stopwatch watch = new Stopwatch();
            watch.Start();

            List<T> entityList = null;
            ////string userRecords = TableQuery.GenerateFilterCondition("UserId", QueryComparisons.Equal, id);
            ////string registrationId = TableQuery.GenerateFilterCondition("RowKey", QueryComparisons.Equal, id);
            if (id == null)
            {
                entityList = this.AzureStorage.FetchEntity<T>(tableName, new TableQuery<T>());
            }
            else
            {
                entityList = this.AzureStorage.FetchEntity<T>(tableName, new { UserId = id });
            }

            ////if (incumbentType == IncumbentType.MVPD)
            ////{

            ////}
            ////else if (incumbentType == IncumbentType.LPAux)
            ////{
            ////    if (id == null)
            ////    {
            ////        entityList = this.AzureStorage.FetchEntity<T>(tableName, new { Licensed = true }).ToList<TableEntity>();
            ////    }
            ////    else
            ////    {
            ////        entityList = this.AzureStorage.FetchEntity<LPAuxRegistration>(tableName, new { Licensed = true, UserId = id }).ToList<TableEntity>();
            ////    }
            ////}
            ////else if (incumbentType == IncumbentType.TBAS)
            ////{
            ////    if (id == null)
            ////    {
            ////        entityList = this.AzureStorage.FetchEntity<TempBASRegistration>(tableName, null, null).ToList<TableEntity>();
            ////    }
            ////    else
            ////    {
            ////        entityList = this.AzureStorage.FetchEntity<TempBASRegistration>(tableName, new { UserId = id }).ToList<TableEntity>();
            ////    }
            ////}
            ////else if (incumbentType == IncumbentType.UnlicensedLPAux)
            ////{
            ////    if (id == null)
            ////    {
            ////        entityList = this.AzureStorage.FetchEntity<LPAuxRegistration>(tableName, null, null).ToList<TableEntity>();
            ////    }
            ////    else
            ////    {
            ////        entityList = this.AzureStorage.FetchEntity<LPAuxRegistration>(tableName, new { Licensed = false, UserId = id }).ToList<TableEntity>();
            ////    }
            ////}

            watch.Stop();

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, string.Format("IncumbentType :{0}, Query Time : {1}", incumbentType, watch.Elapsed.ToString()));

            return entityList;
        }

        /// <summary>
        /// Gets the protected entity with events.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <returns>returns List{TableEntity}.</returns>
        public List<TableEntity> GetProtectedEntityWithEvents(string tableName)
        {
            List<TableEntity> entityList = new List<TableEntity>();
            if (tableName == Constants.RGN1MVPDRegistrationTableName)
            {
                entityList = this.AzureStorage.FetchEntity<MVPDRegistration>(tableName, null, null).ToList<TableEntity>();
            }
            else if (tableName == Constants.RGN1LPAuxRegistrationTable)
            {
                entityList = this.AzureStorage.FetchEntity<LPAuxRegistration>(tableName, null, null).ToList<TableEntity>();
            }
            else if (tableName == Constants.RGN1TempBasRegistrationTable)
            {
                entityList = this.AzureStorage.FetchEntity<TempBASRegistration>(tableName, null, null).ToList<TableEntity>();
            }
            else if (tableName == Constants.RGN1FixedTVBDRegistrationTable)
            {
                entityList = this.AzureStorage.FetchEntity<FixedTVBDRegistration>(tableName, null, null).ToList<TableEntity>();
            }

            return entityList;
        }

        /// <summary>
        /// Gets the protected entity with events.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <returns>returns List{TableEntity}.</returns>
        public List<TableEntity> GetProtectedEntity(string tableName)
        {
            List<TableEntity> entityList = new List<TableEntity>();
            if (tableName == Utils.GetRegionalTableName(Constants.ULSBroadcastAuxiliaryTableName))
            {
                entityList = this.AzureStorage.FetchEntity<ULSRecord>(tableName, null, null).ToList<TableEntity>();
            }
            else if (tableName == Utils.GetRegionalTableName(Constants.ULSPLCMRSTableName))
            {
                entityList = this.AzureStorage.FetchEntity<ULSRecord>(tableName, null, null).ToList<TableEntity>();
            }
            else if (tableName == Utils.GetRegionalTableName(Constants.CDBSTranslatorDataTableName))
            {
                entityList = this.AzureStorage.FetchEntity<CDBSTvEngData>(tableName, null, null).ToList<TableEntity>();
            }
            else if (tableName == Utils.GetRegionalTableName(Constants.CDBSCanadaTvEngDataTableName))
            {
                entityList = this.AzureStorage.FetchEntity<CDBSTvEngData>(tableName, null, null).ToList<TableEntity>();
            }

            return entityList;
        }

        /// <summary>
        /// Gets the protected entity US MEXICO.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="entityType">Type of the entity.</param>
        /// <returns>returns List{TableEntity}.</returns>
        public List<TableEntity> GetProtectedEntityUSMexico(string tableName, string entityType)
        {
            List<TableEntity> entityList = new List<TableEntity>();
            string entityTyp = TableQuery.GenerateFilterCondition("EntityType", QueryComparisons.Equal, entityType);
            TableQuery<CDBSTvEngData> query = new TableQuery<CDBSTvEngData>().Where(entityTyp);
            entityList = this.AzureStorage.FetchEntity<CDBSTvEngData>(tableName, query).ToList<TableEntity>();
            return entityList;
        }

        /// <summary>
        /// Adds the excluded id.
        /// </summary>
        /// <param name="entity">Id to add.</param>
        /// <returns>Returns string</returns>
        public string ExcludeId(DynamicTableEntity entity)
        {
            string resp = "Success";
            try
            {
                string logMethodName = "AzureDalc.ExcludeId(DynamicTableEntity entity)";

                // Begin Log Transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);

                // Route to InsertUpdateDynamicEntity method in AzureStorageHelper
                this.AzureStorage.InsertUpdateDynamicEntity(Utils.GetRegionalTableName(Constants.ExcludedIds), entity);

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName);
                return resp;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Adds the excluded region.
        /// </summary>
        /// <param name="entity">region to add.</param>
        /// <returns>Returns string</returns>
        public string ExcludeChannel(DynamicTableEntity entity)
        {
            string resp = "Success";
            try
            {
                string logMethodName = "AzureDalc.ExcludeChannel(DynamicTableEntity entity)";

                // Begin Log Transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);

                // Route to InsertUpdateDynamicEntity method in AzureStorageHelper
                this.AzureStorage.InsertUpdateDynamicEntity(Utils.GetRegionalTableName(Constants.ExcludedChannels), entity);

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName);
                return resp;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
                throw;
            }
        }

        #endregion

        /// <summary>
        /// Gets the list of devices which match the given latitude and longitude
        /// </summary>
        /// <param name="tableName">The table name.</param>
        /// <param name="finalQuery">The final query.</param>
        /// <returns>returns Device info from the table.</returns>
        public List<UsedSpectrum> GetUsedSpectrumDevices(string tableName, TableQuery<UsedSpectrum> finalQuery)
        {
            return this.AzureStorage.FetchEntity<UsedSpectrum>(Utils.CurrentRegionPrefix + Constants.SpectrumUsageTable, finalQuery);
        }

        /// <summary>
        /// Retrieve the cloud blob data container
        /// </summary>
        /// <param name="dataContainer">The blob storage folder that contains all of the elevation files.</param>
        /// <returns>Cloud Blob Container</returns>
        public CloudBlobContainer GetBlobContainer(string dataContainer)
        {
            return this.AzureStorage.GetCloudBlobContainer(dataContainer);
        }

        /// <summary>
        /// Returns all of the LPAUX Registrations.
        /// </summary>
        /// <returns>Returns an array of LPAUX unlicensed Registration</returns>
        public List<LPAuxRegistration> GetLPAuxRegistrations()
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - " + "AzureDalc.GetLPAuxRegistration");
            TableQuery<LPAuxRegistration> query;
            string filterconditionWSDBA = TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, Utils.Configuration[Constants.ConfigSettingWSDBAName]);
            string filterconditionUnLicensed = TableQuery.GenerateFilterConditionForBool("Licensed", QueryComparisons.Equal, false);
            string finalCondition = TableQuery.CombineFilters(filterconditionWSDBA, TableOperators.And, filterconditionUnLicensed);
            query = new TableQuery<LPAuxRegistration>().Where(finalCondition);
            return this.AzureStorage.FetchEntity<LPAuxRegistration>(Utils.GetRegionalTableName(Constants.LPAuxRegistrationTable), query);
        }

        /// <summary>
        /// Returns LP-AUX Registrations for the given id.
        /// </summary>
        /// <param name="id">LP-AUX Registration ID</param>
        /// <returns>Returns LP-AUX unlicensed registration.</returns>
        public List<LPAuxRegistration> GetLPAuxRegistration(string id)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - GetLPAuxRegistration(string id) :" + "AzureDalc.FetchEntity Lp-AuxRegistration");
            return this.AzureStorage.FetchEntity<LPAuxRegistration>(Utils.GetRegionalTableName(Constants.LPAuxRegistrationTable), Utils.Configuration[Constants.ConfigSettingWSDBAName], id);
        }

        /// <summary>
        /// Save the LP-AUX unlicensed Registration
        /// </summary>
        /// <param name="unlicensedPointAuxRegistration"> Register LP Aux Unlicensed Registration.</param>
        public void RegisterDevice(LPAuxRegistration unlicensedPointAuxRegistration)
        {
            LPAuxRegistration[] lpauxReg = new LPAuxRegistration[1];
            lpauxReg[0] = unlicensedPointAuxRegistration;
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - RegisterDevice :" + "AzureDalc.Update Lp-AuxRegistration");
            ////this.AzureStorage.TableExecuteBatch(Utils.CurrentRegionName + Utils.Configuration.CurrentRegionId + Constants.LPAuxRegistrationTable, this.PrepareTableBatch(lpauxReg)[0]);
            this.SaveLPAuxRegistrationDetailsTable(lpauxReg);
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "End - RegisterDevice :" + "AzureDalc.Update Lp-AuxRegistration");
        }

        /// <summary>
        /// cancel the LPAUX unlicensed registration
        /// </summary>
        /// <param name="id">registration id</param>
        public void CancelRegistrations(string id)
        {
            List<LPAuxRegistration> lpauxReg = this.GetLPAuxRegistration(id);
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Begin - CancelRegistrations :" + "AzureStorage.DeleteEntity Lp-AuxRegistration");
            this.AzureStorage.DeleteEntity(Utils.GetRegionalTableName(Constants.LPAuxRegistrationTable), lpauxReg[0]);
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "End - CancelRegistrations :" + "AzureStorage.DeleteEntity Lp-AuxRegistration");
        }

        /// <summary>
        /// Retrieve the incumbents
        /// </summary>
        /// <param name="squareArea">The square Area.</param>
        /// <returns>returns Incumbents</returns>
        public Incumbent[] GetIncumbents(SquareArea squareArea)
        {
            var qs1 = TableQuery.GenerateFilterConditionForDouble("Latitude", QueryComparisons.LessThanOrEqual, squareArea.TopLeftPoint.Latitude);
            var qs3 = TableQuery.GenerateFilterConditionForDouble("Latitude", QueryComparisons.GreaterThanOrEqual, squareArea.BottomRightPoint.Latitude);

            var qs2 = TableQuery.GenerateFilterConditionForDouble("Longitude", QueryComparisons.GreaterThanOrEqual, squareArea.TopLeftPoint.Longitude);
            var qs4 = TableQuery.GenerateFilterConditionForDouble("Longitude", QueryComparisons.LessThanOrEqual, squareArea.BottomRightPoint.Longitude);

            var finalQuery = TableQuery.CombineFilters(qs4, TableOperators.And, TableQuery.CombineFilters(qs2, TableOperators.And, TableQuery.CombineFilters(qs1, TableOperators.And, qs3)));
            TableQuery<Incumbent> query = (new TableQuery<Incumbent>()).Where(finalQuery);

            var incumbents = this.AzureStorage.FetchEntity<Incumbent>(Utils.GetRegionalTableName(Constants.SpectrumUsageTable), query);

            return incumbents.ToArray();
        }

        /// <summary>
        /// Inserts the file data to database.
        /// </summary>
        /// <param name="pmseData">The PMSE data.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool InsertFileDataToDatabase(List<PmseAssignment> pmseData)
        {
            try
            {
                List<TableOperation> tableOperations = new List<TableOperation>();
                foreach (PmseAssignment pd in pmseData)
                {
                    tableOperations.Add(TableOperation.InsertOrReplace(pd));
                }

                this.AzureStorage.InsertUpdateBatch(Utils.CurrentRegionName + Utils.Configuration.CurrentRegionId + Constants.PMSEAssignmentsTable, tableOperations);
                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        /// <summary>
        /// Inserts the file data to database.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="pmseData">The PMSE data.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool InsertFileDataToDatabase(string tableName, List<PmseAssignment> pmseData)
        {
            try
            {
                List<TableOperation> tableOperations = new List<TableOperation>();
                foreach (PmseAssignment pd in pmseData)
                {
                    tableOperations.Add(TableOperation.InsertOrReplace(pd));
                }

                this.AzureStorage.InsertUpdateBatch(Utils.CurrentRegionName + Utils.Configuration.CurrentRegionId + tableName, tableOperations);
                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        /// <summary>
        /// Inserts the DTT file data.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="batchOperation">The batch operation.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool InsertDTTFileData(string tableName, TableBatchOperation batchOperation)
        {
            try
            {
                this.AzureStorage.InsertUpdateBatchTerrainData(tableName, batchOperation);
                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        /// <summary>
        /// Inserts the DTT file data.
        /// </summary>
        /// <param name="dttData">The DTT data.</param>
        /// <param name="tableName">Name of the table.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool InsertDTTFileData(List<DTTDataAvailability> dttData, string tableName)
        {
            try
            {
                List<TableOperation> tableOperations = new List<TableOperation>();
                foreach (DTTDataAvailability pd in dttData)
                {
                    tableOperations.Add(TableOperation.InsertOrReplace(pd));
                }

                this.AzureStorage.InsertUpdateBatchTerrainData(tableName, tableOperations);
                return true;
            }
            catch (Exception)
            {
                throw;
            }
        }

        /// <summary>
        /// Inserts the DTT file data.
        /// </summary>
        /// <param name="dttData">The DTT data.</param>
        /// <param name="tableName">Name of the table.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool InsertPMSEUnscheduledAdjustment(List<DTTDataAvailability> dttData, string tableName)
        {
            try
            {
                List<TableOperation> tableOperations = new List<TableOperation>();
                foreach (DTTDataAvailability pd in dttData)
                {
                    tableOperations.Add(TableOperation.InsertOrReplace(pd));
                }

                this.AzureStorage.InsertUpdateBatchPMSEUnscheduledAdjustment(tableName, tableOperations);
                return true;
            }
            catch (Exception)
            {
                throw;
            }
        }

        /// <summary>
        /// Moves the blobs DTT synchronize.
        /// </summary>
        /// <param name="blobName">Name of the BLOB.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool MoveBlobsDttSync(string blobName)
        {
            try
            {
                string sourceContainer = Utils.Configuration["DttSyncSourceContainer"];
                string targetContainer = Utils.Configuration["DttSyncArchiveContainer"];
                return this.AzureStorage.MoveBlob(sourceContainer, blobName, targetContainer);
            }
            catch (Exception)
            {
                throw;
            }
        }

        /// <summary>
        /// Return Settings Date
        /// </summary>
        /// <returns>Settings Date</returns>
        public DateTime GetDateSequenceFromSettingsTable()
        {
            string currentSettingsDate = this.FetchEntity<Settings>(Utils.GetRegionalTableName(Constants.SettingsTable), new { RowKey = Constants.SettingsDateKey })[0].ConfigValue;

            DateTime settingsDate = DateTime.Parse(currentSettingsDate, CultureInfo.InvariantCulture);
            if (settingsDate.Date != DateTime.Now.Date || settingsDate.Month != DateTime.Now.Month || settingsDate.Year != DateTime.Now.Year)
            {
                Settings settings = new Settings();
                settings.PartitionKey = Constants.SettingsPartitionKey;
                settings.RowKey = Constants.SettingsDateKey;
                settings.ConfigValue = DateTime.Now.Date.ToString(Utils.Configuration[Constants.DateFormat], DateTimeFormatInfo.InvariantInfo);
                this.UpdateSettingsDateSequence(settings);
                Settings settingsSequence = new Settings();
                settingsSequence.PartitionKey = Constants.SettingsPartitionKey;
                settingsSequence.RowKey = Constants.SettingsSequenceKey;
                settingsSequence.ConfigValue = "0";
                this.UpdateSettingsDateSequence(settingsSequence);
            }

            settingsDate = DateTime.Now;
            return settingsDate;
        }

        /// <summary>
        /// Return Settings Date
        /// </summary>
        /// <returns>Settings Date</returns>
        public int GetSequenceFromSettingsTable()
        {
            int sequenceNumber = Convert.ToInt32(this.FetchEntity<Settings>(Utils.CurrentRegionPrefix + Constants.SettingsTable, new { RowKey = Constants.SettingsSequenceKey })[0].ConfigValue) + 1;
            Settings seqSettings = new Settings();
            seqSettings.PartitionKey = Constants.SettingsPartitionKey;
            seqSettings.RowKey = Constants.SettingsSequenceKey;
            seqSettings.ConfigValue = sequenceNumber.ToString();
            this.UpdateSettingsDateSequence(seqSettings);
            return sequenceNumber;
        }

        /// <summary>
        /// Uploads the All Registrations file to the cloud storage
        /// </summary>
        /// <param name="filename"> file to be uploaded</param>
        /// <param name="containerName"> container name to which file to be uploaded</param>
        /// <param name="blobName">Name of the blob</param>
        public void UploadAllRegistrationsFile(string filename, string containerName, string blobName)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncFileWorkerGenericMessage, "Begin - Dalc - UploadAllRegistrationsFile - All File");
            try
            {
                // upload new blob
                this.AzureStorage.UploadBlobToStorage(filename, containerName, blobName);

                // Move blobs to another container.
                this.AzureStorage.MoveBlobs(containerName, blobName, Utils.Configuration["ALLFileBlobContainerArchive"]);
            }
            catch
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncFileWorkerGenericMessage, "UploadAllRegistrationsFile-DALC- Exceptions");
                throw;
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncFileWorkerGenericMessage, "End - Dalc - UploadAllRegistrationsFile - All File");
        }

        /// <summary>
        /// Uploads the All Registrations file to the cloud storage
        /// </summary>
        /// <param name="filename"> file to be uploaded</param>
        /// <param name="containerName"> container name to which file to be uploaded</param>
        /// <param name="blobName">Name of the blob</param>
        public void UploadINCRegistrationsFile(string filename, string containerName, string blobName)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncFileWorkerGenericMessage, "Begin - Dalc - UploadINCRegistrationsFile - Incremental File");
            try
            {
                this.AzureStorage.UploadBlobToStorage(filename, containerName, blobName);
            }
            catch
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DBSyncFileWorkerGenericMessage, "UploadINCRegistrationsFile-DALC- Exceptions");
                throw;
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DBSyncFileWorkerGenericMessage, "End - Dalc - UploadINCRegistrationsFile - Incremental File");
        }

        /// <summary>
        /// Deletes the specified user from the database.
        /// </summary>
        /// <param name="userId">The user identifier.</param>
        /// <param name="authorizedUser">The authorized user.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool DeleteUser(string userId, string authorizedUser)
        {
            string logMethodName = "AzureDalc.DeleteUser(string userID)";

            // Begin Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);
            try
            {
                User user = this.GetUser(userId);
                if (user != null)
                {
                    user.Deleted = true;
                    List<TableOperation> tableOperations = null;

                    tableOperations = new List<TableOperation>();
                    tableOperations.Add(TableOperation.Merge(user));
                    this.AzureStorage.InsertUpdateBatch(Constants.AuthorizedUsersTable, tableOperations);
                    tableOperations = new List<TableOperation>();
                    foreach (RegionAccess ra in user.Access)
                    {
                        ra.Deleted = true;
                        tableOperations.Add(TableOperation.Merge(ra));
                    }

                    this.AzureStorage.InsertUpdateBatch(Constants.RegionAccessTable, tableOperations);
                    return true;
                }
                else
                {
                    return false;
                }
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Grants the access.
        /// </summary>
        /// <param name="elevatedAccessRequest">The elevated access request.</param>
        /// <param name="authorizedUser">The authorized user.</param>
        /// <returns>returns String</returns>
        public string GrantAccess(ElevatedAccessRequest elevatedAccessRequest, string authorizedUser)
        {
            string logMethodName = "AzureDalc.RequestElevatedAccess(elevatedAccessRequest)";

            // Begin Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);
            try
            {
                var qs1 = TableQuery.GenerateFilterCondition("UserId", QueryComparisons.Equal, elevatedAccessRequest.UserId);
                var qs3 = TableQuery.GenerateFilterCondition("AccessLevelString", QueryComparisons.Equal, elevatedAccessRequest.AccessLevelString);
                var qs2 = TableQuery.GenerateFilterCondition("Region", QueryComparisons.Equal, elevatedAccessRequest.Region);
                var qs4 = TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, elevatedAccessRequest.UserId);
                var finalQuery = TableQuery.CombineFilters(qs2, TableOperators.And, TableQuery.CombineFilters(qs1, TableOperators.And, qs3));

                TableQuery<ElevatedAccessRequest> query = (new TableQuery<ElevatedAccessRequest>()).Where(finalQuery);

                ElevatedAccessRequest[] entities = this.AzureStorage.FetchEntity<ElevatedAccessRequest>(Constants.ElevateAccessTable, query).ToArray();

                entities.ElementAt(0).Status = "Granted";
                entities.ElementAt(0).UpdatedBy = authorizedUser;
                entities.ElementAt(0).UpdatedOn = DateTime.Now;
                List<TableOperation> tableOperations = null;

                tableOperations = new List<TableOperation>();
                tableOperations.Add(TableOperation.Merge(entities.ElementAt(0)));
                this.AzureStorage.InsertUpdateBatch(Constants.ElevateAccessTable, tableOperations);

                var filterQuery = TableQuery.CombineFilters(qs2, TableOperators.And, qs4);
                TableQuery<RegionAccess> elevatedAccessQuery = (new TableQuery<RegionAccess>()).Where(filterQuery);
                RegionAccess[] userAccessLevel = this.AzureStorage.FetchEntity<RegionAccess>(Constants.RegionAccessTable, elevatedAccessQuery).ToArray();

                userAccessLevel.ElementAt(0).AccessLevelString = elevatedAccessRequest.AccessLevelString;

                tableOperations = new List<TableOperation>();
                tableOperations.Add(TableOperation.Merge(userAccessLevel.ElementAt(0)));
                this.AzureStorage.InsertUpdateBatch(Constants.RegionAccessTable, tableOperations);

                User user = this.GetUser(elevatedAccessRequest.UserId);
                user.UpdatedOn = DateTime.Now;
                user.UpdatedBy = authorizedUser;
                this.AzureStorage.UpdateUser(Constants.AuthorizedUsersTable, user);
                return elevatedAccessRequest.UserId;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
                throw;
            }
        }

        #region Implementation of IDalcServiceCacheHelper

        /// <summary>
        /// Fetches the Date or Sequence.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <returns>Entity Name.</returns>
        public List<CacheObjectTvEngdata> FetchTvEngData(string tableName)
        {
            var data = this.AzureStorage.FetchEntity(Utils.GetRegionalTableName(tableName), new TableQuery<CacheObjectTvEngdata>());
            return data;
        }

        /// <summary>
        /// Updates the table.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <returns>returns success code.</returns>
        public List<LPAuxRegistration> FetchLpAuxAregistrations(string tableName)
        {
            var data = this.AzureStorage.FetchEntity(Utils.GetRegionalTableName(tableName), new TableQuery<LPAuxRegistration>());
            return data;
        }

        /// <summary>
        /// Fetches the PMSE assignments.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <returns>returns PMSE list.</returns>
        public List<PmseAssignment> FetchPmseAssignments(string tableName)
        {
            var data = this.AzureStorage.FetchEntity(Utils.GetRegionalTableName(tableName), new TableQuery<PmseAssignment>());
            return data;
        }

        /// <summary>
        /// Fetches the RegionPolygons collections.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="partitionKey">The partitionKey indicating region Country region code.</param>
        /// <returns>Collection of RegionPolygonCache.</returns>
        public List<RegionPolygonsCache> FetchRegionPolygons(string tableName, string partitionKey)
        {
            List<RegionPolygonsCache> regionPolygons = new List<RegionPolygonsCache>();

            List<RegionPolygon> regionPolygonEntities = this.AzureStorage.FetchEntity<RegionPolygon>(tableName, partitionKey);

            foreach (RegionPolygon regionPolygonEntity in regionPolygonEntities)
            {
                string[] blobUriBreakup = regionPolygonEntity.PolygonsUri.Split('/');
                string blobName = blobUriBreakup[blobUriBreakup.Length - 1];
                string containerName = blobUriBreakup[blobUriBreakup.Length - 2];

                string blobContent = this.AzureStorage.Download(blobName, containerName);

                List<List<Location>> polygonsCollection = new List<List<Location>>();

                foreach (List<Location> locations in GeoCalculations.ParseRegionPolygons(blobContent))
                {
                    polygonsCollection.Add(locations);
                }

                IEnumerable<LocationRect> locationRectangles = GeoCalculations.ParseLocationRectangles(regionPolygonEntity.LocationRectangles);

                LocationRect locationRect = new LocationRect(regionPolygonEntity.MaxLatitude, regionPolygonEntity.MinLongitude, regionPolygonEntity.MinLatitude, regionPolygonEntity.MaxLongitude);

                RegionPolygonsCache regionPolygonCache = new RegionPolygonsCache(regionPolygonEntity.RegionName, locationRect, polygonsCollection, locationRectangles);

                regionPolygonCache.LastModifiedTime = regionPolygonEntity.Timestamp.DateTime;

                regionPolygons.Add(regionPolygonCache);
            }

            return regionPolygons;
        }

        #endregion

        /// <summary>
        /// Insert Portal contour records in to Portal Summary and Portal contours tables
        /// </summary>
        /// <param name="portalContours">records to be inserted</param>
        /// <param name="incumbentType">incumbent type</param>
        public void UpdatePortalContoursAndSummary(List<PortalContour> portalContours, int incumbentType)
        {
            try
            {
                string logMethodName = "AzureDalc.UpdatePortalContoursAndSummary";

                // Begin Log Transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName + " for incumbent type " + ((IncumbentType)incumbentType).ToString());

                this.UpdatePortalTable(portalContours, Constants.PortalContoursTable, incumbentType);

                // Generate SummaryRecords
                List<PortalContour> summaryContours = new List<PortalContour>();

                foreach (var portalContour in portalContours)
                {
                    // Truncate to zero
                    int latValue = (int)portalContour.Latitude;
                    int longValue = (int)portalContour.Longitude;

                    for (int i = latValue - 1; i <= latValue + 1; i++)
                    {
                        for (int j = longValue - 1; j <= longValue + 1; j++)
                        {
                            portalContour.PartitionKey = string.Format("RGN1-Lat{0}Long{1}", i, j);

                            summaryContours.Add(new PortalContour
                            {
                                PartitionKey = portalContour.PartitionKey,
                                RowKey = portalContour.RowKey,
                                Latitude = portalContour.Latitude,
                                Longitude = portalContour.Longitude,
                                ParentLatitude = portalContour.ParentLatitude,
                                ParentLongitude = portalContour.ParentLongitude,
                                Contour = portalContour.Contour,
                                CallSign = portalContour.CallSign,
                                Channel = portalContour.Channel,
                                Type = portalContour.Type
                            });
                        }
                    }
                }

                this.UpdatePortalTable(summaryContours, Constants.PortalSummaryTable, incumbentType);

                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName + " for incumbent type " + ((IncumbentType)incumbentType).ToString());
            }
            catch (Exception e)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, e.ToString());
                throw;
            }
        }

        private void UpdatePortalTable(List<PortalContour> contourRecords, string tableName, int incumbentType)
        {
            try
            {
                // Begin Log Transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, tableName + " update  for incumbent type " + ((IncumbentType)incumbentType).ToString() + " started");

                // Get MVPD registrations 
                string likeString = incumbentType + "-";
                List<PortalContour> existingRecords = this.AzureStorage.QueryEntities<PortalContour>(tableName, x => x.RowKey.IndexOf(likeString) == 0);
                List<PortalContour> newRecords = new List<PortalContour>();

                if (existingRecords.Count > 0)
                {
                    var deletedRecords = existingRecords.Where(obj => contourRecords.FirstOrDefault(obj2 => obj2.RowKey == obj.RowKey) == null).ToList();
                    var commonRecords = contourRecords.Where(obj => existingRecords.FirstOrDefault(obj2 => obj2.RowKey == obj.RowKey) != null).ToList();

                    // Delete inactive records
                    foreach (var record in deletedRecords)
                    {
                        this.AzureStorage.DeleteEntity(tableName, record);
                    }

                    // update common records
                    foreach (var record in commonRecords)
                    {
                        record.ETag = "*";
                        this.AzureStorage.MergeEntity(tableName, TableOperation.Merge(record));
                    }

                    newRecords = contourRecords.Where(obj => existingRecords.FirstOrDefault(obj2 => obj2.RowKey == obj.RowKey) == null).ToList();
                }
                else
                {
                    newRecords = contourRecords;
                }

                // Insert new records
                foreach (var record in newRecords)
                {
                    this.AzureStorage.InsertEntity(tableName, record);
                }

                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, tableName + " update  for incumbent type " + ((IncumbentType)incumbentType).ToString() + " ended");
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, "Error :" + tableName + " update  for incumbent type " + ((IncumbentType)incumbentType).ToString() + " failed " + ex.ToString());
                throw;
            }
        }
    }
}
