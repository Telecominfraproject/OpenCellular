// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Dalc
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Linq;
    using System.Threading;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using Microsoft.WindowsAzure.Storage.Table;
    using Microsoft.WindowsAzure.Storage.Table.Protocol;
using System.Linq.Expressions;


    /// <summary>Azure storage helper class is used to to cloud operations.</summary>
    public class AzureStorageHelper
    {
        /// <summary>holds the instance of Cloud Storage Account.</summary>
        private CloudStorageAccount storageAccount;

        /// <summary>holds the instance of Cloud Storage Account.</summary>
        private CloudStorageAccount storageAccountTerrain;

        /// <summary>holds the instance of table.</summary>
        private CloudTable table;

        /// <summary>private variable to have user id</summary>
        private string userId = Utils.UserId;

        /// <summary>Initializes a new instance of the AzureStorageHelper class.</summary>
        /// <param name="logger">ILogger Logger</param>
        public AzureStorageHelper(ILogger logger)
        {
            this.Logger = logger;
        }

        /// <summary>Gets or sets ILogger Interface</summary>
        [Dependency]
        public ILogger Logger { get; set; }

        /// <summary>Gets or sets the transactionId.</summary>
        public Guid TransactionId { get; set; }

        /// <summary>Inserts Table Entity to cloud Storage table</summary>
        /// <param name="entity">Table entity to be inserted.</param>
        /// <param name="entityName">Table entity  name to be inserted.</param>
        public void InsertCloudStorageEntity(TableEntity entity, string entityName)
        {
            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            CloudTableClient tableClient = this.storageAccount.CreateCloudTableClient();

            //// Create the CloudTable object that represents the table.
            CloudTable table = tableClient.GetTableReference(entityName);
            table.CreateIfNotExists();

            //// Create the TableOperation that inserts the customer entity.
            TableOperation insertOperation = TableOperation.Insert(entity);

            //// Execute the insert operation.
            table.Execute(insertOperation);

            //// cleaning objects
            table = null;
            tableClient = null;
            insertOperation = null;
        }

        /// <summary>Inserts entity into table</summary>
        /// <param name="tableName">Azure Table information</param>
        /// <param name="entity">Azure entity information</param>
        public void InsertEntity(string tableName, TableEntity entity)
        {
            string logMethodName = "AzureStorageHelper.Execute(insertOperation)";

            // Begin Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);

            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            CloudTableClient tableClient = this.storageAccount.CreateCloudTableClient();
            this.table = tableClient.GetTableReference(tableName);
            this.table.CreateIfNotExists();
            TableOperation insertOperation = TableOperation.InsertOrMerge(entity);

            // Executes insert operation
            this.table.Execute(insertOperation);

            // End Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName);

            // cleaning objects
            this.table = null;
            tableClient = null;
            insertOperation = null;
        }

        /// <summary>Delete entity from table</summary>
        /// <param name="tableName">Azure Table information</param>
        /// <param name="entity">Azure entity information</param>
        public void DeleteEntity(string tableName, TableEntity entity)
        {
            string logMethodName = "AzureStorageHelper.DeleteEntity(string tableName, TableEntity entity)";

            // Begin Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);

            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            CloudTableClient tableClient = this.storageAccount.CreateCloudTableClient();
            this.table = tableClient.GetTableReference(tableName);
            TableOperation deleteOperation = TableOperation.Delete(entity);
            this.table.CreateIfNotExists();

            // Executes insert operation
            this.table.Execute(deleteOperation);

            // End Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName);

            // cleaning objects
            this.table = null;
            tableClient = null;
            deleteOperation = null;
        }

        /// <summary>Table Batch Operations saving to Azure Table.</summary>
        /// <param name="tableName">Azure Table Name</param>
        /// <param name="tableBatchOperation">TableBatchOperation object</param>
        public void TableExecuteBatch(string tableName, TableBatchOperation tableBatchOperation)
        {
            string logMethodName = "AzureStorageHelper.Execute(insertOperation)";

            // Begin Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);

            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            CloudTableClient tableClient = this.storageAccount.CreateCloudTableClient();
            this.table = tableClient.GetTableReference(tableName);
            this.table.CreateIfNotExists();

            // Executes batch insert operation
            this.table.ExecuteBatch(tableBatchOperation);

            // End Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Exit " + logMethodName);

            // cleaning objects
            this.table = null;
            tableClient = null;
        }

        /// <summary>Return Date and sequence number</summary>
        /// <param name="tableName">Table Name information</param>
        /// <param name="keyValue">Entity information</param>
        /// <returns>Sequence Number and Date</returns>
        public object FetchEntity(string tableName, string keyValue)
        {
            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            object resultValue = null;
            CloudTableClient tableClient = this.storageAccount.CreateCloudTableClient();
            this.table = tableClient.GetTableReference(tableName);
            this.table.CreateIfNotExists();

            switch (tableName)
            {
                case Constants.SettingsTable:
                    {
                        TableQuery<Settings> query = (new TableQuery<Settings>()).Where(TableQuery.GenerateFilterCondition("RowKey", QueryComparisons.Equal, keyValue));
                        IEnumerable<Settings> result = this.table.ExecuteQuery<Settings>(query);
                        List<Settings> resultList = result.ToList();
                        resultValue = resultList[0].ConfigValue;
                        break;
                    }

                case Constants.DeviceRegistrationTable:
                    {
                        TableQuery<DeviceRegistrationMap> query = (new TableQuery<DeviceRegistrationMap>()).Where(TableQuery.GenerateFilterCondition("DeviceType", QueryComparisons.Equal, keyValue));
                        IEnumerable<DeviceRegistrationMap> result = this.table.ExecuteQuery<DeviceRegistrationMap>(query);
                        List<DeviceRegistrationMap> resultList = result.ToList();
                        resultValue = resultList[0].RegistrationType;
                        break;
                    }

                case Constants.RGN1RuleSetInformationTable:
                    {
                        TableQuery<RuleSetInfoEntity> query = (new TableQuery<RuleSetInfoEntity>()).Where(TableQuery.GenerateFilterCondition("Authority", QueryComparisons.Equal, keyValue));
                        IEnumerable<RuleSetInfoEntity> result = this.table.ExecuteQuery<RuleSetInfoEntity>(query);
                        List<RuleSetInfoEntity> resultList = result.ToList();
                        resultValue = resultList;
                        break;
                    }
            }

            return resultValue;
        }

        /// <summary>Retrieves entity from table</summary>
        /// <param name="tableName">Azure Table information</param>
        /// <param name="partitionKey">Partition Key that is to be fetched.</param>
        /// <param name="rowKey">Row Key that is to be fetched.</param>
        /// <returns>Returns response after Retrieving entity</returns>
        public object RetrieveEntity(string tableName, string partitionKey, string rowKey)
        {
            string logMethodName = "AzureStorageHelper.Execute(retrieveOperation)";
            object resp = null;

            // Begin Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);

            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            CloudTableClient tableClient = this.storageAccount.CreateCloudTableClient();
            this.table = tableClient.GetTableReference(tableName);
            this.table.CreateIfNotExists();
            switch (tableName)
            {
                case Constants.RGN1FixedTVBDRegistrationTable:
                    {
                        TableQuery<FixedTVBDRegistration> query = (new TableQuery<FixedTVBDRegistration>()).Where(TableQuery.GenerateFilterCondition("SerialNumber", QueryComparisons.Equal, partitionKey));
                        IEnumerable<FixedTVBDRegistration> result = this.table.ExecuteQuery<FixedTVBDRegistration>(query);
                        TableQuery<FixedTVBDRegistration> queryRowKey = (new TableQuery<FixedTVBDRegistration>()).Where(TableQuery.GenerateFilterCondition("FCCId", QueryComparisons.Equal, rowKey));
                        IEnumerable<FixedTVBDRegistration> resultRowKey = this.table.ExecuteQuery<FixedTVBDRegistration>(queryRowKey);
                        if (result.Count() != 0 && resultRowKey.Count() != 0)
                        {
                            resp = 1;
                        }
                        else
                        {
                            resp = 0;
                        }

                        break;
                    }

                case Constants.RGN1RegisteredDeviceValidationTable:
                    {
                        TableQuery<RegisteredDevice> queryPartitionKey = (new TableQuery<RegisteredDevice>()).Where(TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, partitionKey));
                        IEnumerable<RegisteredDevice> resultPartitionKey = this.table.ExecuteQuery<RegisteredDevice>(queryPartitionKey);
                        TableQuery<RegisteredDevice> queryRowKey = (new TableQuery<RegisteredDevice>()).Where(TableQuery.GenerateFilterCondition("RowKey", QueryComparisons.Equal, rowKey));
                        IEnumerable<RegisteredDevice> resultRowKey = this.table.ExecuteQuery<RegisteredDevice>(queryRowKey);
                        if (resultPartitionKey.Count() != 0 && resultRowKey.Count() != 0)
                        {
                            resp = 1;
                        }
                        else
                        {
                            resp = 0;
                        }

                        break;
                    }
            }

            return resp;
        }

        /// <summary>Retrieves entity from table</summary>
        /// <param name="tableName">Azure Table information</param>
        /// <param name="value">value that is to be fetched.</param>
        /// <returns>Returns response after Retrieving entity</returns>
        public List<TableEntity> RetrieveEntity(string tableName, string value)
        {
            string logMethodName = "AzureStorageHelper.Execute(retrieveOperation)";
            List<TableEntity> resp = null;

            // Begin Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);

            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            CloudTableClient tableClient = this.storageAccount.CreateCloudTableClient();
            this.table = tableClient.GetTableReference(tableName);
            this.table.CreateIfNotExists();

            switch (tableName)
            {
                case Constants.RGN1MVPDRegistrationTableName:
                    {
                        TableQuery<MVPDRegistration> queryRowKey = (new TableQuery<MVPDRegistration>()).Where(TableQuery.GenerateFilterCondition("RowKey", QueryComparisons.Equal, value));
                        List<MVPDRegistration> result = this.table.ExecuteQuery<MVPDRegistration>(queryRowKey).ToList<MVPDRegistration>();
                        resp = result.ToList<TableEntity>();
                        break;
                    }

                case Constants.RGN1LPAuxRegistrationTable:
                    {
                        TableQuery<LPAuxRegistration> queryRowKey = (new TableQuery<LPAuxRegistration>()).Where(TableQuery.GenerateFilterCondition("RowKey", QueryComparisons.Equal, value));
                        List<LPAuxRegistration> result = this.table.ExecuteQuery<LPAuxRegistration>(queryRowKey).ToList<LPAuxRegistration>();
                        resp = result.ToList<TableEntity>();
                        break;
                    }

                case Constants.RGN1TempBasRegistrationTable:
                    {
                        TableQuery<TempBASRegistration> queryRowKey = (new TableQuery<TempBASRegistration>()).Where(TableQuery.GenerateFilterCondition("RowKey", QueryComparisons.Equal, value));
                        List<TempBASRegistration> result = this.table.ExecuteQuery<TempBASRegistration>(queryRowKey).ToList<TempBASRegistration>();
                        resp = result.ToList<TableEntity>();
                        break;
                    }

                case Constants.RGN1LPAuxRegistrationDetailsTable:
                    {
                        TableQuery<LPAuxRegistration> queryRowKey = (new TableQuery<LPAuxRegistration>()).Where(TableQuery.GenerateFilterCondition("RegId", QueryComparisons.Equal, value));
                        List<LPAuxRegistration> result = this.table.ExecuteQuery<LPAuxRegistration>(queryRowKey).ToList<LPAuxRegistration>();
                        resp = result.ToList<TableEntity>();
                        break;
                    }
            }

            return resp;
        }

        /// <summary>
        /// Fetches the entity.
        /// </summary>
        /// <typeparam name="T">The type to fetch</typeparam>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="filters">The filters.</param>
        /// <returns>returns System.Object.</returns>
        public List<T> FetchEntity<T>(string tableName, object filters) where T : ITableEntity, new()
        {
            var filterQuery = this.ConvertFiltersToTableQuery(filters);
            TableQuery<T> query = null;

            if (string.IsNullOrEmpty(filterQuery))
            {
                query = new TableQuery<T>();
            }
            else
            {
                query = new TableQuery<T>().Where(filterQuery);
            }

            return this.FetchEntity<T>(tableName, query);
        }

        /// <summary>Fetches the entity.</summary>
        /// <typeparam name="T">The type to fetch</typeparam>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="partitionKey">The partition key.</param>
        /// <param name="rowKey">The row key.</param>
        /// <returns>returns System.Object.</returns>
        public List<T> FetchEntity<T>(string tableName, string partitionKey = null, string rowKey = null) where T : ITableEntity, new()
        {
            Dictionary<string, string> filtersDict = new Dictionary<string, string>();
            if (!string.IsNullOrEmpty(partitionKey))
            {
                filtersDict.Add("PartitionKey", partitionKey);
            }

            if (!string.IsNullOrEmpty(rowKey))
            {
                filtersDict.Add("RowKey", rowKey);
            }

            return this.FetchEntity<T>(tableName, filtersDict);
        }

        /// <summary>Fetches the entity.</summary>
        /// <typeparam name="T">type of entity</typeparam>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="filters">The filters.</param>
        /// <returns>returns List of Entities.</returns>
        public List<T> FetchEntity<T>(string tableName, Dictionary<string, string> filters) where T : ITableEntity, new()
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

            return this.FetchEntity<T>(tableName, query);
        }

        /// <summary>Fetches the entity.</summary>
        /// <typeparam name="T">type of entity</typeparam>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="finalQuery">The final query.</param>
        /// <returns>returns List of Entities.</returns>
        public List<T> FetchEntity<T>(string tableName, TableQuery<T> finalQuery) where T : ITableEntity, new()
        {
            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            CloudTableClient tableClient = this.storageAccount.CreateCloudTableClient();
            this.table = tableClient.GetTableReference(tableName);
            this.table.CreateIfNotExists();

            List<T> result = this.table.ExecuteQuery(finalQuery).ToList();

            return result;
        }

        /// <summary>Fetches the entity.</summary>
        /// <typeparam name="T">type of entity</typeparam>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="finalQuery">The final query.</param>
        /// <returns>returns List of Entities.</returns>
        public List<T> FetchEntityFromTerrainData<T>(string tableName, TableQuery<T> finalQuery) where T : ITableEntity, new()
        {
            if (this.storageAccountTerrain == null)
            {
                this.storageAccountTerrain = this.GetCloudBlobStorageAccount();
            }

            CloudTableClient tableClient = this.storageAccountTerrain.CreateCloudTableClient();
            this.table = tableClient.GetTableReference(tableName);
            this.table.CreateIfNotExists();

            List<T> result = this.table.ExecuteQuery(finalQuery).ToList();

            return result;
        }

        /// <summary>Retrieves DBAs' Information</summary>
        /// <param name="tableName"> Entity Table Name</param>
        /// <returns>DBAdminInfo Array</returns>
        public DBAdminInfo[] GetWSDBAPollInfo(string tableName)
        {
            string logMethodName = "AzureStorageHelper.Execute(retrieveOperation)";

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DALCGenericMessage, "Enter " + logMethodName);

            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            CloudTableClient tableClient = this.storageAccount.CreateCloudTableClient();
            this.table = tableClient.GetTableReference(tableName);
            this.table.CreateIfNotExists();
            TableQuery<DBAdminInfo> query = (new TableQuery<DBAdminInfo>()).Where(TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.NotEqual, "MSFT"));
            IEnumerable<DBAdminInfo> result = this.table.ExecuteQuery<DBAdminInfo>(query);

            return result.ToArray();
        }

        /// <summary>
        /// Inserts the update batch.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="batchData">The batch data.</param>
        public void InsertUpdateBatch(string tableName, List<TableOperation> batchData)
        {
            var cloudAccount = this.GetCloudStorageAccount();
            var cloudTable = cloudAccount.CreateCloudTableClient().GetTableReference(tableName);
            cloudTable.CreateIfNotExists();

            TableBatchOperation batchOperation = new TableBatchOperation();
            for (int i = 0; i < batchData.Count; i += 100)
            {
                batchOperation.Clear();

                for (int j = i; j < i + 100; j++)
                {
                    if (j >= batchData.Count)
                    {
                        break;
                    }

                    batchOperation.Add(batchData[j]);
                }

                cloudTable.ExecuteBatch(batchOperation);
            }
        }

        /// <summary>
        /// Inserts the update batch.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="batchData">The batch data.</param>
        public void InsertUpdateBatch(string tableName, TableBatchOperation batchData)
        {
            var cloudAccount = this.GetCloudStorageAccount();
            var cloudTable = cloudAccount.CreateCloudTableClient().GetTableReference(tableName);
            cloudTable.CreateIfNotExists();

            cloudTable.ExecuteBatch(batchData);
        }

        /// <summary>
        /// Inserts the update batch terrain data.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="batchData">The batch data.</param>
        public void InsertUpdateBatchTerrainData(string tableName, List<TableOperation> batchData)
        {
            var cloudAccount = this.GetCloudBlobStorageAccount();
            var cloudTable = cloudAccount.CreateCloudTableClient().GetTableReference(tableName);
            cloudTable.CreateIfNotExists();

            TableBatchOperation batchOperation = new TableBatchOperation();
            for (int i = 0; i < batchData.Count; i += 100)
            {
                batchOperation.Clear();

                for (int j = i; j < i + 100; j++)
                {
                    if (j >= batchData.Count)
                    {
                        break;
                    }

                    batchOperation.Add(batchData[j]);
                }

                cloudTable.ExecuteBatch(batchOperation);
            }
        }

        /// <summary>
        /// Inserts the update batch terrain data.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="batchData">The batch data.</param>
        public void InsertUpdateBatchPMSEUnscheduledAdjustment(string tableName, List<TableOperation> batchData)
        {
            var cloudAccount = this.GetCloudStorageAccount();
            var cloudTable = cloudAccount.CreateCloudTableClient().GetTableReference(tableName);
            cloudTable.CreateIfNotExists();

            TableBatchOperation batchOperation = new TableBatchOperation();
            for (int i = 0; i < batchData.Count; i += 100)
            {
                batchOperation.Clear();

                for (int j = i; j < i + 100; j++)
                {
                    if (j >= batchData.Count)
                    {
                        break;
                    }

                    batchOperation.Add(batchData[j]);
                }

                cloudTable.ExecuteBatch(batchOperation);
            }
        }

        /// <summary>
        /// Inserts the update batch terrain data.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="batchData">The batch data.</param>
        public void InsertUpdateBatchTerrainData(string tableName, TableBatchOperation batchData)
        {
            var cloudAccount = this.GetCloudBlobStorageAccount();
            var cloudTable = cloudAccount.CreateCloudTableClient().GetTableReference(tableName);
            cloudTable.CreateIfNotExists();
            cloudTable.ExecuteBatch(batchData);
        }

        /// <summary>
        /// Inserts/updates dynamic entity
        /// </summary>
        /// <param name="tableName">azure table name</param>
        /// <param name="entity">table entity</param>
        public void InsertUpdateDynamicEntity(string tableName, DynamicTableEntity entity)
        {
            var cloudAccount = this.GetCloudStorageAccount();
            var cloudTable = cloudAccount.CreateCloudTableClient().GetTableReference(tableName);
            cloudTable.CreateIfNotExists();

            cloudTable.Execute(TableOperation.InsertOrReplace(entity));
        }

        /// <summary>
        /// Updates the entity.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="entity">The entity.</param>
        public void UpdateEntity(string tableName, TableEntity entity)
        {
            var cloudAccount = this.GetCloudStorageAccount();
            var cloudTable = cloudAccount.CreateCloudTableClient().GetTableReference(tableName);
            cloudTable.CreateIfNotExists();

            cloudTable.Execute(TableOperation.Merge(entity));
        }

        /// <summary>
        /// Gets the cloud BLOB container.
        /// </summary>
        /// <param name="dataContainer">The data container.</param>
        /// <returns>returns CloudBlobContainer.</returns>
        public CloudBlobContainer GetCloudBlobContainer(string dataContainer)
        {
            if (this.storageAccountTerrain == null)
            {
                this.storageAccountTerrain = this.GetCloudBlobStorageAccount();
            }

            CloudBlobClient blobClient = this.storageAccountTerrain.CreateCloudBlobClient();
            CloudBlobContainer container = blobClient.GetContainerReference(dataContainer);
            return container;
        }

        /// <summary>
        /// Gets the cloud BLOB container.
        /// </summary>
        /// <param name="dataContainer">The data container.</param>
        /// <returns>returns CloudBlobContainer.</returns>
        public CloudBlobContainer GetDttSyncCloudBlobContainer(string dataContainer)
        {
            CloudBlobContainer container = this.GetCloudBlobContainer(dataContainer);
            return container;
        }

        /// <summary>
        /// Inserts/updates dynamic entity
        /// </summary>
        /// <param name="tableName">azure table name</param>
        /// <param name="entity">table entity</param>
        public void InsertUser(string tableName, User entity)
        {
            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            var cloudTable = this.storageAccount.CreateCloudTableClient().GetTableReference(tableName);
            cloudTable.CreateIfNotExists();

            cloudTable.Execute(TableOperation.InsertOrReplace(entity));
        }

        /// <summary>
        /// Inserts/updates dynamic entity
        /// </summary>
        /// <param name="tableName">azure table name</param>
        /// <param name="entity">table entity</param>
        public void UpdateUser(string tableName, User entity)
        {
            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            var cloudTable = this.storageAccount.CreateCloudTableClient().GetTableReference(tableName);
            cloudTable.CreateIfNotExists();

            cloudTable.Execute(TableOperation.Merge(entity));
        }

        /// <summary>
        /// Inserts/updates dynamic entity
        /// </summary>
        /// <param name="tableName">azure table name</param>
        /// <param name="entity">table entity</param>
        public void InsertAccessRegion(string tableName, RegionAccess entity)
        {
            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            var cloudTable = this.storageAccount.CreateCloudTableClient().GetTableReference(tableName);
            cloudTable.Execute(TableOperation.InsertOrMerge(entity));
        }

        /// <summary>
        /// Elevates the access request.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="access">The access.</param>
        /// <returns>returns String </returns>
        public string ElevateAccessRequest(string tableName, ElevatedAccessRequest access)
        {
            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            var cloudTable = this.storageAccount.CreateCloudTableClient().GetTableReference(tableName);
            cloudTable.Execute(TableOperation.InsertOrMerge(access));
            return access.PartitionKey;
        }

        /// <summary>
        /// Updates the access region.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="entity">The entity.</param>
        public void UpdateAccessRegion(string tableName, RegionAccess entity)
        {
            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            var cloudTable = this.storageAccount.CreateCloudTableClient().GetTableReference(tableName);
            cloudTable.Execute(TableOperation.InsertOrMerge(entity));
        }

        /// <summary>
        /// Gets the user access levels.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="userId">The user identifier.</param>
        /// <returns> Region Access </returns>
        public List<RegionAccess> GetUserAccessLevels(string tableName, string userId)
        {
            List<RegionAccess> regionAccess = new List<RegionAccess>();

            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            var cloudTable = this.storageAccount.CreateCloudTableClient().GetTableReference(tableName);

            string filtercondition = null;
            string filterPartitionKey = null;

            filtercondition = TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, userId);
            filterPartitionKey = TableQuery.GenerateFilterConditionForBool("Deleted", QueryComparisons.Equal, false);
            TableQuery<RegionAccess> query = new TableQuery<RegionAccess>().Where(TableQuery.CombineFilters(filtercondition, TableOperators.And, filterPartitionKey));

            foreach (RegionAccess entity in cloudTable.ExecuteQuery(query))
            {
                regionAccess.Add(entity);
            }

            return regionAccess;
        }

        /// <summary>
        /// Merges the entity.
        /// </summary>
        /// <param name="regionalTableName">Name of the regional table.</param>
        /// <param name="tableOperation">The table operation.</param>
        public void MergeEntity(string regionalTableName, TableOperation tableOperation)
        {
            var cloudAccount = this.GetCloudStorageAccount();
            var cloudTable = cloudAccount.CreateCloudTableClient().GetTableReference(regionalTableName);
            cloudTable.CreateIfNotExists();

            cloudTable.Execute(tableOperation);
        }

        /// <summary>
        /// Converts the filters to table query.
        /// </summary>
        /// <param name="filters">The filters.</param>
        /// <returns>returns filter string.</returns>
        public string ConvertFiltersToTableQuery(object filters)
        {
            var finalQuery = string.Empty;
            if (filters == null)
            {
                return finalQuery;
            }

            PropertyDescriptorCollection props = TypeDescriptor.GetProperties(filters);
            foreach (PropertyDescriptor prop in props)
            {
                var filterQuery = string.Empty;

                if (prop.PropertyType == typeof(bool))
                {
                    filterQuery = TableQuery.GenerateFilterConditionForBool(prop.Name, QueryComparisons.Equal, (bool)prop.GetValue(filters));
                }
                else if (prop.PropertyType == typeof(DateTime))
                {
                    filterQuery = TableQuery.GenerateFilterConditionForDate(prop.Name, QueryComparisons.Equal, (DateTime)prop.GetValue(filters));
                }
                else if (prop.PropertyType == typeof(double))
                {
                    filterQuery = TableQuery.GenerateFilterConditionForDouble(prop.Name, QueryComparisons.Equal, (double)prop.GetValue(filters));
                }
                else if (prop.PropertyType == typeof(int))
                {
                    filterQuery = TableQuery.GenerateFilterConditionForInt(prop.Name, QueryComparisons.Equal, (int)prop.GetValue(filters));
                }
                else if (prop.PropertyType == typeof(long))
                {
                    filterQuery = TableQuery.GenerateFilterConditionForLong(prop.Name, QueryComparisons.Equal, (long)prop.GetValue(filters));
                }
                else if (prop.PropertyType == typeof(Guid))
                {
                    filterQuery = TableQuery.GenerateFilterConditionForGuid(prop.Name, QueryComparisons.Equal, (Guid)prop.GetValue(filters));
                }
                else
                {
                    filterQuery = TableQuery.GenerateFilterCondition(prop.Name, QueryComparisons.Equal, (string)prop.GetValue(filters));
                }

                if (string.IsNullOrEmpty(finalQuery))
                {
                    finalQuery = filterQuery;
                }
                else
                {
                    finalQuery = TableQuery.CombineFilters(finalQuery, TableOperators.And, filterQuery);
                }
            }

            return finalQuery;
        }

        /// <summary>
        /// Uploads the BLOB to storage.
        /// </summary>
        /// <param name="fileName">Name of the file.</param>
        /// <param name="container">The container.</param>
        /// <param name="blobName">Name of the BLOB.</param>
        /// <returns>boolean value</returns>
        public bool UploadBlobToStorage(string fileName, string container, string blobName)
        {
            // Create the blob client.
            this.storageAccountTerrain = this.GetCloudBlobStorageAccount();
            CloudBlobClient blobClient = this.storageAccountTerrain.CreateCloudBlobClient();

            // Retrieve a reference to a container. 
            CloudBlobContainer blobContainer = blobClient.GetContainerReference(container);

            // Retrieve reference to a blob.
            CloudBlockBlob blockBlob = blobContainer.GetBlockBlobReference(blobName);

            // Create or overwrite the "myblob" blob with contents from a local file.
            using (var fileStream = System.IO.File.OpenRead(fileName))
            {
                blockBlob.UploadFromStream(fileStream);
            }

            return true;
        }

        /// <summary>
        /// Moves all blobs from source to destination container
        /// </summary>
        /// <param name="sourceContainer">Source Container</param>
        /// <param name="currentBlobName"> Current Blob name to be retained</param>
        /// <param name="targetContainer">Target Container</param>
        /// <returns>boolean value</returns>
        public bool MoveBlobs(string sourceContainer, string currentBlobName, string targetContainer)
        {
            // Create the blob client.
            this.storageAccountTerrain = this.GetCloudBlobStorageAccount();
            CloudBlobClient blobClientSource = this.storageAccountTerrain.CreateCloudBlobClient();
            CloudBlobClient blobClientTarget = this.storageAccountTerrain.CreateCloudBlobClient();

            // Retrieve a reference to a container. 
            CloudBlobContainer blobContainerFrom = blobClientSource.GetContainerReference(sourceContainer);
            CloudBlobContainer blobContainerTo = blobClientTarget.GetContainerReference(targetContainer);

            IEnumerable<IListBlobItem> listBlob = blobContainerFrom.ListBlobs(null);

            foreach (var blobItem in listBlob)
            {
                CloudBlockBlob blockBlobSource = (CloudBlockBlob)blobItem;
                CloudBlockBlob blockBlob = blobContainerTo.GetBlockBlobReference(blockBlobSource.Name);
                if (currentBlobName != blockBlobSource.Name)
                {
                    blockBlob.BeginStartCopyFromBlob(blockBlobSource, null, null);
                    blockBlobSource.Delete();
                }
            }

            return true;
        }

        /// <summary>
        /// Moves the BLOB.
        /// </summary>
        /// <param name="sourceContainer">The source container.</param>
        /// <param name="blobName">Name of the BLOB.</param>
        /// <param name="targetContainer">The target container.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool MoveBlob(string sourceContainer, string blobName, string targetContainer)
        {
            // Create the blob client.
            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            // this.storageAccountTerrain = this.GetCloudBlobStorageAccount();
            CloudBlobClient blobClientSource = this.storageAccount.CreateCloudBlobClient();
            CloudBlobClient blobClientTarget = this.storageAccount.CreateCloudBlobClient();

            // Retrieve a reference to a container. 
            CloudBlobContainer blobContainerFrom = blobClientSource.GetContainerReference(sourceContainer);
            CloudBlobContainer blobContainerTo = blobClientTarget.GetContainerReference(targetContainer);
            CloudBlockBlob blockBlobSource = blobContainerFrom.GetBlockBlobReference(blobName);
            CloudBlockBlob blockBlob = blobContainerTo.GetBlockBlobReference(blobName);
            blockBlob.StartCopyFromBlob(blockBlobSource);
            blockBlobSource.Delete();

            return true;
        }

        /// <summary>Return Cloud Blob Storage Account</summary>
        /// <returns>Cloud Storage Account</returns>
        public CloudStorageAccount GetCloudBlobStorageAccount()
        {
            //// creates cloud storage account by reading connection string from app settings
            return CloudStorageAccount.Parse(Utils.TerrainDbConnectionString);
        }

        /// <summary>Return Cloud Storage Account</summary>
        /// <returns>Cloud Storage Account</returns>
        public CloudStorageAccount GetCloudStorageAccount()
        {
            //// creates cloud storage account by reading connection string from app settings
            return CloudStorageAccount.Parse(Utils.Configuration.DbConnectionString);
        }

        /// <summary>
        /// Gets the cloud table.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <returns>returns CloudTable.</returns>
        public CloudTable GetCloudTable(string tableName)
        {
            var cloudTable = this.GetCloudStorageAccount().CreateCloudTableClient().GetTableReference(tableName);
            cloudTable.CreateIfNotExists();
            return cloudTable;
        }

        /// <summary>
        /// Downloads blob content as string.
        /// </summary>
        /// <param name="blobName">Blob name.</param>
        /// <param name="containerName">Blob container name.</param>        
        /// <returns>Blob content as string.</returns>
        /// <exception cref="Microsoft.WindowsAzure.Storage.StorageException">
        /// CloudBlockBlob.DownloadText method call fails with an exception.
        /// </exception>
        public string Download(string blobName, string containerName)
        {
            CloudBlobClient blobClient = this.storageAccount.CreateCloudBlobClient();

            CloudBlobContainer container = blobClient.GetContainerReference(containerName);
            CloudBlockBlob blockBlob = container.GetBlockBlobReference(blobName);

            try
            {
                return blockBlob.DownloadText();
            }
            catch (StorageException ex)
            {
                string errorMessage = string.Format("Blob download operation failed. Container Name: {0},Blob Name: {1} {2} {3}", containerName, blobName, Environment.NewLine, ex.ToString());

                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DALCGenericMessage, errorMessage);

                throw;
            }
        }


        public List<T> QueryEntities<T>(string tableName, Func<T,bool> query) where T : ITableEntity, new()
        {
            if (this.storageAccount == null)
            {
                this.storageAccount = this.GetCloudStorageAccount();
            }

            var table = this.storageAccount.CreateCloudTableClient().GetTableReference(tableName);

            table.CreateIfNotExists();

            return table.CreateQuery<T>().Where(query).ToList();          
        }
    }
}
