// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.InterpolationLibrary
{
    /// <summary>Represents Class F2DArray (2 Dimensional) for fortran. Indexing starts with 1 instead of 0</summary>
    public class F2DArray
    {
        /// <summary>The internal array</summary>
        private float[,] internalArray;

        /// <summary>Initializes a new instance of the <see cref="F2DArray" /> class.</summary>
        /// <param name="row">The row.</param>
        /// <param name="columns">The columns.</param>
        public F2DArray(int row, int columns)
        {
            this.Rows = row + 1;
            this.Columns = columns + 1;
            this.internalArray = new float[this.Rows, this.Columns];
        }

        /// <summary>Initializes a new instance of the <see cref="F2DArray" /> class.</summary>
        /// <param name="dataArray">The data array.</param>
        public F2DArray(float[,] dataArray)
        {
            this.Rows = dataArray.GetLength(0) + 1;
            this.Columns = dataArray.GetLength(1) + 1;
            this.internalArray = new float[this.Rows, this.Columns];
            this.SetArray(dataArray);
        }

        /// <summary>Gets or sets the rows.</summary>
        /// <value>The rows.</value>
        public int Rows { get; set; }

        /// <summary>Gets or sets the columns.</summary>
        /// <value>The columns.</value>
        public int Columns { get; set; }

        /// <summary>Gets or sets the <see cref="System.decimal" /> with the specified row.</summary>
        /// <param name="row">The row.</param>
        /// <param name="column">The column.</param>
        /// <returns>returns System.decimal.</returns>
        public virtual float this[float row, float column]
        {
            get
            {
                return this.internalArray[(int)row, (int)column];
            }

            set
            {
                this.internalArray[(int)row, (int)column] = value;
            }
        }

        /// <summary>Sets the array.</summary>
        /// <param name="dataArray">The data array.</param>
        private void SetArray(float[,] dataArray)
        {
            for (int rows = 1; rows < this.Rows; rows++)
            {
                for (int columns = 1; columns < this.Columns; columns++)
                {
                    this.internalArray[rows, columns] = dataArray[rows - 1, columns - 1];
                }
            }
        }
    }
}
