// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{    
    using System;
    using System.Collections.Generic;
    using System.IO;    
    using Entities;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents the Configuration interface into the data access layer component.
    /// </summary>
    public interface IDalcCommon
    {
        /// <summary>
        /// Fetches the Date or Sequence.
        /// </summary>
        /// <param name="settingsTable">Table type to be fetched.</param>
        /// <param name="keyValue">Value that is to be fetched.</param>
        /// <returns>Entity Name.</returns>
        object FetchEntity(string settingsTable, string keyValue);

        /// <summary>
        /// Updates the table.
        /// </summary>
        /// <typeparam name="T">type of entity</typeparam>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="tableRows">The table rows.</param>
        /// <returns>returns success code.</returns>
        int UpdateTable<T>(string tableName, List<T> tableRows) where T : ITableEntity, new();

        /// <summary>
        /// Deletes the records.
        /// </summary>
        /// <typeparam name="T">type of entity</typeparam>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="tableRows">The table rows.</param>
        void DeleteRecords<T>(string tableName, List<T> tableRows) where T : ITableEntity, new();

        /// <summary>
        /// Fetches the entity.
        /// </summary>
        /// <typeparam name="T">Table type to be fetched.</typeparam>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns object.</returns>
        List<T> FetchEntity<T>(string tableName, object parameters) where T : ITableEntity, new();

        /// <summary>
        /// Fetches the entity with desired columns.
        /// </summary>
        /// <typeparam name="T">entity type to be fetched</typeparam>
        /// <param name="tableName">table name</param>
        /// <param name="filters">condition filters</param>
        /// <param name="columns">columns of an entity</param>
        /// <returns>list of entities</returns>
        List<T> FetchEntity<T>(string tableName, Dictionary<string, string> filters, List<string> columns) where T : ITableEntity, new();

        /// <summary>
        /// Return Settings Date
        /// </summary>
        /// <returns>Settings Date</returns>
        DateTime GetDateSequenceFromSettingsTable();

        /// <summary>
        /// Return Settings Date
        /// </summary>
        /// <returns>Settings Date</returns>
        int GetSequenceFromSettingsTable();

        /// <summary>
        /// Update Method.
        /// </summary>
        /// <param name="settings">Update Date or Sequence</param>
        void UpdateSettingsDateSequence(object settings);

        /// <summary>
        /// Streams the BLOB file.
        /// </summary>
        /// <param name="fileName">Name of the file.</param>
        /// <param name="dataDirectory">The data directory.</param>
        /// <param name="timeSpan">The time span.</param>
        /// <returns>returns Stream.</returns>
        Stream StreamBlobFile(string fileName, string dataDirectory, TimeSpan? timeSpan = null);

        /// <summary>
        /// Inserts the entity.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="tableEntity">The table entity.</param>
        void InsertEntity(string tableName, DynamicTableEntity tableEntity);

        /// <summary>
        /// Inserts the batch.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="batchOperation">The batch operation.</param>
        void InsertBatch(string tableName, TableBatchOperation batchOperation);

        /// <summary>
        /// Gets the unscheduled adjustments.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <returns>returns adjustments.</returns>
        List<DttData> GetUnscheduledAdjustments(string tableName);
    }
}
