// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.InterpolationLibrary
{
    /// <summary>Represents Class ZB for BivariateInterpolation program. This class cannot be inherited.</summary>
    public sealed class ZB : F2DArray
    {
        /// <summary>Initializes a new instance of the <see cref="ZB" /> class.</summary>
        /// <param name="row">The row.</param>
        /// <param name="columns">The columns.</param>
        public ZB(int row, int columns)
            : base(row, columns)
        {
        }

        /// <summary>Initializes a new instance of the <see cref="ZB" /> class.</summary>
        /// <param name="dataArray">The data array.</param>
        public ZB(float[,] dataArray)
            : base(dataArray)
        {
        }

        /// <summary>Gets or sets the Z3B1.</summary>
        /// <value>The Z3B1.</value>
        public float Z3B1
        {
            ////(Z3B1,ZB(1,1))
            get
            {
                return this[1, 1];
            }

            set
            {
                this[1, 1] = value;
            }
        }

        /// <summary>Gets or sets the Z3B2.</summary>
        /// <value>The Z3B2.</value>
        public float Z3B2
        {
            ////(Z3B2,ZB(1,2))
            get
            {
                return this[1, 2];
            }

            set
            {
                this[1, 2] = value;
            }
        }

        /// <summary>Gets or sets the Z3B3.</summary>
        /// <value>The Z3B3.</value>
        public float Z3B3
        {
            ////(Z3B3,ZB(1,3))
            get
            {
                return this[1, 3];
            }

            set
            {
                this[1, 3] = value;
            }
        }

        /// <summary>Gets or sets the Z3B4.</summary>
        /// <value>The Z3B4.</value>
        public float Z3B4
        {
            ////(Z3B4,ZB(1,4))
            get
            {
                return this[1, 4];
            }

            set
            {
                this[1, 4] = value;
            }
        }

        /// <summary>Gets or sets the Z3B5.</summary>
        /// <value>The Z3B5.</value>
        public float Z3B5
        {
            ////(Z3B5,ZB(1,5))
            get
            {
                return this[1, 5];
            }

            set
            {
                this[1, 5] = value;
            }
        }

        /// <summary>Gets or sets the Z4B1.</summary>
        /// <value>The Z4B1.</value>
        public float Z4B1
        {
            ////(Z4B1,ZB(2,1))
            get
            {
                return this[2, 1];
            }

            set
            {
                this[2, 1] = value;
            }
        }

        /// <summary>Gets or sets the Z4B2.</summary>
        /// <value>The Z4B2.</value>
        public float Z4B2
        {
            ////(Z4B2,ZB(2,2))
            get
            {
                return this[2, 2];
            }

            set
            {
                this[2, 2] = value;
            }
        }

        /// <summary>Gets or sets the Z4B3.</summary>
        /// <value>The Z4B3.</value>
        public float Z4B3
        {
            ////(Z4B3,ZB(2,3))
            get
            {
                return this[2, 3];
            }

            set
            {
                this[2, 3] = value;
            }
        }

        /// <summary>Gets or sets the Z4B4.</summary>
        /// <value>The Z4B4.</value>
        public float Z4B4
        {
            ////(Z4B4,ZB(2,4))
            get
            {
                return this[2, 4];
            }

            set
            {
                this[2, 4] = value;
            }
        }

        /// <summary>Gets or sets the Z4B5.</summary>
        /// <value>The Z4B5.</value>
        public float Z4B5
        {
            ////(Z4B5,ZB(2,5))
            get
            {
                return this[2, 5];
            }

            set
            {
                this[2, 5] = value;
            }
        }
    }
}
