// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.WebApiHelper
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Web.Http.Dependencies;
    using Microsoft.Practices.Unity;

    /// <summary>
    /// to resolve API dependencies
    /// </summary>
    public sealed class ApiDependencyResolver : IDependencyResolver
    {
        /// <summary>
        /// current container
        /// </summary>
        private IUnityContainer container;

        /// <summary>
        /// shared scope container
        /// </summary>
        private UnityDependencyScope sharedScope;

        /// <summary>
        /// Initializes a new instance of the <see cref="ApiDependencyResolver"/> class for a container.
        /// </summary>
        /// <param name="container">The <see cref="IUnityContainer"/> to wrap with the <see cref="IDependencyResolver"/>
        /// interface implementation.</param>
        public ApiDependencyResolver(IUnityContainer container)
        {
            if (container == null)
            {
                throw new ArgumentNullException("container");
            }

            this.container = container;
            this.sharedScope = new UnityDependencyScope(container);
        }

        /// <summary>
        /// Reuses the same scope to resolve all the instances.
        /// </summary>
        /// <returns>The shared dependency scope.</returns>
        public IDependencyScope BeginScope()
        {
            return this.sharedScope;
        }

        /// <summary>
        /// Disposes the wrapped <see cref="IUnityContainer"/>.
        /// </summary>
        public void Dispose()
        {
            this.container.Dispose();
            this.sharedScope.Dispose();
        }

        /// <summary>
        /// Resolves an instance of the default requested type from the container.
        /// </summary>
        /// <param name="serviceType">The <see cref="Type"/> of the object to get from the container.</param>
        /// <returns>The requested object.</returns>
        public object GetService(Type serviceType)
        {
            try
            {
                return this.container.Resolve(serviceType);
            }
            catch (ResolutionFailedException)
            {
                return null;
            }
        }

        /// <summary>
        /// Resolves multiply registered services.
        /// </summary>
        /// <param name="serviceType">The type of the requested services.</param>
        /// <returns>The requested services.</returns>
        public IEnumerable<object> GetServices(Type serviceType)
        {
            try
            {
                return this.container.ResolveAll(serviceType);
            }
            catch (ResolutionFailedException)
            {
                return null;
            }
        }
    }
}
