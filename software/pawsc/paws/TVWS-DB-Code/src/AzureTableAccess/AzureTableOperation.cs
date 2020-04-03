// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureTableAccess
{
    using System.Collections.Generic;
    using System.Configuration;
    using Microsoft.WindowsAzure.ServiceRuntime;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// do data access operations on a azure table
    /// </summary>
    public class AzureTableOperation : IAzureTableOperation
    {
        /// <summary>
        /// variable to hold <see cref="CloudStorageAccount"/> value
        /// </summary>
        private readonly CloudStorageAccount storageAccount;

        /// <summary>
        /// variable to hold <see cref="CloudTableClient"/> value
        /// </summary>
        private readonly CloudTableClient tableClient;

        /// <summary>
        /// variable to hold <see cref="CloudTable"/> value
        /// </summary>
        private CloudTable table;

        /// <summary>
        /// variable to hold table name
        /// </summary>
        private string tableName;

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureTableOperation"/> class
        /// </summary>
        public AzureTableOperation()
        {
            this.storageAccount = CloudStorageAccount.Parse(this.StorageConnectionString);
            this.tableClient = this.storageAccount.CreateCloudTableClient();
        }

        /// <summary>
        /// Gets storage connection string value from configuration
        /// </summary>
        public string StorageConnectionString
        {
            get
            {
                return RoleEnvironment.IsAvailable
                            ? RoleEnvironment.GetConfigurationSettingValue("AzureStorageAccountConnectionString")
                            : ConfigurationManager.ConnectionStrings["AzureStorageAccountConnectionString"].ConnectionString;
            }
        }

        /// <summary>
        /// Insert entity in to azure table
        /// </summary>
        /// <param name="entity">table entity</param>
        public void InsertEntity(TableEntity entity)
        {
            this.tableName = entity.GetType().Name;
            this.table = this.tableClient.GetTableReference(this.tableName);
            this.table.CreateIfNotExists();

            TableOperation insertOperation = TableOperation.InsertOrReplace(entity);
            this.table.Execute(insertOperation);
        }

        /// <summary>
        /// Fetch entity from table
        /// </summary>
        /// <typeparam name="TElement">generic table element</typeparam>
        /// <param name="partionKey">partition key in a table</param>
        /// <param name="rowKey">row key in a table</param>
        /// <param name="tableName">name of the table</param>
        /// <returns>entity of a table</returns>
        public TElement FetchEntity<TElement>(string partionKey, string rowKey, string tableName = "") where TElement : ITableEntity
        {
            TableOperation retrieveOperation = null;

            if (string.IsNullOrEmpty(tableName))
            {
                this.tableName = typeof(TElement).Name;
                retrieveOperation = TableOperation.Retrieve<TElement>(partionKey.ToLower(), rowKey.ToLower());
            }
            else
            {
                this.tableName = tableName;
                retrieveOperation = TableOperation.Retrieve<TElement>(partionKey, rowKey);
            }

            this.table = this.tableClient.GetTableReference(this.tableName);

            return (TElement)this.table.Execute(retrieveOperation).Result;
        }

        /// <summary>
        /// Get entities having supplied partition key
        /// </summary>
        /// <typeparam name="TElement">generic table element</typeparam>
        /// <param name="partitionKey">partition key</param>
        /// <returns>list of entities</returns>
        public IEnumerable<TElement> GetEntityByPartitionKey<TElement>(string partitionKey) where TElement : ITableEntity, new()
        {
            this.tableName = typeof(TElement).Name;
            this.table = this.tableClient.GetTableReference(this.tableName);
            TableQuery<TElement> rangeQuery = new TableQuery<TElement>().Where(TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, partitionKey.ToLower()));

            return this.table.ExecuteQuery(rangeQuery);
        }

        /// <summary>
        /// Gets all entities in a table
        /// </summary>
        /// <typeparam name="TElement">generic table element</typeparam>
        /// <param name="tableName">name of the table</param>
        /// <returns>list of entities</returns>
        public IEnumerable<TElement> GetAllEntities<TElement>(string tableName = "") where TElement : ITableEntity, new()
        {
            if (string.IsNullOrEmpty(tableName))
            {
                this.tableName = typeof(TElement).Name;
            }
            else
            {
                this.tableName = tableName;
            }

            this.table = this.tableClient.GetTableReference(this.tableName);

            // To avoid exceptions, if table doesn't exists
            this.table.CreateIfNotExists();

            TableQuery<TElement> rangeQuery = new TableQuery<TElement>();

            return this.table.ExecuteQuery(rangeQuery);
        }

        /// <summary>
        /// Gets all entities in a table
        /// </summary>
        /// <typeparam name="TElement">generic table element</typeparam>
        /// <param name="tableName">name of the table</param>
        /// <param name="filter">filter for the query</param>
        /// <returns>list of entities</returns>
        public IEnumerable<TElement> GetAllEntities<TElement>(string tableName, string filter) where TElement : ITableEntity, new()
        {
            this.tableName = tableName;
            this.table = this.tableClient.GetTableReference(this.tableName);

            TableQuery<TElement> rangeQuery = new TableQuery<TElement>();

            if (!string.IsNullOrWhiteSpace(filter))
            {
                rangeQuery.Where(filter);
            }

            // TODO: We may probably have to pass following column names as a part of TableQuery.
            // The fact that, if we are query a table which has none of the fields below that might break the code and this method becomes
            // to specific only table having following fields.
            List<string> columns = new List<string>();
            columns.Add("CallSign");
            columns.Add("Channel");
            columns.Add("Contour");
            columns.Add("Latitude");
            columns.Add("Longitude");
            rangeQuery.Select(columns);
            IEnumerable<TElement> elements = this.table.ExecuteQuery(rangeQuery);
            return elements;
        }

        public IEnumerable<DynamicTableEntity> GetTableEntityProjection(string tableName, TableQuery<DynamicTableEntity> projectionQuery, TableRequestOptions requestOptions = null, OperationContext operationContext = null)
        {
            this.tableName = tableName;
            this.table = this.tableClient.GetTableReference(this.tableName);

            return this.table.ExecuteQuery(projectionQuery, requestOptions, operationContext);
        }

        public void DeleteEntity(string tableName, TableEntity entity)
        {
            this.table = this.tableClient.GetTableReference(tableName);

            TableOperation deleteOperation = TableOperation.Delete(entity);
            this.table.CreateIfNotExists();

            // Executes insert operation
            this.table.Execute(deleteOperation);
        }

        public void InsertBatchEntities<TElement>(string tableName, IEnumerable<TElement> entities) where TElement : ITableEntity
        {
            List<TableOperation> tableOperations = new List<TableOperation>();

            foreach (var entity in entities)
            {
                tableOperations.Add(TableOperation.InsertOrReplace(entity));
            }

            this.ProcessBatchOperations(tableName, tableOperations);
        }

        public void DeleteBatchEntities<TElement>(string tableName, IEnumerable<TElement> entities) where TElement : ITableEntity
        {
            List<TableOperation> tableOperations = new List<TableOperation>();

            foreach (var entity in entities)
            {
                tableOperations.Add(TableOperation.Delete(entity));
            }

            this.ProcessBatchOperations(tableName, tableOperations);
        }

        private void ProcessBatchOperations(string tableName, List<TableOperation> tableOperations)
        {
            this.table = this.tableClient.GetTableReference(tableName);

            TableBatchOperation batchOperation = new TableBatchOperation();
            for (int i = 0; i < tableOperations.Count; i += 100)
            {
                batchOperation.Clear();

                for (int j = i; j < i + 100; j++)
                {
                    if (j >= tableOperations.Count)
                    {
                        break;
                    }

                    batchOperation.Add(tableOperations[j]);
                }

                this.table.ExecuteBatch(batchOperation);
            }
        }       
    }
}
