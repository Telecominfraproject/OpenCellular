// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    
    /// <summary>
    /// Interface used for performing region sync with government DBs.
    /// </summary>
    public interface IRegionSync
    {
        /// <summary>
        /// Performs a DB Sync with all government DBs.
        /// </summary>
        void SyncDB();
    }
}
