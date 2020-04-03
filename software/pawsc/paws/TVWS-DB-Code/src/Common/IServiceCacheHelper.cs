// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System.Collections.Generic;
    using System.Security.Cryptography.X509Certificates;
    using Entities;

    /// <summary>
    ///     Interface IServiceCacheHelper
    /// </summary>
    public interface IServiceCacheHelper
    {
        /// <summary>
        ///     Downloads the objects for service cache.
        /// </summary>
        void DownloadObjectsForServiceCache();

        /// <summary>
        /// Gets the service cache objects.
        /// </summary>
        /// <param name="cacheObjectType">Type of the cache object.</param>
        /// <param name="serviceCacheRequestParameters">The service cache request parameters.</param>
        /// <returns>returns List of cache data.</returns>
        object GetServiceCacheObjects(ServiceCacheObjectType cacheObjectType, ServiceCacheRequestParameters serviceCacheRequestParameters);

        /// <summary>
        /// Searches the cache objects.
        /// </summary>
        /// <param name="cacheObjectType">Type of the cache object.</param>
        /// <param name="searchCacheRequestType">Type of the search cache request.</param>
        /// <param name="serviceCacheRequestParameters">The service cache request parameters.</param>
        /// <returns>returns System.Object.</returns>
        object SearchCacheObjects(ServiceCacheObjectType cacheObjectType, SearchCacheRequestType searchCacheRequestType, ServiceCacheRequestParameters serviceCacheRequestParameters);
    }
}
