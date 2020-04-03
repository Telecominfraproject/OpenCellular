// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System.Linq;
    using Practices.Unity;

    /// <summary>
    ///     Represents Class DatabaseCache.
    /// </summary>
    public static class DatabaseCache
    {
        /// <summary>The _sync root</summary>
        private static object syncRoot = new object();

        /// <summary>
        ///     Initializes static members of the <see cref="DatabaseCache" /> class.
        /// </summary>
        static DatabaseCache()
        {
            lock (syncRoot)
            {
                if (ServiceCacheHelper == null)
                {
                    ServiceCacheHelper = Utils.Configuration.CurrentContainer.Resolve<IServiceCacheHelper>();
                }
            }
        }

        /// <summary>
        /// Gets or sets the service cache helper.
        /// </summary>
        /// <value>The service cache helper.</value>
        public static IServiceCacheHelper ServiceCacheHelper { get; set; }
    }
}
