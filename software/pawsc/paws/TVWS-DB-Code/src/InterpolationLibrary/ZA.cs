// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.InterpolationLibrary
{
    /// <summary>
    /// Represents Class ZA for BivariateInterpolation program. This class cannot be inherited.
    /// </summary>
    public sealed class ZA : F2DArray
    {
        //// (Z3A1,ZA(1,1)), (Z3A2,ZA(2,1)), (Z3A3,ZA(3,1)),
        //// (Z3A4,ZA(4,1)), (Z3A5,ZA(5,1)), (Z4A1,ZA(1,2)), (Z4A2,ZA(2,2)), 
        //// (Z4A3,ZA(3,2)), (Z4A4,ZA(4,2)), (Z4A5,ZA(5,2)),

        /// <summary>
        /// Initializes a new instance of the <see cref="ZA" /> class.
        /// </summary>
        /// <param name="row">The row.</param>
        /// <param name="columns">The columns.</param>
        public ZA(int row, int columns)
            : base(row, columns)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ZA" /> class.
        /// </summary>
        /// <param name="dataArray">The data array.</param>
        public ZA(float[,] dataArray)
            : base(dataArray)
        {
        }

        /// <summary>
        /// Gets or sets the Z3A1.
        /// </summary>
        /// <value>The Z3A1.</value>
        public float Z3A1
        {
            //// (Z3A1,ZA(1,1))
            get
            {
                return this[1, 1];
            }

            set
            {
                this[1, 1] = value;
            }
        }

        /// <summary>
        /// Gets or sets the Z3A2.
        /// </summary>
        /// <value>The Z3A2.</value>
        public float Z3A2
        {
            get
            {
                return this[2, 1];
            }

            set
            {
                this[2, 1] = value;
            }
        }

        /// <summary>
        /// Gets or sets the Z3A3.
        /// </summary>
        /// <value>The Z3A3.</value>
        public float Z3A3
        {
            get
            {
                return this[3, 1];
            }

            set
            {
                this[3, 1] = value;
            }
        }

        /// <summary>
        /// Gets or sets the Z3A4.
        /// </summary>
        /// <value>The Z3A4.</value>
        public float Z3A4
        {
            get
            {
                return this[4, 1];
            }

            set
            {
                this[4, 1] = value;
            }
        }

        /// <summary>
        /// Gets or sets the Z3A5.
        /// </summary>
        /// <value>The Z3A5.</value>
        public float Z3A5
        {
            get
            {
                return this[5, 1];
            }

            set
            {
                this[5, 1] = value;
            }
        }

        /// <summary>
        /// Gets or sets the Z4A1.
        /// </summary>
        /// <value>The Z4A1.</value>
        public float Z4A1
        {
            get
            {
                return this[1, 2];
            }

            set
            {
                this[1, 2] = value;
            }
        }

        /// <summary>
        /// Gets or sets the Z4A2.
        /// </summary>
        /// <value>The Z4A2.</value>
        public float Z4A2
        {
            get
            {
                return this[2, 2];
            }

            set
            {
                this[2, 2] = value;
            }
        }

        /// <summary>
        /// Gets or sets the Z4A3.
        /// </summary>
        /// <value>The Z4A3.</value>
        public float Z4A3
        {
            get
            {
                return this[3, 2];
            }

            set
            {
                this[3, 2] = value;
            }
        }

        /// <summary>
        /// Gets or sets the Z4A4.
        /// </summary>
        /// <value>The Z4A4.</value>
        public float Z4A4
        {
            get
            {
                return this[4, 2];
            }

            set
            {
                this[4, 2] = value;
            }
        }

        /// <summary>
        /// Gets or sets the Z4A5.
        /// </summary>
        /// <value>The Z4A5.</value>
        public float Z4A5
        {
            get
            {
                return this[5, 2];
            }

            set
            {
                this[5, 2] = value;
            }
        }
    }
}
