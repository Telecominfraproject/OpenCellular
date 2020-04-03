// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.RegionManagement
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Web.Http;

    /// <summary>
    /// Perform web API Configuration.
    /// </summary>
    public static class WebApiConfig
    {
        /// <summary>
        /// Configures web API.
        /// </summary>
        /// <param name="config">Configuration of HttpServer instances.</param>
        public static void Register(HttpConfiguration config)
        {
            config.Routes.MapHttpRoute(
             name: "ApiById",
             routeTemplate: "api/{controller}/{id}",
             defaults: new { id = RouteParameter.Optional },
             constraints: new { id = @"^[0-9]+$" });

            config.Routes.MapHttpRoute(
                name: "ApiByName",
                routeTemplate: "api/{controller}/{action}/{name}",
                defaults: null,
                constraints: new { name = @"^[a-z]+$" });

            config.Routes.MapHttpRoute(
                name: "ApiByUserID",
                routeTemplate: "api/{controller}/{action}/{userId}",
               defaults: new { userId = RouteParameter.Optional });

            config.Routes.MapHttpRoute(
                name: "ApiByAction",
                routeTemplate: "api/{controller}/{action}",
                defaults: new { action = "Get" });

            // Uncomment the following line of code to enable query support for actions with an IQueryable or IQueryable<T> return type.
            // To avoid processing unexpected or malicious queries, use the validation settings on QueryableAttribute to validate incoming queries.
            // For more information, visit http://go.microsoft.com/fwlink/?LinkId=279712.
            // config.EnableQuerySupport();

            // To disable tracing in your application, please comment out or remove the following line of code
            // For more information, refer to: http://www.asp.net/web-api
            config.EnableSystemDiagnosticsTracing();
        }
    }
}
