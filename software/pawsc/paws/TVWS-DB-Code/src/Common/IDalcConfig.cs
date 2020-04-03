// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents the Configuration interface into the data access layer component.
    /// </summary>
    public interface IDalcConfig
    {
        /// <summary>
        /// Retrieves the current configFieldName value from the config table.
        /// </summary>
        /// <param name="configFieldName">Name of the field to retrieve from config table.</param>
        /// <returns>Returns the config value.</returns>
        string GetValue(string configFieldName);

        /// <summary>
        /// Updates or adds to the configFieldValue to the config table under 
        /// the configFieldName.
        /// </summary>
        /// <param name="configFieldName">Name of the field to set in config table.</param>
        /// <param name="configFieldValue">Value to set in the config table.</param>
        void SetValue(string configFieldName, string configFieldValue);
    }
}
