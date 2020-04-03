// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.WebApiHelper
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;
    using System.Web.Http.Dependencies;
    using Microsoft.Practices.Unity;

    /// <summary>
    /// unity Dependency scope for API Dependency resolver
    /// </summary>
    public sealed class UnityDependencyScope : IDependencyScope
    {
        /// <summary>
        /// stores UnityContainer
        /// </summary>
        private IUnityContainer container;

        /// <summary>
        /// Initializes a new instance of the <see cref="UnityDependencyScope"/> class for a container.
        /// </summary>
        /// <param name="container">The <see cref="IUnityContainer"/> to wrap with the <see cref="IDependencyResolver"/>
        /// interface implementation.</param>
        public UnityDependencyScope(IUnityContainer container)
        {
            this.container = container;
        }

        /// <summary>
        /// return Resolved type
        /// </summary>
        /// <param name="serviceType">service Type</param>
        /// <returns>returns resolved service type</returns>
        public object GetService(Type serviceType)
        {
            return this.container.Resolve(serviceType);
        }

        /// <summary>
        /// returns Resolved Types
        /// </summary>
        /// <param name="serviceType">service type</param>
        /// <returns>returns resolved service types</returns>
        public IEnumerable<object> GetServices(Type serviceType)
        {
            return this.container.ResolveAll(serviceType);
        }

        /// <summary>
        /// calls dispose
        /// </summary>
        public void Dispose()
        {
        }
    }
}
