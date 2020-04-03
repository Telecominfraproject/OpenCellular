// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System;
    using System.Web.Mvc;
    using DotNetOpenAuth.AspNet;
    using Microsoft.Practices.EnterpriseLibrary.TransientFaultHandling;
    using Microsoft.Practices.Unity;
    using Microsoft.WhiteSpaces.AzureOAuthProvider;
    using Microsoft.WhiteSpaces.BusinessManager;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.DataAccessManager;
    using Unity.Mvc5;

    /// <summary>
    /// Configures Unity
    /// </summary>
    public static class Bootstrapper
    {
        /// <summary>
        /// Initialize unity container
        /// </summary>
        public static void Initialize()
        {
            // Unity container with loaded config values
            IUnityContainer container = ConfigHelper.CurrentContainer;

            Check.IsNotNull(container, "container");

            container.RegisterType<IAuthenticationClient, MicrosoftOAuthClient>(new InjectionConstructor(ConfigHelper.LiveClientId, ConfigHelper.LiveSecretClientId, ConfigHelper.RequestScopes));
            container.RegisterType<IRegionSource, AppConfigRegionSource>();

            RetryPolicy retryPolicy = new RetryPolicy<HttpClientTransientErrorDetectionStrategy>(3, TimeSpan.FromSeconds(2));

            container.RegisterType<IHttpClientManager, HttpClientManager>(
                "Real_HttpClientManager",
                new PerThreadLifetimeManager(),
                new InjectionConstructor(ConfigHelper.AuthorizationSchema));

            container.RegisterType<IHttpClientManager, RetryHttpClientManager>(
                "Retry_HttpClientManager",
                new InjectionConstructor(
                    new ResolvedParameter<IHttpClientManager>("Real_HttpClientManager"),
                    retryPolicy));

            container.RegisterType<IWhitespacesDataClient, WhitespacesDataClient>(
                new InjectionConstructor(new ResolvedParameter<IHttpClientManager>("Retry_HttpClientManager")));

            container.RegisterType<IPublicDataAccess, PublicDataAccess>(new InjectionConstructor(new ResolvedParameter<IHttpClientManager>("Retry_HttpClientManager")));

            // Resolve dependencies
            DependencyResolver.SetResolver(new UnityDependencyResolver(container));

            // Add infrastructure tables
            Infrastructure.CreateAccessLevels();
            Infrastructure.CreateAuthorities();
        }
    }
}
