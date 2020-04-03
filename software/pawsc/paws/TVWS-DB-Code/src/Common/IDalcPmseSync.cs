// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Defines all of the required audit methods used by the data access layer component.
    /// </summary>
    public interface IDalcPmseSync
    {
        /// <summary>
        /// Inserts the file data to database.
        /// </summary>
        /// <param name="filePath">The file path.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        bool InsertFileDataToDatabase(List<PmseAssignment> filePath);

        /// <summary>
        /// Inserts the file data to database.
        /// </summary>
        /// <param name="filePath">The file path.</param>
        /// <param name="tableName">Table Name.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        bool InsertDTTFileData(List<DTTDataAvailability> filePath, string tableName);

        /// <summary>
        /// Inserts the Program Making and Special Events unscheduled adjustment.
        /// </summary>
        /// <param name="filePath">The file path.</param>
        /// <param name="tableName">Name of the table.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        bool InsertPMSEUnscheduledAdjustment(List<DTTDataAvailability> filePath, string tableName);

        /// <summary>
        /// Fetches the entity.
        /// </summary>
        /// <typeparam name="T">Table type to be fetched.</typeparam>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns object.</returns>
        List<T> FetchEntity<T>(string tableName, object parameters) where T : ITableEntity, new();

        /// <summary>
        /// Inserts the entity.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="tableEntity">The table entity.</param>
        void InsertEntity(string tableName, DynamicTableEntity tableEntity);
    }
}
