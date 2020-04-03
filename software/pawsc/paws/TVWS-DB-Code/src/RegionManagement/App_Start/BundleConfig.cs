// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.RegionManagement
{
    using System.Web;
    using System.Web.Optimization;

    /// <summary>
    /// Class used to bundle multiple files into a single file (improve first page load performance).
    /// </summary>
    public class BundleConfig
    {
        /// <summary>
        /// Adds files to a bundle collection - for more information on Bundling, visit <see cref="http://go.microsoft.com/fwlink/?LinkId=254725"/>
        /// </summary>
        /// <param name="bundles">Contains and manages the set of registered bundle objects.</param>
        public static void RegisterBundles(BundleCollection bundles)
        {
            ////bundles.Add(new ScriptBundle("~/bundles/jquery").Include(
            ////            "~/Scripts/jquery-{version}.js"));

            ////bundles.Add(new ScriptBundle("~/bundles/jqueryui").Include(
            ////            "~/Scripts/jquery-ui-{version}.js"));

            ////bundles.Add(new ScriptBundle("~/bundles/jqueryval").Include(
            ////            "~/Scripts/jquery.unobtrusive*",
            ////            "~/Scripts/jquery.validate*"));

            ////// Use the development version of Modernizr to develop with and learn from. Then, when you're
            ////// ready for production, use the build tool at http://modernizr.com to pick only the tests you need.
            ////bundles.Add(new ScriptBundle("~/bundles/modernizr").Include(
            ////            "~/Scripts/modernizr-*"));

            ////bundles.Add(new StyleBundle("~/Content/css").Include("~/Content/site.css"));

            ////bundles.Add(new StyleBundle("~/Content/themes/base/css").Include(
            ////            "~/Content/themes/base/jquery.ui.core.css",
            ////            "~/Content/themes/base/jquery.ui.resizable.css",
            ////            "~/Content/themes/base/jquery.ui.selectable.css",
            ////            "~/Content/themes/base/jquery.ui.accordion.css",
            ////            "~/Content/themes/base/jquery.ui.autocomplete.css",
            ////            "~/Content/themes/base/jquery.ui.button.css",
            ////            "~/Content/themes/base/jquery.ui.dialog.css",
            ////            "~/Content/themes/base/jquery.ui.slider.css",
            ////            "~/Content/themes/base/jquery.ui.tabs.css",
            ////            "~/Content/themes/base/jquery.ui.datepicker.css",
            ////            "~/Content/themes/base/jquery.ui.progressbar.css",
            ////            "~/Content/themes/base/jquery.ui.theme.css"));
        }
    }
}
