// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Entities;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents the Configuration interface into the data access layer component.
    /// </summary>
    public interface IDalcServiceCacheHelper
    {
        /// <summary>
        /// Fetches the Date or Sequence.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <returns>Entity Name.</returns>
        List<CacheObjectTvEngdata> FetchTvEngData(string tableName);

        /// <summary>
        /// Updates the table.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <returns>returns success code.</returns>
        List<LPAuxRegistration> FetchLpAuxAregistrations(string tableName);

        /// <summary>
        /// Fetches the PMSE assignments.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <returns>returns PMSE list.</returns>
        List<PmseAssignment> FetchPmseAssignments(string tableName);

        /// <summary>
        /// Fetches the RegionPolygons collections.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="partitionKey">The partitionKey indicating region Country region code.</param>
        /// <returns>Collection of RegionPolygonCache.</returns>
        List<RegionPolygonsCache> FetchRegionPolygons(string tableName, string partitionKey);
    }
}
