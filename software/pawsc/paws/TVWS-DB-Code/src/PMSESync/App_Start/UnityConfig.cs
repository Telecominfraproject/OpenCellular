// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//------------------------------------------------------------------------------------------------- 
// <copyright file="UnityConfig.cs" company="Microsoft">
//      Copyright (c) 2014 Microsoft Corporation. All rights reserved.
//      Licensed under the Microsoft Limited Public License (the "License"); you may not use these 
//      files except in compliance with the License. You may obtain a copy of the License at 
//      http://clrinterop.codeplex.com/license. Unless required by applicable law or agreed to in 
//      writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT 
//      WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the 
//      specific language governing permissions and limitations under the License.
// </copyright>
//------------------------------------------------------------------------------------------------- 

using System;
using Microsoft.Practices.Unity;
using Microsoft.Practices.Unity.Configuration;

namespace Microsoft.Whitespace.PMSESync.App_Start
{
    /// <summary>
    /// Specifies the Unity configuration for the main container.
    /// </summary>
    public class UnityConfig
    {
        #region Unity Container
        private static Lazy<IUnityContainer> container = new Lazy<IUnityContainer>(() =>
        {
            var container = new UnityContainer();
            RegisterTypes(container);
            return container;
        });

        /// <summary>
        /// Gets the configured Unity container.
        /// </summary>
        public static IUnityContainer GetConfiguredContainer()
        {
            return container.Value;
        }
        #endregion

        /// <summary>Registers the type mappings with the Unity container.</summary>
        /// <param name="container">The unity container to configure.</param>
        /// <remarks>There is no need to register concrete types such as controllers or API controllers (unless you want to 
        /// change the defaults), as Unity allows resolving a concrete type even if it was not previously registered.</remarks>
        public static void RegisterTypes(IUnityContainer container)
        {
            // NOTE: To load from web.config uncomment the line below. Make sure to add a Microsoft.Practices.Unity.Configuration to the using statements.
            // container.LoadConfiguration();

            // TODO: Register your types here
            // container.RegisterType<IProductRepository, ProductRepository>();
        }
    }
}
