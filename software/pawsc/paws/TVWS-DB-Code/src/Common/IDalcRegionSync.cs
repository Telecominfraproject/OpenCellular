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
    /// Represents the RegionSync interface into the data access layer component.
    /// </summary>
    public interface IDalcRegionSync
    {
        /// <summary>
        /// Updates the specified table.
        /// </summary>
        /// <param name="tableName">Name of the table being updated.</param>
        /// <param name="tableRows">All the rows within the table being updated.</param>
        /// <param name="partitionKey">The partition key.</param>
        /// <returns>returns success code.</returns>
        int UpdateTable(string tableName, List<SyncRow> tableRows, string partitionKey);

        /// <summary>
        /// Updated the sync status for the specified database.
        /// </summary>
        /// <param name="syncRegionName">Name of the synchronize region.</param>
        /// <param name="syncEntityName">Name of the synchronize entity.</param>
        /// <param name="success">Indicates if the database was successfully synchronized.</param>
        void SetDatabaseSyncStatus(string syncRegionName, string syncEntityName, bool success);

        /// <summary>
        /// Updates the CDBS data.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="cdbsData">The CDBS data.</param>
        /// <returns>returns status.</returns>
        int UpdateCDBSData(string tableName, List<CDBSTvEngData> cdbsData);

        /// <summary>
        /// Deletes the CDBS data.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="cdbsData">The CDBS data.</param>
        void DeleteCDBSData(string tableName, List<CDBSTvEngData> cdbsData);

        /// <summary>
        /// Deletes the ULS data.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="ulsdata">The ULS data.</param>
        void DeleteULSData(string tableName, List<ULSRecord> ulsdata);

        /// <summary>
        /// Updates the ULS data.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="ulsData">The ULS data.</param>
        /// <returns>returns status.</returns>
        int UpdateULSData(string tableName, List<ULSRecord> ulsData);

        /// <summary>
        /// Returns the last time the specified database was synchronized.
        /// </summary>
        /// <param name="name">The database name that is being queried(CDBS, ULS, EAS)</param>
        /// <returns>DateTime of the last successful sync time.</returns>
        System.DateTime GetLastsuccessfulSyncTime(string name);

        /// <summary>
        /// Updates the authorized device records.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="deviceRecords">The device records.</param>
        /// <returns>returns status.</returns>
        int UpdateAuthorizedDeviceRecords(string tableName, List<AuthorizedDeviceRecord> deviceRecords);

        /// <summary>
        /// Insert Portal contour records in to Portal Summary and Portal contours tables
        /// </summary>
        /// <param name="contours">records to be inserted</param>
        /// <param name="incumbentType">incumbent type</param>
        void UpdatePortalContoursAndSummary(List<PortalContour> contours, int incumbentType);
    }
}
