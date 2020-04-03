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
    /// Interface is used for storing and retrieving objects from the cache.
    /// </summary>
    public interface ICache
    {
        /// <summary>
        /// Gets an object from the cache using the specified key.
        /// </summary>
        /// <param name="fileName">Name of the file.</param>
        /// <returns>The object that was cached by using the specified key. Null is returned if the key does not exist.</returns>
        object ReadCachedData(string fileName);

        /// <summary>
        /// Gets the memory usage.
        /// </summary>
        /// <returns>returns memory usage of cache.</returns>
        double GetMemoryUsage();
    }
}
