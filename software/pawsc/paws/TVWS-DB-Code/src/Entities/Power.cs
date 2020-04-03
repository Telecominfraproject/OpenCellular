// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;

    /// <summary>Represents PowerUnit  </summary>
    public enum PowerUnit
    {
        /// <summary>The watt </summary>
        Watt,

        /// <summary>The kilo watt </summary>
        KiloWatt,

        /// <summary>The mega watt </summary>
        MegaWatt,

        /// <summary>The d b </summary>
        dB
    }

    /// <summary>
    ///  This class presents a power value
    /// </summary>
    public class Power
    {
        /// <summary>The unit value </summary>
        private PowerUnit unitValue;

        /// <summary>The power value </summary>
        private double value;

        /// <summary>
        /// Initializes a new instance of the <see cref="Power" /> class.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="unit">The unit.</param>
        public Power(double value, PowerUnit unit)
        {
            this.value = value;
            this.unitValue = unit;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Power" /> class.
        /// </summary>
        /// <param name="power">The power.</param>
        public Power(Power power)
        {
            this.value = power.Value;
            this.unitValue = power.Unit;
        }

        /// <summary>Gets the unit.
        ///  </summary>
        /// <value>The unit.</value>
        public PowerUnit Unit
        {
            get { return this.unitValue; }
        }

        /// <summary>Gets the value.
        ///  </summary>
        /// <value>The value.</value>
        public double Value
        {
            get { return this.value; }
        }

        /// <summary>
        /// Same the unit.
        /// </summary>
        /// <param name="power">The power.</param>
        /// <returns><c>true</c> if same, <c>false</c> otherwise.</returns>
        public bool SameUnit(Power power)
        {
            return power.Unit == this.Unit;
        }
    }
}
