// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.PAWS.Service
{   
    using System.Threading.Tasks;
    using System.Web.Http;
    using System.Web.Mvc;
    using System.Web.Optimization;
    using System.Web.Routing;
    using Microsoft.Whitespace.Common;
    using Microsoft.WindowsAzure.ServiceRuntime;

    /// <summary>
    /// Defines methods, properties and events for the application start page.
    /// </summary>
    /// <remarks>
    /// Note: For instructions on enabling IIS6 or IIS7 classic mode, 
    /// visit http://go.microsoft.com/?LinkId=9394801
    /// </remarks>
    public class WebApiApplication : System.Web.HttpApplication
    {
        /// <summary>
        /// Starts the application.
        /// </summary>
        protected void Application_Start()
        {
            UnityMvcActivator.Start();
            AreaRegistration.RegisterAllAreas();

            WebApiConfig.Register(GlobalConfiguration.Configuration);
            FilterConfig.RegisterGlobalFilters(GlobalFilters.Filters);
            RouteConfig.RegisterRoutes(RouteTable.Routes);
            BundleConfig.RegisterBundles(BundleTable.Bundles);

            if (RoleEnvironment.IsAvailable && !RoleEnvironment.IsEmulated)
            {
                // Api start up should not get delayed because of first time cache data downloading operation.
                Task cacheWorker = new Task(() =>
                {
                    DatabaseCache.ServiceCacheHelper.DownloadObjectsForServiceCache();
                });

                cacheWorker.Start();
            }
        }
    }
}
