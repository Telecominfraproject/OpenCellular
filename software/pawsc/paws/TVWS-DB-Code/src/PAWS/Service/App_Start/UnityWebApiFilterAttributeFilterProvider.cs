// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.PAWS.Service.App_Start
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Web.Http;
    using System.Web.Http.Controllers;
    using System.Web.Http.Filters;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common.Utilities;

    public class UnityWebApiFilterAttributeFilterProvider : ActionDescriptorFilterProvider, IFilterProvider
    {
        private IUnityContainer container;

        public UnityWebApiFilterAttributeFilterProvider(IUnityContainer container)
        {
            if (container == null)
            {
                throw new ArgumentNullException("container");
            }

            this.container = container;
        }

        public new IEnumerable<FilterInfo> GetFilters(HttpConfiguration configuration, HttpActionDescriptor actionDescriptor)
        {
            IEnumerable<IActionFilter> actionFilters = this.container.ResolveAll<IActionFilter>();

            foreach (IActionFilter actionFilter in actionFilters)
            {
                yield return new FilterInfo(actionFilter, FilterScope.Action);
            }
        }
    }
}
