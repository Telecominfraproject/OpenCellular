// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.InterpolationLibrary
{
    /// <summary>Represents Class FArray for fortran. Indexing starts with 1 instead of 0</summary>
    public class FArray
    {
        /// <summary>The internal array</summary>
        private float[] internalArray;

        /// <summary>Initializes a new instance of the <see cref="FArray" /> class.</summary>
        /// <param name="arraySize">Size of the array.</param>
        public FArray(int arraySize)
        {
            this.internalArray = new float[arraySize + 1];
        }

        /// <summary>Initializes a new instance of the <see cref="FArray" /> class.</summary>
        /// <param name="dataArray">The data array.</param>
        public FArray(float[] dataArray)
        {
            this.internalArray = new float[dataArray.Length + 1];
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
        public float this[float index]
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
        private void SetArray(float[] dataArray)
        {
            for (int i = 0; i < dataArray.Length; i++)
            {
                this.internalArray[i + 1] = dataArray[i];
            }
        }
    }
}
