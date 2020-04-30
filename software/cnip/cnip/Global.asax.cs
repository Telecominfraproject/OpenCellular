using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Web;
using System.Web.Mvc;
using System.Web.Optimization;
using System.Web.Routing;
using System.Web.Security;
using System.Web.SessionState;
using OSGeo.GDAL;
using OSGeo.OGR;
using OSGeo.OSR;
using cnip.Models;
using System.Data;
using System.Web.UI;
using System.Web.Services;
using System.Xml;
using Newtonsoft;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Diagnostics;
using System.Text;

namespace cnip
{
    public class MvcApplication : System.Web.HttpApplication
    {
        protected void Application_Start()
        {
            Application["Name"] = "CNIP";
            AreaRegistration.RegisterAllAreas();
            FilterConfig.RegisterGlobalFilters(GlobalFilters.Filters);
            RouteConfig.RegisterRoutes(RouteTable.Routes);
            BundleTable.EnableOptimizations = true;
            BundleConfig.RegisterBundles(BundleTable.Bundles);
            GdalConfiguration.ConfigureGdal();
            GdalConfiguration.ConfigureOgr();
        }
    }
}
