// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System.Collections.Generic;

    /// <summary>
    /// Struct IntermediateResults2
    /// </summary>
    public struct IntermediateResults2
    {
        /// <summary>The P1</summary>
        public Dictionary<int, int> P1;

        /// <summary>The PMSE</summary>
        public Dictionary<int, double> PMSE;

        /// <summary>
        /// Initializes a new instance of the <see cref="IntermediateResults2"/> struct.
        /// </summary>
        /// <param name="p1values">The P1 values.</param>
        /// <param name="pmseValues">The PMSE values.</param>
        /// <param name="tableName">Name of the table.</param>
        public IntermediateResults2(int[] p1values, double[] pmseValues, string tableName = null)
            : this()
        {
            if (p1values != null)
            {
                this.AddP1Values(p1values);
            }

            if (pmseValues != null)
            {
                this.AddPmseValues(pmseValues);
            }

            this.TableName = tableName;
        }

        /// <summary>
        /// Gets or sets the name of the table.
        /// </summary>
        /// <value>The name of the table.</value>
        public string TableName { get; set; }

        /// <summary>
        /// Adds the P1 values.
        /// </summary>
        /// <param name="p1values">The P1 values.</param>
        public void AddP1Values(int[] p1values)
        {
            this.P1 = new Dictionary<int, int>();
            for (int i = 21; i <= 60; i++)
            {
                this.P1.Add(i, p1values[i - 21]);
            }
        }

        /// <summary>
        /// Adds the PMSE values.
        /// </summary>
        /// <param name="pmseValues">The PMSE values.</param>
        public void AddPmseValues(double[] pmseValues)
        {
            this.PMSE = new Dictionary<int, double>();
            for (int i = 21; i <= 60; i++)
            {
                this.PMSE.Add(i, pmseValues[i - 21]);
            }
        }
    }
}
