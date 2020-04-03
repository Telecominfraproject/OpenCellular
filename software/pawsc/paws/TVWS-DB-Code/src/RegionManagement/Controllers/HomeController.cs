// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.RegionManagement.Controllers
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Web;
    using System.Web.Mvc;
    using Microsoft.Whitespace.Common;

    /// <summary>
    /// Controller for the home page.
    /// </summary>
    public class HomeController : Controller
    {
        /// <summary>
        /// Generates the index page.
        /// </summary>
        /// <returns>Return the home page.</returns>
        public ActionResult Index()
        {
            return this.View();
        }
    }
}
