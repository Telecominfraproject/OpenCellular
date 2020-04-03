// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common.Configurations
{
    using System.Configuration;

    public class FeaturesConfigurationElementCollection : ConfigurationElementCollection
    {
        public FeaturesConfigurationElementCollection()
        {
            this.AddElementName = "feature";
        }

        public FeatureConfigurationElement this[int index]
        {
            get { return (FeatureConfigurationElement)BaseGet(index); }
        }

        public new FeatureConfigurationElement this[string name]
        {
            get { return (FeatureConfigurationElement)BaseGet(name); }
        }

        protected override ConfigurationElement CreateNewElement()
        {
            return new FeatureConfigurationElement();
        }

        protected override object GetElementKey(ConfigurationElement element)
        {
            return ((FeatureConfigurationElement)element).FriendlyName;
        }
    }
}
