// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Entities.Versitcard;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents a Settings class.
    /// </summary>
    public class Settings : TableEntity
    {
        /// <summary>
        /// Gets or sets the Registrant.
        /// </summary>
        public string ConfigValue { get; set; }
    }
}
