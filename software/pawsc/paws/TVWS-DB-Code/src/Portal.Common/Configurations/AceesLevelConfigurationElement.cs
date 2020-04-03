// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System.Configuration;

    public class AceesLevelConfigurationElement : ConfigurationElement
    {
        [ConfigurationProperty("name", IsRequired = true, IsKey = true)]
        public string Name
        {
            get { return (string)this["name"]; }
        }

        [ConfigurationProperty("id", IsRequired = true)]
        public int Id
        {
            get { return (int)this["id"]; }
        }

        [ConfigurationProperty("friendlyName", IsRequired = false)]
        public string FriendlyName
        {
            get
            {
                if (string.IsNullOrEmpty((string)this["friendlyName"]))
                {
                    return (string)this["name"];
                }

                return (string)this["friendlyName"];
            }
        }

        [ConfigurationProperty("applicableRegulatories", IsRequired = true)]
        public string ApplicableRegulatories
        {
            get { return (string)this["applicableRegulatories"]; }           
        }        
    }
}
