// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    /// <summary>Represents Class FortranDoubleArray for fortran. Indexing starts with 1 instead of 0</summary>
    public class FortranDoubleArray
    {
        /// <summary>The internal array</summary>
        private double[] internalArray;

        /// <summary>
        /// Initializes a new instance of the <see cref="FortranDoubleArray"/> class.
        /// </summary>
        /// <param name="arraySize">Size of the array.</param>
        public FortranDoubleArray(int arraySize)
        {
            this.internalArray = new double[arraySize + 1];
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FortranDoubleArray"/> class.
        /// </summary>
        /// <param name="dataArray">The data array.</param>
        public FortranDoubleArray(double[] dataArray)
        {
            this.internalArray = new double[dataArray.Length + 1];
            this.SetArray(dataArray);
        }

        /// <summary>
        /// Gets the length.
        /// </summary>
        /// <value>The length.</value>
        public int Length
        {
            get
            {
                return this.internalArray.Length;
            }
        }

        /// <summary>Gets or sets the <see cref="System.float" /> at the specified index.</summary>
        /// <param name="index">The index.</param>
        /// <returns>returns data at index.</returns>
        public double this[float index]
        {
            get
            {
                return this.internalArray[(int)index];
            }

            set
            {
                this.internalArray[(int)index] = value;
            }
        }

        /// <summary>Sets the array.</summary>
        /// <param name="dataArray">The data array.</param>
        private void SetArray(double[] dataArray)
        {
            for (int i = 0; i < dataArray.Length; i++)
            {
                this.internalArray[i + 1] = dataArray[i];
            }
        }
    }
}
