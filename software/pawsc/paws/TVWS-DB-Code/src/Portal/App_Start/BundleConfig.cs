// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System.Web.Optimization;

    /// <summary>
    /// Create a bundle for all the scripts and style sheets
    /// </summary>
    public class BundleConfig
    {
        /// <summary>
        /// Register script and style sheets file in a bundle
        /// </summary>
        /// <param name="bundles">Bundle collection</param>
        public static void RegisterBundles(BundleCollection bundles)
        {
            bundles.UseCdn = true;
            BundleTable.EnableOptimizations = true;

            bundles.Add(new ScriptBundle("~/bundles/jquery", "http://ajax.aspnetcdn.com/ajax/jQuery/jquery-2.1.0.js").Include("~/Scripts/jquery-{version}.js")); // TODO: Why Include method be called here?
            bundles.Add(new ScriptBundle("~/bundles/jqueryui", "http://ajax.aspnetcdn.com/ajax/jquery.ui/1.10.0/jquery-ui.min.js").Include("~/Scripts/jquery-ui-{version}.js"));
            bundles.Add(new ScriptBundle("~/bundles/jqueryunobtrusive").Include("~/Scripts/jquery.unobtrusive-ajax.min.js"));
            bundles.Add(new ScriptBundle("~/bundles/slimScroll").Include("~/Scripts/jquery.slimscroll.js"));
           
            bundles.Add(new StyleBundle("~/Content/theme", "http://ajax.aspnetcdn.com/ajax/jquery.ui/1.10.0/themes/redmond/jquery-ui.css"));
            bundles.Add(new StyleBundle("~/Content/main").Include("~/Content/master.css"));
            bundles.Add(new StyleBundle("~/Content/overridden").Include("~/Content/overridden.css"));
            bundles.Add(new StyleBundle("~/Content/perfect-scrollbar").Include("~/Content/perfect-scrollbar.css"));
        }
    }
}
