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
    /// Represents the DB Sync Poller interface used by the azure worker thread.
    /// </summary>
    public interface IDBSyncPoller
    {
        /// <summary>
        /// Polls from all of the registered WSDBAs.
        /// </summary>
        void Poll();
    }
}
