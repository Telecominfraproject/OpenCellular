// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System.Web.Mvc;

    /// <summary>
    /// Register filters to Global filter collection
    /// </summary>
    public class FilterConfig
    {
        /// <summary>
        /// Register new filter to global filter collection
        /// </summary>
        /// <param name="filters">global filter collection</param>
        public static void RegisterGlobalFilters(GlobalFilterCollection filters)
        {
            filters.Add(new CustomHandleErrorAttribute());
            filters.Add(new AuthorizeAttribute());
        }
    }
}
