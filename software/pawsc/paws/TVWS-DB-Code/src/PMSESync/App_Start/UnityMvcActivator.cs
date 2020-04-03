// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//------------------------------------------------------------------------------------------------- 
// <copyright file="UnityMvcActivator.cs" company="Microsoft">
//      Copyright (c) 2014 Microsoft Corporation. All rights reserved.
//      Licensed under the Microsoft Limited Public License (the "License"); you may not use these 
//      files except in compliance with the License. You may obtain a copy of the License at 
//      http://clrinterop.codeplex.com/license. Unless required by applicable law or agreed to in 
//      writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT 
//      WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the 
//      specific language governing permissions and limitations under the License.
// </copyright>
//------------------------------------------------------------------------------------------------- 

using System.Linq;
using System.Web.Mvc;
using Microsoft.Practices.Unity.Mvc;

[assembly: WebActivatorEx.PreApplicationStartMethod(typeof(Microsoft.Whitespace.PMSESync.App_Start.UnityWebActivator), "Start")]
[assembly: WebActivatorEx.ApplicationShutdownMethod(typeof(Microsoft.Whitespace.PMSESync.App_Start.UnityWebActivator), "Shutdown")]

namespace Microsoft.Whitespace.PMSESync.App_Start
{
    /// <summary>Provides the bootstrapping for integrating Unity with ASP.NET MVC.</summary>
    public static class UnityWebActivator
    {
        /// <summary>Integrates Unity when the application starts.</summary>
        public static void Start() 
        {
            var container = UnityConfig.GetConfiguredContainer();

            FilterProviders.Providers.Remove(FilterProviders.Providers.OfType<FilterAttributeFilterProvider>().First());
            FilterProviders.Providers.Add(new UnityFilterAttributeFilterProvider(container));

            DependencyResolver.SetResolver(new UnityDependencyResolver(container));

            // TODO: Uncomment if you want to use PerRequestLifetimeManager
            // Microsoft.Web.Infrastructure.DynamicModuleHelper.DynamicModuleUtility.RegisterModule(typeof(UnityPerRequestHttpModule));
        }

        /// <summary>Disposes the Unity container when the application is shut down.</summary>
        public static void Shutdown()
        {
            var container = UnityConfig.GetConfiguredContainer();
            container.Dispose();
        }
    }
}
