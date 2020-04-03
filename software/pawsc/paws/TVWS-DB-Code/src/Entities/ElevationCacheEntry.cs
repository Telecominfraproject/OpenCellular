// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.WindowsAzure.Storage.Blob.Protocol;

    /// <summary>
    /// Elevation Cache Entry.
    /// </summary>
    [Serializable]
    public class ElevationCacheEntry
    {
        /// <summary>
        /// Initializes a new instance of the ElevationCacheEntry class.
        /// </summary>
        /// <param name="data">Bytes of Data</param>       
        public ElevationCacheEntry(object data)
        {
            this.Data = data;
            this.LastAccessed = DateTime.Now;
            this.AccessCount = 1;
        }

        /// <summary>
        /// Gets or sets the Bytes of Data.
        /// </summary>
        public object Data { get; set; }

        /// <summary>
        /// Gets or sets the Last Accessed
        /// </summary>
        public DateTime LastAccessed { get; set; }

        /// <summary>
        /// Gets or sets the Access Count
        /// </summary>
        public int AccessCount { get; set; }

        /// <summary>
        /// Update Details of Elevation Cache Entry.
        /// </summary>        
        public void RefreshCacheDetails()
        {
            this.LastAccessed = DateTime.Now;
            this.AccessCount += 1;
        }
    }
}
