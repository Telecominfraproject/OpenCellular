// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System;
    using System.Web.Mvc;
    using DotNetOpenAuth.AspNet.Clients;
    using Microsoft.Owin.Host.SystemWeb;
    using Microsoft.Web.WebPages.OAuth;    
    using Microsoft.WhiteSpaces.Common;

    /// <summary>
    /// External login result
    /// </summary>
    internal class ExternalLoginResult : ActionResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ExternalLoginResult"/> class
        /// </summary>
        /// <param name="provider">provider name</param>
        /// <param name="returnUrl">return URL</param>
        public ExternalLoginResult(string provider, string returnUrl)
        {
            this.Provider = provider;
            this.ReturnUrl = returnUrl;
        }

        /// <summary>
        /// Gets Provider name
        /// </summary>
        public string Provider { get; private set; }

        /// <summary>
        /// Gets Return URL
        /// </summary>
        public string ReturnUrl { get; private set; }

        /// <summary>
        /// Request authentication for a given controller context
        /// </summary>
        /// <param name="context">controller context</param>
        public override void ExecuteResult(ControllerContext context)
        {  
           OAuthWebSecurity.RequestAuthentication(this.Provider, this.ReturnUrl);           
        }
    }
}
