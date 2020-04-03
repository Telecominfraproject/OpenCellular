// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
using System.Configuration;

    public class AccessLevelConfigurationElementCollection : ConfigurationElementCollection
    {
        public AccessLevelConfigurationElementCollection()
        {
            this.AddElementName = "accessLevel";
        }

        public AceesLevelConfigurationElement this[int index]
        {
            get { return (AceesLevelConfigurationElement)BaseGet(index); }
        }

        public new AceesLevelConfigurationElement this[string name]
        {
            get { return (AceesLevelConfigurationElement)BaseGet(name); }
        }

        protected override ConfigurationElement CreateNewElement()
        {
            return new AceesLevelConfigurationElement();
        }

        protected override object GetElementKey(ConfigurationElement element)
        {
            return ((AceesLevelConfigurationElement)element).Name;
        }
    }
}
