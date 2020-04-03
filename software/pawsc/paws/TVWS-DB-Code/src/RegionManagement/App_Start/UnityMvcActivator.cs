// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.RegionManagement
{
    using System.Configuration;
    using System.Linq;
    using System.Web.Helpers;
    using System.Web.Http;
    using System.Web.Mvc;
    using Entities;
    using Microsoft.Practices.Unity.Mvc;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.WebApiHelper;
    using Microsoft.WindowsAzure.ServiceRuntime;
    using Newtonsoft.Json.Serialization;

    /// <summary>
    /// Unity Activation for MVC
    /// </summary>
    public static class UnityMvcActivator
    {
        internal static string AdminToolProductToken { get; set; }

        /// <summary>
        /// calls Start 
        /// </summary>
        public static void Start()
        {
            if (RoleEnvironment.IsAvailable)
            {
                AdminToolProductToken = RoleEnvironment.GetConfigurationSettingValue("AdminToolProductToken");
            }
            else
            {
                AdminToolProductToken = ConfigurationManager.AppSettings["AdminToolProductToken"];
            }

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
