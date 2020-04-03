// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System.Configuration;

    public class AvailableRegionsConfigurationSection : ConfigurationSection
    {
        [ConfigurationProperty("", IsDefaultCollection = true)]
        public RegionsConfigurationElementCollection AvailableRegions
        {
            get
            {
                return base[string.Empty] as RegionsConfigurationElementCollection;
            }
        }
    }
}
