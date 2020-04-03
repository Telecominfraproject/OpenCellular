// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureTableAccess
{
    using System.Collections.Generic;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// interface which defines azure table operations
    /// </summary>
    public interface IAzureTableOperation
    {
        /// <summary>
        /// Insert entity in to azure table
        /// </summary>
        /// <param name="entity">table entity</param>
        void InsertEntity(TableEntity entity);

        /// <summary>
        /// Fetch entity from table
        /// </summary>
        /// <typeparam name="TElement">generic table element</typeparam>
        /// <param name="partionKey">partition key in a table</param>
        /// <param name="rowKey">row key in a table</param>
        /// <param name="tableName">table name</param>
        /// <returns>entity of a table</returns>
        TElement FetchEntity<TElement>(string partionKey, string rowKey, string tableName = "") where TElement : ITableEntity;

        /// <summary>
        /// Get entities having supplied partition key
        /// </summary>
        /// <typeparam name="TElement">generic table element</typeparam>
        /// <param name="partitionKey">partition key</param>
        /// <returns>list of entities</returns>
        IEnumerable<TElement> GetEntityByPartitionKey<TElement>(string partitionKey) where TElement : ITableEntity, new();

        /// <summary>
        /// Gets all entities in a table
        /// </summary>
        /// <typeparam name="TElement">generic table element</typeparam>
        /// <param name="tableName">table name</param>
        /// <returns>list of entities</returns>
        IEnumerable<TElement> GetAllEntities<TElement>(string tableName = "") where TElement : ITableEntity, new();

        /// <summary>
        /// Gets sub set of entity properties in a table
        /// </summary>
        /// <param name="tableName">table name</param>
        /// <param name="projectionQuery">projection query</param>
        /// <param name="requestOptions"> table service request options</param>
        /// <param name="operationContext">table service operation context</param>
        /// <returns>list of projected entities</returns>
        IEnumerable<DynamicTableEntity> GetTableEntityProjection(string tableName, TableQuery<DynamicTableEntity> projectionQuery, TableRequestOptions requestOptions = null, OperationContext operationContext = null);

        void DeleteEntity(string tableName, TableEntity entity);

        void InsertBatchEntities<TElement>(string tableName, IEnumerable<TElement> entities) where TElement : ITableEntity;

        void DeleteBatchEntities<TElement>(string tableName, IEnumerable<TElement> entities) where TElement : ITableEntity;       
    }
}
