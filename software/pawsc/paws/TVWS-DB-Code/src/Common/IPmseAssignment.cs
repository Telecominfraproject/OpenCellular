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
    /// Represents the Paws Business Layer interface.
    /// </summary>
    public interface IPmseAssignment
    {
        /// <summary>
        /// Synchronizes the database.
        /// </summary>
        void SyncDB();
    }
}
