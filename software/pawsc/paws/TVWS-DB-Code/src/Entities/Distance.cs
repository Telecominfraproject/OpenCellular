// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    /// <summary>Represent DistanceUnit</summary>
    public enum DistanceUnit
    {
        /// <summary>The feet</summary>
        Feet,

        /// <summary>The meter</summary>
        Meter,

        /// <summary>The km</summary>
        KM,

        /// <summary>The miles</summary>
        Miles
    }

    /// <summary>Represents Distance.</summary>
    public class Distance
    {
        /// <summary>The unit value</summary>
        private DistanceUnit unitValue;

        /// <summary>The value</summary>
        private double value;

        /// <summary>Initializes a new instance of the <see cref="Distance" /> class.</summary>
        /// <param name="value">The value.</param>
        /// <param name="unit">The unit.</param>
        public Distance(double value, DistanceUnit unit)
        {
            this.value = value;
            this.unitValue = unit;
        }

        /// <summary>Initializes a new instance of the <see cref="Distance" /> class.</summary>
        /// <param name="distance">The distance.</param>
        public Distance(Distance distance)
        {
            this.value = distance.Value;
            this.unitValue = distance.Unit;
        }

        /// <summary>
        /// Gets the unit.
        /// </summary>
        /// <value>The unit.</value>
        public DistanceUnit Unit
        {
            get { return this.unitValue; }
        }

        /// <summary>
        /// Gets the value.
        /// </summary>
        /// <value>The value.</value>
        public double Value
        {
            get { return this.value; }
        }

        /// <summary>Same the unit.</summary>
        /// <param name="distance">The distance.</param>
        /// <returns><c>true</c> if same unit, <c>false</c> otherwise.</returns>
        public bool SameUnit(Distance distance)
        {
            return distance.Unit == this.unitValue;
        }
    }
}
