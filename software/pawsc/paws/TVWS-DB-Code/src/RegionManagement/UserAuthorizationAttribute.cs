// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.RegionManagement
{
    using System;
    using System.Linq;
    using System.Net;
    using System.Net.Http;
    using System.Net.Http.Headers;
    using System.Web;
    using System.Web.Http;
    using System.Web.Http.Controllers;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using OAuthProvider = Microsoft.WhiteSpaces.AzureOAuthProvider;

    /// <summary>Class UserAuthorizationAttribute.</summary>
    public class UserAuthorizationAttribute : AuthorizeAttribute
    {
        private readonly OAuthProvider.IUserManager userManager;

        /// <summary>Initializes a new instance of the <see cref="UserAuthorizationAttribute" /> class.</summary>
        public UserAuthorizationAttribute()
        {
            this.userManager = Utils.Configuration.CurrentContainer.Resolve<OAuthProvider.IUserManager>();
        }

        /// <summary>Indicates whether the specified control is authorized.</summary>
        /// <param name="actionContext">The context.</param>
        /// <returns>true if the control is authorized; otherwise, false.</returns>
        protected override bool IsAuthorized(HttpActionContext actionContext)
        {
            // Following code is to bypass authentication and authorization for an internal Admin/developer Tool [i.e Service Test App].
            if (this.IsRequestFromAdminUserAgent(actionContext))
            {
                return true;
            }

            AuthenticationHeaderValue authorizationHeader = actionContext.Request.Headers.Authorization;

            if (authorizationHeader != null)
            {
                // TODO: Is it must to compare authorization schema equal "OAuth"?
                string accesstoken = authorizationHeader.Parameter;

                var userDetails = this.userManager.GetUserDetailsByAccessToken(accesstoken);

                if (userDetails != null)
                {
                    bool isAuthorized = userDetails.UserInfo.IsSuperAdmin;

                    if (!isAuthorized)
                    {
                        string[] accessLevels = this.Roles.ToString().Split(',');
                        isAuthorized = userDetails.AccessInfo.Any(user => accessLevels.Contains(user.AccessLevel.ToString()));
                    }

                    if (isAuthorized)
                    {
                        // Is it required to add custom header to know the authorization status ? if yes, where and when it will be used ?
                        HttpContext.Current.Response.AddHeader("AuthenticationStatus", "Authorized");

                        return true;
                    }
                    else
                    {
                        // Is it required to add custom header to know the authorization status ? if yes, where and when it will be used ?
                        HttpContext.Current.Response.AddHeader("AuthenticationStatus", "Un-Authorized");
                        actionContext.Response = actionContext.Request.CreateResponse(HttpStatusCode.Unauthorized);

                        return false;
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Validates if the current request is from admin tool
        /// </summary>
        /// <param name="actionContext">context for the http request containing the user agent string.</param>
        /// <returns>Boolean value indicating is request from admin tool or not.</returns>
        private bool IsRequestFromAdminUserAgent(HttpActionContext actionContext)
        {
            if (string.IsNullOrWhiteSpace(UnityMvcActivator.AdminToolProductToken))
            {
                return false;
            }

            HttpHeaderValueCollection<ProductInfoHeaderValue> userAgents = actionContext.Request.Headers.UserAgent;

            // Note: Check if, product token obtained from the current request matches with configured internal admin/developer tool product token.
            return userAgents != null && userAgents.Any(productInfoHeader => string.Compare(productInfoHeader.Product.Name, UnityMvcActivator.AdminToolProductToken, StringComparison.OrdinalIgnoreCase) == 0);
        }
    }
}
