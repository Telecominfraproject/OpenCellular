// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Web;
    using DotNetOpenAuth.AspNet;
    using DotNetOpenAuth.AspNet.Clients;
    using Microsoft.WhiteSpaces.Common;    

    public class MicrosoftOAuthClient : IAuthenticationClient
    {
        private readonly MicrosoftClient client;

        private Uri returnUrl;

        public MicrosoftOAuthClient(string clientId, string clientSecretId)
            : this(clientId, clientSecretId, null)
        { 
        }

        public MicrosoftOAuthClient(string clientId, string clientSecretId, string requestScopes)
        {
            Check.IsNotNull(clientId, "ClientId");
            Check.IsNotNull(clientSecretId, "ClientSecretId");

            if (requestScopes != null && requestScopes.Count() <= 0)
            {
                this.client = new MicrosoftClient(clientId, clientSecretId);
            }
            else
            {
                var scopes = requestScopes.Split(',');
                this.client = new MicrosoftClient(clientId, clientSecretId, scopes);
            }
        }

        public string ProviderName
        {
            get { return this.client != null ? this.client.ProviderName : Constants.MicrosoftProvider; }
        }

        public void RequestAuthentication(HttpContextBase context, Uri returnUrl)
        {
            this.returnUrl = returnUrl;
            this.client.RequestAuthentication(context, returnUrl);
        }

        public AuthenticationResult VerifyAuthentication(HttpContextBase context)
        {
            if (this.returnUrl == null || !this.returnUrl.IsWellFormedOriginalString())
            {
                throw new UriFormatException("Invalid return url");
            }

            return this.client.VerifyAuthentication(context, this.returnUrl);
        }
    }
}
