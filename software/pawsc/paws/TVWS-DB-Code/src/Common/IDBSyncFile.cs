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

    /// <summary>
    /// Represents the DB Sync interface (used by azure worker thread).
    /// </summary>
    public interface IDBSyncFile
    {
        /// <summary>
        /// Generates the DB-to-DB sync file and places it up on the FTP server.
        /// </summary>
        /// <param name="scope"> DB Sync Scope ALL/INCR</param>
        void GenerateFile(DbSyncScope scope);
    }
}
