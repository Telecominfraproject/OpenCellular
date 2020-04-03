// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{    
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Web;
    using System.Web.Mvc;
    using Microsoft.WhiteSpaces.Common;

    public class AuthorizeUserAttribute : AuthorizeAttribute
    {
        public string AccessLevels { get; set; }

        public string Authority { get; set; }

        public override void OnAuthorization(AuthorizationContext filterContext)
        {
            if (this.AuthorizeCore(filterContext.HttpContext))
            {
                base.OnAuthorization(filterContext);
            }
            else
            {
                this.HandleUnauthorizedRequest(filterContext);
            }
        }

        protected override bool AuthorizeCore(HttpContextBase httpContext)
        {
            var isAuthorized = base.AuthorizeCore(httpContext);
            if (!isAuthorized)
            {
                return false;
            }

            if (string.IsNullOrEmpty(this.AccessLevels))
            {
                return true;
            }
            else
            {
                UserPrincipal principal = (UserPrincipal)httpContext.User;

                string[] roles = this.AccessLevels.Split(',');

                foreach (string role in roles)
                {
                    AccessLevels level = (AccessLevels)Enum.Parse(typeof(AccessLevels), role);
                    Authorities authority = (Authorities)Enum.Parse(typeof(Authorities), this.Authority);

                    if (principal.IsInRole(level, authority))
                    {
                        return true;
                    }
                }

                return false;                
            }
        }       

        protected override void HandleUnauthorizedRequest(AuthorizationContext filterContext)
        {
            // TODO : Need to handle unauthorize access
            filterContext.HttpContext.Response.Redirect(filterContext.HttpContext.Request.UrlReferrer.ToString());
        }
    }
}
