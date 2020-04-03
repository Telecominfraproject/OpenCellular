// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.InterpolationLibrary
{
    /// <summary>Represents Class ZXY for BivariateInterpolation program. This class cannot be inherited.</summary>
    public sealed class ZXY : F2DArray
    {
        //// (ZXY33,ZXY(2,2)), (ZXY43,ZXY(3,2)), (ZXY34,ZXY(2,3)), (ZXY44,ZXY(3,3))
        //// (P11,ZXY33), (IX,ZXY(1,1)), (IY,ZXY(4,1)), (IXPV,ZXY(1,4)), (IYPV,ZXY(4,4))
        //// (Z42,ZXY(2,1),P22), (Z43,ZXY(1,2),P23), (Z44,ZXY(3,1),P30), (Z45,ZXY(4,2),P31)
        //// (Z53,ZXY(1,3),P32), (Z54,ZXY(4,3),P33), (WX2,ZXY(2,4)), (WX3,ZXY(3,4))

        /// <summary>Initializes a new instance of the <see cref="ZXY" /> class.</summary>
        /// <param name="row">The row.</param>
        /// <param name="columns">The columns.</param>
        public ZXY(int row, int columns)
            : base(row, columns)
        {
        }

        /// <summary>Initializes a new instance of the <see cref="ZXY" /> class.</summary>
        /// <param name="dataArray">The data array.</param>
        public ZXY(float[,] dataArray)
            : base(dataArray)
        {
        }

        /// <summary>Gets or sets the IX.</summary>
        /// <value>The IX.</value>
        public int IX
        {
            ////(IX,ZXY(1,1))
            get
            {
                return (int)this[1, 1];
            }

            set
            {
                this[1, 1] = value;
            }
        }

        /// <summary>Gets or sets the IY.</summary>
        /// <value>The IY.</value>
        public int IY
        {
            ////(IY,ZXY(4,1))
            get
            {
                return (int)this[4, 1];
            }

            set
            {
                this[4, 1] = value;
            }
        }

        /// <summary>Gets or sets the IXPV.</summary>
        /// <value>The IXPV.</value>
        public int IXPV
        {
            ////(IXPV,ZXY(1,4))
            get
            {
                return (int)this[1, 4];
            }

            set
            {
                this[1, 4] = value;
            }
        }

        /// <summary>Gets or sets the IYPV.</summary>
        /// <value>The IYPV.</value>
        public int IYPV
        {
            ////(IYPV,ZXY(4,4))
            get
            {
                return (int)this[4, 4];
            }

            set
            {
                this[4, 4] = value;
            }
        }

        /// <summary>Gets or sets the ZXY33.</summary>
        /// <value>The ZXY33.</value>
        public float ZXY33
        {
            ////(ZXY33,ZXY(2,2))
            get
            {
                return this[2, 2];
            }

            set
            {
                this[2, 2] = value;
            }
        }

        /// <summary>Gets or sets the ZXY43.</summary>
        /// <value>The ZXY43.</value>
        public float ZXY43
        {
            ////(ZXY43,ZXY(3,2))
            get
            {
                return this[3, 2];
            }

            set
            {
                this[3, 2] = value;
            }
        }

        /// <summary>Gets or sets the ZXY34.</summary>
        /// <value>The ZXY34.</value>
        public float ZXY34
        {
            ////(ZXY34,ZXY(2,3))
            get
            {
                return this[2, 3];
            }

            set
            {
                this[2, 3] = value;
            }
        }

        /// <summary>Gets or sets the ZXY44.</summary>
        /// <value>The ZXY44.</value>
        public float ZXY44
        {
            ////(ZXY44,ZXY(3,3))
            get
            {
                return this[3, 3];
            }

            set
            {
                this[3, 3] = value;
            }
        }

        /// <summary>Gets or sets the P11.</summary>
        /// <value>The P11.</value>
        public float P11
        {
            ////(P11,ZXY33)
            get
            {
                return this.ZXY33;
            }

            set
            {
                this.ZXY33 = value;
            }
        }

        #region Row 3

        /// <summary>Gets or sets the Z42.</summary>
        /// <value>The Z42.</value>
        public float Z42
        {
            ////(Z42,ZXY(2,1),P22)
            get
            {
                return this[2, 1];
            }

            set
            {
                this[2, 1] = value;
            }
        }

        /// <summary>Gets or sets the P22.</summary>
        /// <value>The P22.</value>
        public float P22
        {
            ////(Z42,ZXY(2,1),P22)
            get
            {
                return this.Z42;
            }

            set
            {
                this.Z42 = value;
            }
        }

        /// <summary>Gets or sets the Z43.</summary>
        /// <value>The Z43.</value>
        public float Z43
        {
            ////(Z43,ZXY(1,2),P23)
            get
            {
                return this[1, 2];
            }

            set
            {
                this[1, 2] = value;
            }
        }

        /// <summary>Gets or sets the P23.</summary>
        /// <value>The P23.</value>
        public float P23
        {
            ////(Z43,ZXY(1,2),P23)
            get
            {
                return this.Z43;
            }

            set
            {
                this.Z43 = value;
            }
        }

        /// <summary>Gets or sets the Z44.</summary>
        /// <value>The Z44.</value>
        public float Z44
        {
            ////(Z44,ZXY(3,1),P30)
            get
            {
                return this[3, 1];
            }

            set
            {
                this[3, 1] = value;
            }
        }

        /// <summary>Gets or sets the P30.</summary>
        /// <value>The P30.</value>
        public float P30
        {
            ////(Z44,ZXY(3,1),P30)
            get
            {
                return this.Z44;
            }

            set
            {
                this.Z44 = value;
            }
        }

        /// <summary>Gets or sets the Z45.</summary>
        /// <value>The Z45.</value>
        public float Z45
        {
            ////(Z45,ZXY(4,2), P31)
            get
            {
                return this[4, 2];
            }

            set
            {
                this[4, 2] = value;
            }
        }

        /// <summary>Gets or sets the P31.</summary>
        /// <value>The P31.</value>
        public float P31
        {
            ////(Z45,ZXY(4,2),P31)
            get
            {
                return this.Z45;
            }

            set
            {
                this.Z45 = value;
            }
        }

        #endregion

        /// <summary>Gets or sets the Z53.</summary>
        /// <value>The Z53.</value>
        public float Z53
        {
            ////(Z53,ZXY(1,3),P32)
            get
            {
                return this[1, 3];
            }

            set
            {
                this[1, 3] = value;
            }
        }

        /// <summary>Gets or sets the P32.</summary>
        /// <value>The P32.</value>
        public float P32
        {
            ////(Z53,ZXY(1,3),P32)
            get
            {
                return this.Z53;
            }

            set
            {
                this.Z53 = value;
            }
        }

        /// <summary>Gets or sets the Z54.</summary>
        /// <value>The Z54.</value>
        public float Z54
        {
            ////(Z54,ZXY(4,3),P33)
            get
            {
                return this[4, 3];
            }

            set
            {
                this[4, 3] = value;
            }
        }

        /// <summary>Gets or sets the P33.</summary>
        /// <value>The P33.</value>
        public float P33
        {
            ////(Z54,ZXY(4,3),P33)
            get
            {
                return this.Z54;
            }

            set
            {
                this.Z54 = value;
            }
        }

        /// <summary>Gets or sets the WX2.</summary>
        /// <value>The WX2.</value>
        public float WX2
        {
            ////(WX2,ZXY(2,4))
            get
            {
                return this[2, 4];
            }

            set
            {
                this[2, 4] = value;
            }
        }

        /// <summary>Gets or sets the WX3.</summary>
        /// <value>The WX3.</value>
        public float WX3
        {
            ////(WX3,ZXY(3,4))
            get
            {
                return this[3, 4];
            }

            set
            {
                this[3, 4] = value;
            }
        }
    }
}
