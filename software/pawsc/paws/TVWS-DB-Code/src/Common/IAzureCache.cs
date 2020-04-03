// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//------------------------------------------------------------------------------------------------- 
// <copyright file="ICache.cs" company="Microsoft">
//     Copyright (c) 2013 Microsoft. All rights reserved.
// </copyright>
//------------------------------------------------------------------------------------------------- 

namespace Microsoft.Whitespace.Common
{
    using Microsoft.Whitespace.Entities;
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Interface is used for storing and retrieving objects from the cache.
    /// </summary>
    public interface IAzureCache
    {       

        /// <summary>
        /// Gets an object from the cache using the specified key.
        /// </summary>
        /// <param name="key">The unique value that is used to identify the object in the cache</param>
        /// <returns>The object that was cached by using the specified key. Null is returned if the key does not exist.</returns>
        byte[] Get(string Filename, int FirstByteIndex, int SecondByteIndex);

        /// <summary>
        /// Gets an object from the cache using the specified key.
        /// </summary>
        /// <param name="key">The unique value that is used to identify the object in the cache</param>
        /// <returns>The object that was cached by using the specified key. Null is returned if the key does not exist.</returns>
        void Put(string Filename);

        /// <summary>
        /// Gets an object from the cache using the specified key.
        /// </summary>
        /// <param name="key">The unique value that is used to identify the object in the cache</param>
        /// <returns>The object that was cached by using the specified key. Null is returned if the key does not exist.</returns>
        void Update(string Filename);
    }
}
