// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents a table that is to be updated from the Protected Region Worker.
    /// </summary>
    public class SyncTable
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SyncTable"/> class.
        /// </summary>
        /// <param name="dataRows">The data rows.</param>
        public SyncTable(List<SyncRow> dataRows)
        {
            this.Rows = dataRows;
        }

        /// <summary>
        /// Gets the table rows.
        /// </summary>
        public List<SyncRow> Rows { get; private set; }
    }
}
