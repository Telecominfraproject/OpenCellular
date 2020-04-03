// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.RegionManagement
{
    using System.Web;
    using System.Web.Mvc;

    /// <summary>
    /// Class used for registering global filters.
    /// </summary>
    public class FilterConfig
    {
        /// <summary>
        /// Register all global filters such as error handling.
        /// </summary>
        /// <param name="filters">Global filter collection whose filters are to be updated.</param>
        public static void RegisterGlobalFilters(GlobalFilterCollection filters)
        {
            filters.Add(new HandleErrorAttribute());
        }
    }
}
