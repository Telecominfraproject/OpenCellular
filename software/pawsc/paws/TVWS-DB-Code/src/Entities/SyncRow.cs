// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents a row that is to be updated from the Protected Region Worker.
    /// </summary>
    public class SyncRow
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SyncRow"/> class.
        /// </summary>
        public SyncRow()
        {
            this.Columns = new Dictionary<string, EntityProperty>();
        }

        /// <summary>
        /// Gets or sets the column values of a table row (dictionary key is the column name within the table).
        /// </summary>
        public Dictionary<string, EntityProperty> Columns { get; set; }

        /// <summary>
        /// Gets or sets the unique system identifier.
        /// </summary>
        /// <value>The unique system identifier.</value>
        public int UniqueSystemIdentifier { get; set; }

        /// <summary>
        /// Gets or sets the partition key.
        /// </summary>
        /// <value>The partition key.</value>
        public string PartitionKey { get; set; }

        /// <summary>
        /// Gets or sets the row key.
        /// </summary>
        /// <value>The row key.</value>
        public string RowKey { get; set; }

        /// <summary>
        /// Gets or sets the ETag.
        /// </summary>
        /// <value>The ETag.</value>
        public string ETag { get; set; }

        /// <summary>
        /// Gets the <see cref="System.String" /> with the specified name.
        /// </summary>
        /// <param name="propName">Name of the property.</param>
        /// <returns>returns System.String.</returns>
        public EntityProperty this[string propName]
        {
            get
            {
                if (this.Columns.Count == 0)
                {
                    return EntityProperty.GeneratePropertyForString(string.Empty);
                }

                return this.Columns[propName];
            }
        }

        /// <summary>
        /// Adds the specified property name.
        /// </summary>
        /// <param name="propName">Name of the property.</param>
        /// <param name="entityProperty">The entity property.</param>
        public void Add(string propName, EntityProperty entityProperty)
        {
            this.Columns.Add(propName, entityProperty);
        }

        /// <summary>
        /// Adds the specified entity property.
        /// </summary>
        /// <param name="entityProperty">The entity property.</param>
        public void Add(KeyValuePair<string, EntityProperty> entityProperty)
        {
            this.Columns.Add(entityProperty.Key, entityProperty.Value);
        }
    }
}
