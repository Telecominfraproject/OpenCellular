// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.InterpolationLibrary
{
    /// <summary>
    /// Represents Class ZAB. This class cannot be inherited.
    /// </summary>
    public sealed class ZAB : F2DArray
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ZAB" /> class.
        /// </summary>
        /// <param name="row">The row.</param>
        /// <param name="columns">The columns.</param>
        public ZAB(int row, int columns)
            : base(row, columns)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ZAB" /> class.
        /// </summary>
        /// <param name="dataArray">The data array.</param>
        public ZAB(float[,] dataArray)
            : base(dataArray)
        {
        }

        /// <summary>
        /// Gets or sets the ZA2B2.
        /// </summary>
        /// <value>The ZA2B2.</value>
        public float ZA2B2
        {
            ////(ZA2B2,ZAB(1,1))
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
        /// Gets or sets the ZA2B3.
        /// </summary>
        /// <value>The ZA2B3.</value>
        public float ZA2B3
        {
            //// (ZA2B3,ZAB(1,2))
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
        /// Gets or sets the ZA2B4.
        /// </summary>
        /// <value>The ZA2B4.</value>
        public float ZA2B4
        {
            ////(ZA2B4,ZAB(1,3))
            get
            {
                return this[1, 3];
            }

            set
            {
                this[1, 3] = value;
            }
        }

        /// <summary>
        /// Gets or sets the ZA3B2.
        /// </summary>
        /// <value>The ZA3B2.</value>
        public float ZA3B2
        {
            ////(ZA3B2,ZAB(2,1))
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
        /// Gets or sets the ZA3B3.
        /// </summary>
        /// <value>The ZA3B3.</value>
        public float ZA3B3
        {
            ////(ZA3B3,ZAB(2,2))
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
        /// Gets or sets the ZA3B4.
        /// </summary>
        /// <value>The ZA3B4.</value>
        public float ZA3B4
        {
            ////(ZA3B4,ZAB(2,3))
            get
            {
                return this[2, 3];
            }

            set
            {
                this[2, 3] = value;
            }
        }

        /// <summary>
        /// Gets or sets the ZA4B2.
        /// </summary>
        /// <value>The ZA4B2.</value>
        public float ZA4B2
        {
            ////(ZA4B2,ZAB(3,1))
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
        /// Gets or sets the ZA4B3.
        /// </summary>
        /// <value>The ZA4B3.</value>
        public float ZA4B3
        {
            ////(ZA4B3,ZAB(3,2))
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
        /// Gets or sets the ZA4B4.
        /// </summary>
        /// <value>The ZA4B4.</value>
        public float ZA4B4
        {
            ////(ZA4B4,ZAB(3,3))
            get
            {
                return this[3, 3];
            }

            set
            {
                this[3, 3] = value;
            }
        }
    }
}
