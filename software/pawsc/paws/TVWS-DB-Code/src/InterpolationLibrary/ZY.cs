// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.InterpolationLibrary
{
    /// <summary>
    /// Represents Class ZY. This class cannot be inherited.
    /// </summary>
    public sealed class ZY : F2DArray
    {
        //// (ZY33,ZY(2,2)), (ZY43,ZY(3,2)), (ZY34,ZY(2,3)), (ZY44,ZY(3,3)), (P01,ZY33)
        //// (LY0,ZY(1,1)), (LYM1,ZY(4,1)), (LYM2,ZY(1,4)), (LYP1,ZY(4,4)), (B2,ZY(2,1),D,Q3)
        //// (Y4,ZY(3,1),B3SQ), (B4,ZY(2,4),E), (Z23,ZY(1,2),P03),
        //// (Z24,ZY(4,2),P12), (Z32,ZY(1,3),P13), (Z34,ZY(4,3),P20), (Z35,ZY(3,4),P21)
       
        /// <summary>
        /// Initializes a new instance of the <see cref="ZY" /> class.
        /// </summary>
        /// <param name="row">The row.</param>
        /// <param name="columns">The columns.</param>
        public ZY(int row, int columns)
            : base(row, columns)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ZY" /> class.
        /// </summary>
        /// <param name="dataArray">The data array.</param>
        public ZY(float[,] dataArray)
            : base(dataArray)
        {
        }

        #region Row 1
        /// <summary>
        /// Gets or sets the ZY33.
        /// </summary>
        /// <value>The ZY33.</value>
        public float ZY33
        {
            ////(ZY33,ZY(2,2))
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
        /// Gets or sets the ZY43.
        /// </summary>
        /// <value>The ZY43.</value>
        public float ZY43
        {
            ////(ZY43,ZY(3,2))
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
        /// Gets or sets the ZY34.
        /// </summary>
        /// <value>The ZY34.</value>
        public float ZY34
        {
            ////(ZY34,ZY(2,3))
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
        /// Gets or sets the ZY44.
        /// </summary>
        /// <value>The ZY44.</value>
        public float ZY44
        {
            ////(ZY44,ZY(3,3))
            get
            {
                return this[3, 3];
            }

            set
            {
                this[3, 3] = value;
            }
        }

        /// <summary>
        /// Gets or sets the P01.
        /// </summary>
        /// <value>The P01.</value>
        public float P01
        {
            ////(P01,ZY33)
            get
            {
                return this.ZY33;
            }

            set
            {
                this.ZY33 = value;
            }
        }
        #endregion

        #region Row 2
        /// <summary>
        /// Gets or sets the D.
        /// </summary>
        /// <value>The D.</value>
        public float D
        {
            ////(B2,ZY(2,1),D,Q3)
            get
            {
                return this.B2;
            }

            set
            {
                this.B2 = value;
            }
        }

        /// <summary>
        /// Gets or sets the Q3.
        /// </summary>
        /// <value>The Q3.</value>
        public float Q3
        {
            ////(B2,ZY(2,1),D,Q3)
            get
            {
                return this.B2;
            }

            set
            {
                this.B2 = value;
            }
        }

        /// <summary>
        /// Gets or sets the B2.
        /// </summary>
        /// <value>The B2.</value>
        public float B2
        {
            ////(B2,ZY(2,1),D,Q3)
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
        /// Gets or sets the LY0.
        /// </summary>
        /// <value>The LY0.</value>
        public int LY0
        {
            ////(LY0,ZY(1,1))
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
        /// Gets or sets the LYM1.
        /// </summary>
        /// <value>The LYM1.</value>
        public int LYM1
        {
            ////(LYM1,ZY(4,1))
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
        /// Gets or sets the LYM2.
        /// </summary>
        /// <value>The LYM2.</value>
        public int LYM2
        {
            ////(LYM2,ZY(1,4))
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
        /// Gets or sets the LYP1.
        /// </summary>
        /// <value>The LYP1.</value>
        public int LYP1
        {
            ////(LYP1,ZY(4,4))
            get
            {
                return (int)this[4, 4];
            }

            set
            {
                this[4, 4] = value;
            }
        }
        #endregion

        #region Row 3
        /// <summary>
        /// Gets or sets the Y4.
        /// </summary>
        /// <value>The Y4.</value>
        public float Y4
        {
            ////(Y4,ZY(3,1),B3SQ)
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
        /// Gets or sets the B3SQ.
        /// </summary>
        /// <value>The B3SQ.</value>
        public float B3SQ
        {
            ////(Y4,ZY(3,1),B3SQ)
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
        /// Gets or sets the B4.
        /// </summary>
        /// <value>The B4.</value>
        public float B4
        {
            ////(B4,ZY(2,4),E)
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
        /// Gets or sets the E.
        /// </summary>
        /// <value>The E.</value>
        public float E
        {
            ////(B4,ZY(2,4),E)
            get
            {
                return this.B4;
            }

            set
            {
                this.B4 = value;
            }
        }

        /// <summary>
        /// Gets or sets the Z23.
        /// </summary>
        /// <value>The Z23.</value>
        public float Z23
        {
            ////(Z23,ZY(1,2),P03)
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
        /// Gets or sets the P03.
        /// </summary>
        /// <value>The P03.</value>
        public float P03
        {
            ////(Z23,ZY(1,2),P03)
            get
            {
                return this.Z23;
            }

            set
            {
                this.Z23 = value;
            }
        } 
        #endregion

        #region Row 4
        /// <summary>
        /// Gets or sets the Z24.
        /// </summary>
        /// <value>The Z24.</value>
        public float Z24
        {
            ////(Z24,ZY(4,2),P12)
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
        /// Gets or sets the P12.
        /// </summary>
        /// <value>The P12.</value>
        public float P12
        {
            ////(Z24,ZY(4,2),P12)
            get
            {
                return this.Z24;
            }

            set
            {
                this.Z24 = value;
            }
        }

        /// <summary>
        /// Gets or sets the Z32.
        /// </summary>
        /// <value>The Z32.</value>
        public float Z32
        {
            ////(Z32,ZY(1,3),P13)
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
        /// Gets or sets the P13.
        /// </summary>
        /// <value>The P13.</value>
        public float P13
        {
            ////(Z32,ZY(1,3),P13)
            get
            {
                return this.Z32;
            }

            set
            {
                this.Z32 = value;
            }
        }

        /// <summary>
        /// Gets or sets the Z34.
        /// </summary>
        /// <value>The Z34.</value>
        public float Z34
        {
            ////(Z34,ZY(4,3),P20)
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
        /// Gets or sets the P20.
        /// </summary>
        /// <value>The P20.</value>
        public float P20
        {
            ////(Z34,ZY(4,3),P20)
            get
            {
                return this.Z34;
            }

            set
            {
                this.Z34 = value;
            }
        }

        /// <summary>
        /// Gets or sets the Z35.
        /// </summary>
        /// <value>The Z35.</value>
        public float Z35
        {
            ////(Z35,ZY(3,4),P21)
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
        /// Gets or sets the P21.
        /// </summary>
        /// <value>The P21.</value>
        public float P21
        {
            ////(Z35,ZY(3,4),P21)
            get
            {
                return this.Z35;
            }

            set
            {
                this.Z35 = value;
            }
        }  
        #endregion
    }
}
