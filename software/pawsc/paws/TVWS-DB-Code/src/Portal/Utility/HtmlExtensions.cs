// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Web;
    using System.Web.Mvc;

    public static class HtmlExtensions
    {
        public static string FormattedText(this HtmlHelper helper, string target)
        {
            if (target.Length > 12)
            {
                return string.Format(target.Substring(0, 11) + "...");
            }

            return target;
        }
    }
}
