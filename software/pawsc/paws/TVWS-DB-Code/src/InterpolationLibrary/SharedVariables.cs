// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.InterpolationLibrary
{
    //// 1. (P00,Z33)
    //// 2. (IMN,JX)
    //// 3. (IMX,JY)
    //// 4. (JXM2,JX1)
    //// 5. (JYM2,JY1)
    //// 6. (UK,DX)
    //// 7. (VK,DY) 
    //// 8. (W2,WY2,W4)
    //// 9. (W3,WY3,W1,W5), 

    /// <summary>Represents Class SharedVariables for BivariateInterpolation program.</summary>
    public class SharedVariables
    {
        /// <summary>The pair1</summary>
        private float pair1 = 0;

        /// <summary>The pair2</summary>
        private int pair2 = 0;

        /// <summary>The pair3</summary>
        private int pair3 = 0;

        /// <summary>The pair4</summary>
        private int pair4 = 0;

        /// <summary>The pair5</summary>
        private int pair5 = 0;

        /// <summary>The pair6</summary>
        private float pair6 = 0;

        /// <summary>The pair7</summary>
        private float pair7 = 0;

        /// <summary>The pair8</summary>
        private float pair8 = 0;

        /// <summary>The pair9</summary>
        private float pair9 = 0;

        #region Pair 1

        /// <summary>Gets or sets the P00.</summary>
        /// <value>The P00.</value>
        public float P00
        {
            get
            {
                return this.pair1;
            }

            set
            {
                this.pair1 = value;
            }
        }

        /// <summary>Gets or sets the Z33.</summary>
        /// <value>The Z33.</value>
        public float Z33
        {
            get
            {
                return this.pair1;
            }

            set
            {
                this.pair1 = value;
            }
        }

        #endregion

        #region Pair 2

        /// <summary>Gets or sets the IMN.</summary>
        /// <value>The IMN.</value>
        public int IMN
        {
            get
            {
                return this.pair2;
            }

            set
            {
                this.pair2 = value;
            }
        }

        /// <summary>Gets or sets the JX.</summary>
        /// <value>The JX.</value>
        public int JX
        {
            get
            {
                return this.pair2;
            }

            set
            {
                this.pair2 = value;
            }
        }

        #endregion

        #region Pair 3

        /// <summary>Gets or sets the IMX.</summary>
        /// <value>The IMX.</value>
        public int IMX
        {
            get
            {
                return this.pair3;
            }

            set
            {
                this.pair3 = value;
            }
        }

        /// <summary>Gets or sets the JY.</summary>
        /// <value>The JY.</value>
        public int JY
        {
            get
            {
                return this.pair3;
            }

            set
            {
                this.pair3 = value;
            }
        }

        #endregion

        #region Pair 4

        /// <summary>Gets or sets the JXM2.</summary>
        /// <value>The JXM2.</value>
        public int JXM2
        {
            get
            {
                return this.pair4;
            }

            set
            {
                this.pair4 = value;
            }
        }

        /// <summary>Gets or sets the JX1.</summary>
        /// <value>The JX1.</value>
        public int JX1
        {
            get
            {
                return this.pair4;
            }

            set
            {
                this.pair4 = value;
            }
        }

        #endregion

        #region Pair 5

        /// <summary>Gets or sets the JYM2.</summary>
        /// <value>The JYM2.</value>
        public int JYM2
        {
            get
            {
                return this.pair5;
            }

            set
            {
                this.pair5 = value;
            }
        }

        /// <summary>Gets or sets the JY1.</summary>
        /// <value>The JY1.</value>
        public int JY1
        {
            get
            {
                return this.pair5;
            }

            set
            {
                this.pair5 = value;
            }
        }

        #endregion

        #region Pair 6

        /// <summary>Gets or sets the UK.</summary>
        /// <value>The UK.</value>
        public float UK
        {
            get
            {
                return this.pair6;
            }

            set
            {
                this.pair6 = value;
            }
        }

        /// <summary>Gets or sets the DX.</summary>
        /// <value>The DX.</value>
        public float DX
        {
            get
            {
                return this.pair6;
            }

            set
            {
                this.pair6 = value;
            }
        }

        #endregion

        #region Pair 7

        /// <summary>Gets or sets the VK.</summary>
        /// <value>The VK.</value>
        public float VK
        {
            get
            {
                return this.pair7;
            }

            set
            {
                this.pair7 = value;
            }
        }

        /// <summary>Gets or sets the DY.</summary>
        /// <value>The DY.</value>
        public float DY
        {
            get
            {
                return this.pair7;
            }

            set
            {
                this.pair7 = value;
            }
        }

        #endregion

        #region Pair 8

        /// <summary>Gets or sets the W2.</summary>
        /// <value>The W2.</value>
        public float W2
        {
            get
            {
                return this.pair8;
            }

            set
            {
                this.pair8 = value;
            }
        }

        /// <summary>Gets or sets the WY2.</summary>
        /// <value>The WY2.</value>
        public float WY2
        {
            get
            {
                return this.pair8;
            }

            set
            {
                this.pair8 = value;
            }
        }

        /// <summary>Gets or sets the W4.</summary>
        /// <value>The W4.</value>
        public float W4
        {
            get
            {
                return this.pair8;
            }

            set
            {
                this.pair8 = value;
            }
        }

        #endregion

        #region Pair 9

        /// <summary>Gets or sets the W3.</summary>
        /// <value>The W3.</value>
        public float W3
        {
            get
            {
                return this.pair9;
            }

            set
            {
                this.pair9 = value;
            }
        }

        /// <summary>Gets or sets the WY3.</summary>
        /// <value>The WY3.</value>
        public float WY3
        {
            get
            {
                return this.pair9;
            }

            set
            {
                this.pair9 = value;
            }
        }

        /// <summary>Gets or sets the W1.</summary>
        /// <value>The W1.</value>
        public float W1
        {
            get
            {
                return this.pair9;
            }

            set
            {
                this.pair9 = value;
            }
        }

        /// <summary>Gets or sets the W5.</summary>
        /// <value>The W5.</value>
        public float W5
        {
            get
            {
                return this.pair9;
            }

            set
            {
                this.pair9 = value;
            }
        }

        #endregion
    }
}
