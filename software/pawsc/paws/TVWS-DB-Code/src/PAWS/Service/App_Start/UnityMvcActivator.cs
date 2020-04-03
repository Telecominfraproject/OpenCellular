// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.PAWS.Service
{
    using System.Linq;
    using System.Web.Helpers;
    using System.Web.Http;
    using System.Web.Mvc;
    using Entities;
    using Microsoft.Practices.Unity.Mvc;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.WebApiHelper;
    using Newtonsoft.Json.Serialization;

    /// <summary>
    /// Unity Activation for MVC
    /// </summary>
    public static class UnityMvcActivator
    {
        /// <summary>B
        /// calls Start 
        /// </summary>
        public static void Start()
        {
            var container = Utils.Configuration.CurrentContainer;

            FilterProviders.Providers.Remove(FilterProviders.Providers.OfType<FilterAttributeFilterProvider>().First());
            FilterProviders.Providers.Add(new UnityFilterAttributeFilterProvider(container));

            DependencyResolver.SetResolver(new UnityDependencyResolver(container));

            var currentJsonFormatter = GlobalConfiguration.Configuration.Formatters.JsonFormatter;
            currentJsonFormatter.SerializerSettings.NullValueHandling = JsonSerialization.PawsJsonSerializerSetting.NullValueHandling;
            currentJsonFormatter.SerializerSettings.DefaultValueHandling = JsonSerialization.PawsJsonSerializerSetting.DefaultValueHandling;
            currentJsonFormatter.SerializerSettings.ContractResolver = JsonSerialization.PawsJsonSerializerSetting.ContractResolver;

            GlobalConfiguration.Configuration.DependencyResolver = new ApiDependencyResolver(container);
        }
    }
}
