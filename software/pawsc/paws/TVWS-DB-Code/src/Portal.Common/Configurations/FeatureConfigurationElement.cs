// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common.Configurations
{
    using System.Configuration;

    public class FeatureConfigurationElement : ConfigurationElement
    {
        [ConfigurationProperty("name", IsRequired = true, IsKey = true)]
        public string FriendlyName
        {
            get { return (string)this["name"]; }
        }
    }
}
