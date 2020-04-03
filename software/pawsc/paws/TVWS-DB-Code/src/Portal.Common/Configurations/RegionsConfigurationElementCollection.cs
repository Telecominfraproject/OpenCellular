// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System.Configuration;

    public class RegionsConfigurationElementCollection : ConfigurationElementCollection
    {
        public RegionsConfigurationElementCollection()
        {
            this.AddElementName = "region";
        }

        public RegionConfigurationElement this[int index]
        {
            get { return (RegionConfigurationElement)BaseGet(index); }
        }

        public new RegionConfigurationElement this[string name]
        {
            get { return (RegionConfigurationElement)BaseGet(name); }
        }

        protected override ConfigurationElement CreateNewElement()
        {
            return new RegionConfigurationElement();
        }

        protected override object GetElementKey(ConfigurationElement element)
        {
            return ((RegionConfigurationElement)element).Name;
        }
    }
}
