// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AdminNotificationSystem
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading.Tasks;
    using Microsoft.WhiteSpaces.AdminNotificationSystem;
    using Microsoft.WhiteSpaces.AzureTableAccess;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;

    public class AuditTracker : NotificationSystem
    {
        private const int ErrorStatusCode = 0;

        private const string AuditIdKey = "AuditId";
        private const string MessageKey = "Message";
        private const string TransactionIdKey = "TransactionID";
        private const string Type = "AuditNotificationSystem";

        private static object lockObj = new object();

        private readonly IAzureTableOperation azureTableOperation;
        private readonly int[] regionIdList;

        public AuditTracker(INotificationClient notificationClient, IAzureTableOperation azureTableOperation, int[] regionIdList)
            : base(notificationClient, AuditTracker.Type)
        {
            if (azureTableOperation == null)
            {
                throw new ArgumentNullException("azureTableOperation");
            }

            if (regionIdList == null)
            {
                throw new ArgumentNullException("regionIdList");
            }

            this.azureTableOperation = azureTableOperation;
            this.regionIdList = regionIdList;
        }

        public AuditTracker(INotificationClient notificationClient, IAzureTableOperation azureTableOperation, double notificationInterval, int[] regionIdList)
            : base(notificationClient, notificationInterval, AuditTracker.Type)
        {
            if (azureTableOperation == null)
            {
                throw new ArgumentNullException("azureTableOperation");
            }

            if (regionIdList == null)
            {
                throw new ArgumentNullException("regionIdList");
            }

            this.azureTableOperation = azureTableOperation;
            this.regionIdList = regionIdList;
        }

        protected string TableName
        {
            get
            {
                return "Audit";
            }
        }

        public override void Notify(DateTime signalTime)
        {
            if (this.regionIdList.Length == 0)
            {
                System.Diagnostics.Trace.TraceWarning("There are no entries in the regionIdList");
                return;
            }

            IEnumerable<TableQuery<DynamicTableEntity>> tableQueryList = this.GetTableQueries(signalTime.ToUniversalTime());
            IEnumerable<AuditLog> auditLogs = this.GetAuditLogs(tableQueryList);

            if (auditLogs != null && auditLogs.Any())
            {
                Notification auditNotification = new AuditLogData(auditLogs, AuditTracker.Type, this.TrackingTimeStart, signalTime.ToUniversalTime());

                // boolean status would be helpful if any further processing needs to performed based on the status of notification delivery .
                this.NotificationClient.NotifyClient(auditNotification);
            }
        }

        private IEnumerable<AuditLog> GetAuditLogs(IEnumerable<TableQuery<DynamicTableEntity>> tableQueries)
        {
            List<AuditLog> auditLogs = new List<AuditLog>();

            try
            {
                Parallel.ForEach(
                tableQueries,
                (tableQuery) =>
                {
                    IEnumerable<DynamicTableEntity> tableEntityList = this.azureTableOperation.GetTableEntityProjection(this.TableName, tableQuery);

                    foreach (var tableEntity in tableEntityList)
                    {
                        lock (lockObj)
                        {
                            auditLogs.Add(
                                new AuditLog(
                                    tableEntity[AuditTracker.AuditIdKey].Int32Value.Value,
                                    tableEntity[AuditTracker.MessageKey].StringValue,
                                    tableEntity[AuditTracker.TransactionIdKey].StringValue));
                        }
                    }
                });
            }
            catch (System.AggregateException ex)
            {
                if (!(ex.InnerException is StorageException))
                {
                    // Something wrong with the code, so throw the exception.
                    throw;
                }

                System.Diagnostics.Trace.TraceError(ex.ToString());
            }

            return auditLogs;
        }

        private IEnumerable<TableQuery<DynamicTableEntity>> GetTableQueries(DateTime signalTime)
        {
            List<TableQuery<DynamicTableEntity>> tableQueryList = new List<TableQuery<DynamicTableEntity>>();

            for (int regionIndex = 0; regionIndex < this.regionIdList.Length; regionIndex++)
            {
                TableQuery<DynamicTableEntity> tableQuery = new TableQuery<DynamicTableEntity>();
                tableQuery.SelectColumns = new List<string> { AuditTracker.AuditIdKey, AuditTracker.MessageKey, AuditTracker.TransactionIdKey };

                string regionPartitionKey = this.PartitionKeyFromDateTime(this.regionIdList[regionIndex], this.TrackingTimeStart);

                tableQuery = TableAzureQueryHelper.AddCondition(tableQuery, TableOperators.And, TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, regionPartitionKey));

                if (this.TrackingTimeStart.DayOfYear != signalTime.DayOfYear)
                {
                    // TODO: Case to handle start time belong a one date and end time belongs to next day data.
                    string regionPartitionKeyForEndTime = this.PartitionKeyFromDateTime(this.regionIdList[regionIndex], signalTime);

                    tableQuery = TableAzureQueryHelper.AddCondition(tableQuery, TableOperators.Or, TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, regionPartitionKeyForEndTime));
                }

                tableQuery = TableAzureQueryHelper.AddCondition(tableQuery, TableOperators.And, TableQuery.GenerateFilterConditionForInt("Status", QueryComparisons.Equal, AuditTracker.ErrorStatusCode));
                tableQuery = TableAzureQueryHelper.AddCondition(tableQuery, TableOperators.And, TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.GreaterThanOrEqual, this.TrackingTimeStart));
                tableQuery = TableAzureQueryHelper.AddCondition(tableQuery, TableOperators.And, TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.LessThanOrEqual, signalTime));

                tableQueryList.Add(tableQuery);
            }

            return tableQueryList;
        }

        private string PartitionKeyFromDateTime(int regionCode, DateTime dateTime)
        {
            string partitionKey = string.Format("{0}-{1}", regionCode, dateTime.ToString("yyyyMMdd"));

            return partitionKey;
        }       
    }
}
