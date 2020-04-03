// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{    
    using System;
    using System.Security.Principal;
    using System.Threading.Tasks;
    using System.Web;
    using System.Web.Mvc;
    using System.Web.Optimization;
    using System.Web.Routing;
    using System.Web.Script.Serialization;
    using System.Web.Security;
    using Microsoft.Whitespace.Common;

    /// <summary>
    /// Defines application level events
    /// </summary>
    public class MvcApplication : System.Web.HttpApplication
    {
        /// <summary>
        /// register configurations in application start
        /// </summary>
        protected void Application_Start()
        {
            Bootstrapper.Initialize();

            FilterConfig.RegisterGlobalFilters(GlobalFilters.Filters);
            RouteConfig.RegisterRoutes(RouteTable.Routes);
            BundleConfig.RegisterBundles(BundleTable.Bundles);
            AuthConfig.RegisterAuth();
                        
            Task cacheIntializer = new Task(() =>
            {
                DatabaseCache.ServiceCacheHelper.DownloadObjectsForServiceCache();
            });

            cacheIntializer.Start();
        }

        protected void Application_PostAuthenticateRequest(object sender, EventArgs e)
        {
            HttpCookie cookie = Request.Cookies[FormsAuthentication.FormsCookieName];

            if (cookie != null)
            {
                FormsAuthenticationTicket authTicket = FormsAuthentication.Decrypt(cookie.Value);

                //// JavaScriptSerializer serializer = new JavaScriptSerializer();

                if (authTicket.UserData == "OAuth")
                {
                    return;
                }

                // UserPrincipalSerializationModel serializationModel = serializer.Deserialize<UserPrincipalSerializationModel>(authTicket.UserData);
                var userData = authTicket.UserData.Split(':');
                bool isSuperAdmin = Convert.ToInt32(userData[0]) == 1 ? true : false;
                bool isRegionAdmin = Convert.ToInt32(userData[1]) == 1 ? true : false;

                IPrincipal userPrincipal = new UserPrincipal(userData[2], authTicket.Name, isSuperAdmin, isRegionAdmin);
                HttpContext.Current.User = userPrincipal;
            }
        }
    }
}
