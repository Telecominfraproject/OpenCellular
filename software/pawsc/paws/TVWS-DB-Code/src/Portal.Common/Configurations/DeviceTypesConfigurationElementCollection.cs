// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common.Configurations
{
    using System.Configuration;

    public class DeviceTypesConfigurationElementCollection : ConfigurationElementCollection
    {
        public DeviceTypesConfigurationElementCollection()
        {
            this.AddElementName = "device";
        }

        public DeviceTypeConfigurationElement this[int index]
        {
            get { return (DeviceTypeConfigurationElement)BaseGet(index); }
        }

        public new DeviceTypeConfigurationElement this[string name]
        {
            get { return (DeviceTypeConfigurationElement)BaseGet(name); }
        }

        protected override ConfigurationElement CreateNewElement()
        {
            return new DeviceTypeConfigurationElement();
        }

        protected override object GetElementKey(ConfigurationElement element)
        {
            return ((DeviceTypeConfigurationElement)element).FriendlyName;
        }
    }
}
