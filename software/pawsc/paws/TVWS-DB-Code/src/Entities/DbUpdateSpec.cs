// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using Newtonsoft.Json;

    /// <summary>
    /// Class for Paws database update spec.
    /// </summary>
    [JsonConverter(typeof(DbUpdateSpecConverter))]
    public class DbUpdateSpec
    {
        /// <summary>
        /// Gets or sets the databases.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameDatabases)]
        public DatabaseSpec[] Databases { get; set; }
    }
}
