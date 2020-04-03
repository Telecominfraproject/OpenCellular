// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

[assembly: WebActivatorEx.PreApplicationStartMethod(typeof(Microsoft.Whitespace.PAWS.Service.App_Start.UnityWebApiActivator), "Start")]
[assembly: WebActivatorEx.ApplicationShutdownMethod(typeof(Microsoft.Whitespace.PAWS.Service.App_Start.UnityWebApiActivator), "Shutdown")]
namespace Microsoft.Whitespace.PAWS.Service.App_Start
{
    using System.Collections.Generic;
    using System.Linq;
    using System.Web.Http;
    using System.Web.Http.Filters;
    using Microsoft.Practices.Unity;
    using Microsoft.Practices.Unity.WebApi;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;
    using Microsoft.Whitespace.PAWS.Service.ActionFilters;

    /// <summary>Provides the bootstrapping for integrating Unity with WebApi when it is hosted in ASP.NET</summary>
    public static class UnityWebApiActivator
    {
        /// <summary>Integrates Unity when the application starts.</summary>
        public static void Start()
        {
            IUnityContainer container = Utils.Configuration.CurrentContainer;
            container.RegisterType<IActionFilter, ValidateLocationAttribute>();

            List<IFilterProvider> filterProviders = GlobalConfiguration.Configuration.Services.GetFilterProviders().ToList();

            GlobalConfiguration.Configuration.Services.Add(typeof(IFilterProvider), new UnityWebApiFilterAttributeFilterProvider(container));

            IFilterProvider defaultProvider = filterProviders.FirstOrDefault(provider => provider is ActionDescriptorFilterProvider);

            if (defaultProvider != null)
            {
                GlobalConfiguration.Configuration.Services.Remove(typeof(IFilterProvider), defaultProvider);
            }

            // Use UnityHierarchicalDependencyResolver if you want to use a new child container for each IHttpController resolution.
            // var resolver = new UnityHierarchicalDependencyResolver(UnityConfig.GetConfiguredContainer());
            var resolver = new UnityDependencyResolver(container);

            var currentJsonFormatter = GlobalConfiguration.Configuration.Formatters.JsonFormatter;
            currentJsonFormatter.SerializerSettings.NullValueHandling = JsonSerialization.PawsJsonSerializerSetting.NullValueHandling;
            currentJsonFormatter.SerializerSettings.DefaultValueHandling = JsonSerialization.PawsJsonSerializerSetting.DefaultValueHandling;
            currentJsonFormatter.SerializerSettings.ContractResolver = JsonSerialization.PawsJsonSerializerSetting.ContractResolver;

            GlobalConfiguration.Configuration.DependencyResolver = resolver;
        }

        /// <summary>Disposes the Unity container when the application is shut down.</summary>
        public static void Shutdown()
        {
            var container = Utils.Configuration.CurrentContainer;
            container.Dispose();
        }
    }
}
