// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.InterpolationLibrary
{
    /// <summary>
    /// Represents Class ZX for ITPLBV program. This class cannot be inherited.
    /// </summary>
    public sealed class ZX : F2DArray
    {
        //// (ZX33,ZX(2,2)), (ZX43,ZX(3,2)), (ZX34,ZX(2,3)), (ZX44,ZX(3,3))
        //// (P10,ZX33), (LX0,ZX(1,1)), (LXM1,ZX(4,1)), (LXM2,ZX(1,4)), (LXP1,ZX(4,4))
        //// (A2, ZX(1,2), B, Q1), (A4, ZX(4,2), C, Q2)
        //// (A1, A5, B1, B5, ZX(2,1), A, Q0)
        //// (X2,ZX(3,1),A3SQ), (X4,ZX(1,3)), (Y2,ZX(2,4)), (X5,ZX(4,3)), (Y5,ZX(3,4),P02)

        /// <summary>
        /// Initializes a new instance of the <see cref="ZX" /> class.
        /// </summary>
        /// <param name="row">The row.</param>
        /// <param name="columns">The columns.</param>
        public ZX(int row, int columns)
            : base(row, columns)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ZX" /> class.
        /// </summary>
        /// <param name="dataArray">The data array.</param>
        public ZX(float[,] dataArray)
            : base(dataArray)
        {
        }

        #region Row 1
        /// <summary>
        /// Gets or sets the ZX33.
        /// </summary>
        /// <value>The ZX33.</value>
        public float ZX33
        {
            ////(ZX33,ZX(2,2))
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
        /// Gets or sets the ZX43.
        /// </summary>
        /// <value>The ZX43.</value>
        public float ZX43
        {
            ////(ZX43,ZX(3,2))
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
        /// Gets or sets the ZX34.
        /// </summary>
        /// <value>The ZX34.</value>
        public float ZX34
        {
            ////(ZX34,ZX(2,3))
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
        /// Gets or sets the ZX44.
        /// </summary>
        /// <value>The ZX44.</value>
        public float ZX44
        {
            ////(ZX44,ZX(3,3))
            get
            {
                return this[3, 3];
            }

            set
            {
                this[3, 3] = value;
            }
        }
        #endregion

        #region Row 2
        /// <summary>
        /// Gets or sets the LX0.
        /// </summary>
        /// <value>The LX0.</value>
        public int LX0
        {
            ////(LX0,ZX(1,1))
            get
            {
                return (int)this[1, 1];
            }

            set
            {
                this[1, 1] = value;
            }
        }

        /// <summary>
        /// Gets or sets the LXM1.
        /// </summary>
        /// <value>The LXM1.</value>
        public int LXM1
        {
            ////(LXM1,ZX(4,1))
            get
            {
                return (int)this[4, 1];
            }

            set
            {
                this[4, 1] = value;
            }
        }

        /// <summary>
        /// Gets or sets the LXM2.
        /// </summary>
        /// <value>The LXM2.</value>
        public int LXM2
        {
            ////(LXM2,ZX(1,4))
            get
            {
                return (int)this[1, 4];
            }

            set
            {
                this[1, 4] = value;
            }
        }

        /// <summary>
        /// Gets or sets the LXP1.
        /// </summary>
        /// <value>The LXP1.</value>
        public int LXP1
        {
            ////(LXP1,ZX(4,4))
            get
            {
                return (int)this[4, 4];
            }

            set
            {
                this[4, 4] = value;
            }
        }

        /// <summary>
        /// Gets or sets the P10.
        /// </summary>
        /// <value>The P10.</value>
        public float P10
        {
            ////(P10,ZX33)
            get
            {
                return this.ZX33;
            }

            set
            {
                this.ZX33 = value;
            }
        }
        #endregion

        #region Row 3
        /// <summary>
        /// Gets or sets the Q2.
        /// </summary>
        /// <value>The Q2.</value>
        public float Q2
        {
            ////(A4,ZX(4,2),C,Q2)
            get
            {
                return this.A4;
            }

            set
            {
                this.A4 = value;
            }
        }

        /// <summary>
        /// Gets or sets the C.
        /// </summary>
        /// <value>The C.</value>
        public float C
        {
            ////(A4,ZX(4,2),C,Q2)
            get
            {
                return this.A4;
            }

            set
            {
                this.A4 = value;
            }
        }

        /// <summary>
        /// Gets or sets the A4.
        /// </summary>
        /// <value>The A4.</value>
        public float A4
        {
            ////(A4,ZX(4,2),C,Q2)
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
        /// Gets or sets the Q1.
        /// </summary>
        /// <value>The Q1.</value>
        public float Q1
        {
            ////(A2,ZX(1,2),B,Q1)
            get
            {
                return this.A2;
            }

            set
            {
                this.A2 = value;
            }
        }

        /// <summary>
        /// Gets or sets the B.
        /// </summary>
        /// <value>The B.</value>
        public float B
        {
            ////(A2,ZX(1,2),B,Q1)
            get
            {
                return this.A2;
            }

            set
            {
                this.A2 = value;
            }
        }

        /// <summary>
        /// Gets or sets the A2.
        /// </summary>
        /// <value>The A2.</value>
        public float A2
        {
            ////(A2,ZX(1,2),B,Q1)
            get
            {
                return this[1, 2];
            }

            set
            {
                this[1, 2] = value;
            }
        }
        #endregion

        #region Row 4
        /// <summary>
        /// Gets or sets the A1.
        /// </summary>
        /// <value>The A1.</value>
        public float A1
        {
            ////(A1, A5, B1, B5, ZX(2,1), A, Q0)
            get
            {
                return this.A;
            }

            set
            {
                this.A = value;
            }
        }

        /// <summary>
        /// Gets or sets the A5.
        /// </summary>
        /// <value>The A5.</value>
        public float A5
        {
            ////(A1, A5, B1, B5, ZX(2,1), A, Q0)
            get
            {
                return this.A;
            }

            set
            {
                this.A = value;
            }
        }

        /// <summary>
        /// Gets or sets the B1.
        /// </summary>
        /// <value>The B1.</value>
        public float B1
        {
            ////(A1, A5, B1, B5, ZX(2,1), A, Q0)
            get
            {
                return this.A;
            }

            set
            {
                this.A = value;
            }
        }

        /// <summary>
        /// Gets or sets the B5.
        /// </summary>
        /// <value>The B5.</value>
        public float B5
        {
            ////(A1, A5, B1, B5, ZX(2,1), A, Q0)
            get
            {
                return this.A;
            }

            set
            {
                this.A = value;
            }
        }

        /// <summary>
        /// Gets or sets A.
        /// </summary>
        /// <value>returns A.</value>
        public float A
        {
            ////(A1, A5, B1, B5, ZX(2,1), A, Q0)
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
        /// Gets or sets the Q0.
        /// </summary>
        /// <value>The Q0.</value>
        public float Q0
        {
            ////(A1, A5, B1, B5, ZX(2,1), A, Q0)
            get
            {
                return this.A;
            }

            set
            {
                this.A = value;
            }
        }
        #endregion

        #region Row 5
        /// <summary>
        /// Gets or sets the X2.
        /// </summary>
        /// <value>The X2.</value>
        public float X2
        {
            //// (X2,ZX(3,1),A3SQ)
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
        /// Gets or sets the A3SQ.
        /// </summary>
        /// <value>The A3SQ.</value>
        public float A3SQ
        {
            //// (X2,ZX(3,1),A3SQ)
            get
            {
                return this.X2;
            }

            set
            {
                this.X2 = value;
            }
        }

        /// <summary>
        /// Gets or sets the X4.
        /// </summary>
        /// <value>The X4.</value>
        public float X4
        {
            //// (X4,ZX(1,3))
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
        /// Gets or sets the Y2.
        /// </summary>
        /// <value>The Y2.</value>
        public float Y2
        {
            //// (Y2,ZX(2,4))
            get
            {
                return this[2, 4];
            }

            set
            {
                this[2, 4] = value;
            }
        }

        /// <summary>
        /// Gets or sets the X5.
        /// </summary>
        /// <value>The X5.</value>
        public float X5
        {
            //// (Y2,ZX(2,4))
            get
            {
                return this[4, 3];
            }

            set
            {
                this[4, 3] = value;
            }
        }

        /// <summary>
        /// Gets or sets the Y5.
        /// </summary>
        /// <value>The Y5.</value>
        public float Y5
        {
            //// (Y5,ZX(3,4),P02)
            get
            {
                return this[3, 4];
            }

            set
            {
                this[3, 4] = value;
            }
        }

        /// <summary>
        /// Gets or sets the P02.
        /// </summary>
        /// <value>The P02.</value>
        public float P02
        {
            //// (Y5,ZX(3,4),P02)
            get
            {
                return this[3, 4];
            }

            set
            {
                this[3, 4] = value;
            }
        } 
        #endregion
    }
}
