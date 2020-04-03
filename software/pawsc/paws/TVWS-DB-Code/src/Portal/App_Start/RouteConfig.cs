// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System.Web.Mvc;
    using System.Web.Routing;

    /// <summary>
    /// Register routes
    /// </summary>
    public class RouteConfig
    {
        /// <summary>
        /// Register user defined routes
        /// </summary>
        /// <param name="routes">route collection</param>
        public static void RegisterRoutes(RouteCollection routes)
        {
            routes.MapRoute(
               "Default",
               "{controller}/{action}/{id}",
               new { controller = "Home", action = "Index", id = UrlParameter.Optional },
               new string[] { "Microsoft.WhiteSpaces.Portal" });  
        }
    }
}
