// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Xml;
    using Microsoft.Practices.Unity.Configuration;

    /// <summary>
    /// A configuration section describing configuration for 
    /// an Microsoft.Practices.Unity.Configuration.IUnityContainer.
    /// </summary>
    /// <remarks>
    /// The UnityContainer expects its XML configuration to be physically present 
    /// in the app.config or web.config file.  Having this constraint on the application's
    /// config file limits the IUnityContainer from being dynamically modified after 
    /// installation.  To get around this limitation, the XmlUnityConfigurationSection
    /// exposes the deserialize method.  
    /// </remarks>
    public class XmlUnityConfigurationSection : UnityConfigurationSection
    {
        /// <summary>
        /// Deserialize from xml reader.
        /// </summary>
        /// <param name="reader">Xml source (can be from any stream).</param>
        public void DeserializeFromXml(XmlReader reader)
        {
            this.DeserializeSection(reader);
        }
    }
}
