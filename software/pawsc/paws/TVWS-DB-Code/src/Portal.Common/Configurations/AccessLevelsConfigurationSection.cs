// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    public class AccessLevelsConfigurationSection : ConfigurationSection
    {
      [ConfigurationProperty("", IsDefaultCollection = true)]
      public AccessLevelConfigurationElementCollection AccessLevels
      {
          get
          {
              return base[string.Empty] as AccessLevelConfigurationElementCollection;
          }
      }
    }
}
