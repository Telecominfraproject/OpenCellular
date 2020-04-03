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
    /// Represents the Incumbent interface into the data access layer component.
    /// </summary>
    public interface IDalcIncumbent
    {
        /// <summary>
        /// Gets all of the registered incumbents at the current region.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="columnNames">The column names.</param>
        /// <param name="filters">The filters.</param>
        /// <param name="rowCount">The row count.</param>
        /// <returns>Returns all of the registered incumbents at the current region.</returns>
        Incumbent[] GetIncumbents(string tableName, string[] columnNames = null, object filters = null, int rowCount = 0);

        /// <summary>
        /// Gets the combined incumbents.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="searchArea">The search area.</param>
        /// <returns>returns Incumbent[].</returns>
        Incumbent[] GetCombinedIncumbents(string tableName, SquareArea searchArea);

        /// <summary>
        /// Gets the PMSE assignments.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="searchArea">The search area.</param>
        /// <returns>returns Incumbent[][].</returns>
        PmseAssignment[] GetPMSEAssignments(string tableName, SquareArea searchArea);

        /// <summary>
        /// Gets the PMSE assignments.
        /// </summary>
        /// <param name="table">The table.</param>
        /// <param name="locations">The locations.</param>
        /// <returns>returns PMSEAssignment[][].</returns>
        PmseAssignment[] GetPMSEAssignments(string table, List<OSGLocation> locations);

        /// <summary>
        /// Gets the antenna pattern for specific incumbents.
        /// </summary>
        /// <param name="incumbentInfo">The incumbent information.</param>
        /// <returns>Returns the specified incumbent (or null if Id does not exist)</returns>
        double[] GetAntennaPatterns(Incumbent incumbentInfo);

        /// <summary>
        /// Updates the specified incumbent.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="incumbent">Incumbent that is to be updated.</param>
        void UpdateIncumbent(string tableName, Incumbent incumbent);

        /// <summary>
        /// Updates the specified incumbent.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="propertiesRow">The properties row.</param>
        void UpdateIncumbent(string tableName, SyncRow propertiesRow);

        /// <summary>
        /// Gets the incumbents inside a square area
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="squareArea">The square area.</param>
        /// <param name="columnNames">The column names.</param>
        /// <param name="filters">The filters.</param>
        /// <returns>returns Incumbents.</returns>
        Incumbent[] GetIncumbents(string tableName, SquareArea squareArea, string[] columnNames = null, object filters = null);

        /// <summary>
        /// Gets all stations.
        /// </summary>
        /// <param name="squareArea">The square area.</param>
        /// <returns>returns Incumbents.</returns>
        Incumbent[] GetAllStations(SquareArea squareArea);

        /// <summary>
        /// Filters the LP AUX entities.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="incumbentInfo">The incumbent information.</param>
        /// <param name="squareArea">The square area.</param>
        /// <param name="startDate">The start date.</param>
        /// <returns>returns Incumbents.</returns>
        Incumbent[] FilterLPAuxEntities(string tableName, Incumbent incumbentInfo, SquareArea squareArea, DateTime startDate);

        /// <summary>
        /// Adds the incumbents
        /// </summary>
        /// <param name="tableName">The table name.</param>
        /// <param name="regEntity">The incumbents list.</param>
        /// <returns>returns string.</returns>
        string InsertIncumbentData(string tableName, object regEntity);

        /// <summary>
        /// Deletes the incumbents
        /// </summary>
        /// <param name="tableName">The table name.</param>
        /// <param name="entity">The channel number.</param>
        /// <returns>returns string.</returns>
        string DeleteIncumbentData(string tableName, string entity);

        /// <summary>
        /// Gets the registered incumbents
        /// </summary>
        /// <typeparam name="T">type of incumbent</typeparam>
        /// <param name="incumbentType">Type of the incumbent.</param>
        /// <param name="tableName">The table name.</param>
        /// <param name="id">The user id.</param>
        /// <returns>returns list.</returns>
        List<T> GetIncumbentsData<T>(IncumbentType incumbentType, string tableName, string id) where T : ITableEntity, new();

        /// <summary>
        /// Adds to the list of excluded Ids
        /// </summary>
        /// <param name="entity">The excluded Ids list.</param>
        /// <returns>returns string.</returns>
        string ExcludeId(DynamicTableEntity entity);

        /// <summary>
        /// Excludes white space registration from the specified region
        /// </summary>
        /// <param name="entity">The excluded regions list.</param>
        /// <returns>returns string.</returns>
        string ExcludeChannel(DynamicTableEntity entity);

        /// <summary>
        /// Updates the specified table.
        /// </summary>
        /// <param name="tableName">Name of the table being updated.</param>
        /// <param name="tableRows">All the rows within the table being updated.</param>
        /// <param name="partitionKey">The partition key.</param>
        /// <returns>returns success code.</returns>
        int UpdateTable(string tableName, List<SyncRow> tableRows, string partitionKey);

        /// <summary>
        /// Retrieves an entity.
        /// </summary>
        /// <param name="table">Table that is to be fetched.</param>
        /// <param name="value">Second value that is to be fetched.</param>
        /// <returns>Entity Name.</returns>
        List<TableEntity> RetrieveEntity(string table, string value);

        /// <summary>
        /// Gets the DTT dataset values.
        /// </summary>
        /// <param name="tableName">The table.</param>
        /// <param name="queryParameters">The query parameters.</param>
        /// <returns>returns System.String[][].</returns>
        List<DttData> GetDTTDatasetValues(string tableName, List<OSGLocation> queryParameters);

        /// <summary>
        /// Gets the excluded regions.
        /// </summary>
        /// <returns>returns List excluded Regions.</returns>
        List<DynamicTableEntity> GetExcludedRegions();

        /// <summary>
        /// cancel the LP-AUX unlicensed registration
        /// </summary>
        /// <param name="id">registration id</param>
        void CancelRegistrations(string id);

        /// <summary>
        /// Gets the protected entity.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <returns>returns List{TableEntity}.</returns>
        List<TableEntity> GetProtectedEntity(string tableName);

        /// <summary>
        /// Gets the protected entity with events.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <returns>returns List{TableEntity}.</returns>
        List<TableEntity> GetProtectedEntityWithEvents(string tableName);

        /// <summary>
        /// Gets the protected entity us mexico.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="entityType">Type of the entity.</param>
        /// <returns>returns List{TableEntity}.</returns>
        List<TableEntity> GetProtectedEntityUSMexico(string tableName, string entityType);

        /// <summary>
        /// Gets the call sign array.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="filters">The filters.</param>
        /// <returns>returns String[][].</returns>
        string[] GetCallSignArray(string tableName, object filters = null);

        /// <summary>
        /// Saves the LPAUX registration details table.
        /// </summary>
        /// <param name="lowPowerAuxRegistrations">The LPAUX registrations.</param>
        void SaveLPAuxRegistrationDetailsTable(params LPAuxRegistration[] lowPowerAuxRegistrations);
    }
}
