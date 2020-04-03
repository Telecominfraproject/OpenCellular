// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using DotNetOpenAuth.AspNet;
    using Microsoft.Practices.Unity;
    using Microsoft.Web.WebPages.OAuth;
    using Microsoft.WhiteSpaces.Common;

    /// <summary>
    /// Configures provider id
    /// </summary>
    public static class AuthConfig
    {
        public static void RegisterAuth()
        {           
            var providerClient = ConfigHelper.CurrentContainer.Resolve<IAuthenticationClient>();

            OAuthWebSecurity.RegisterClient(providerClient);
        }
    }
}
