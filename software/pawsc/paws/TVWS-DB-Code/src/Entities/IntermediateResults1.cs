// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    /// <summary>
    ///     Struct IntermediateResults1
    /// </summary>
    public class IntermediateResults1
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="IntermediateResults1" /> class.
        /// </summary>
        /// <param name="coverage">The coverage.</param>
        /// <param name="pmseDistance">The pmse distance.</param>
        /// <param name="clutterType">Type of the clutter.</param>
        public IntermediateResults1(double coverage, double pmseDistance = 0.0, int clutterType = 0)
        {
            this.CoverageArea = coverage;
            this.DistanceToPmseVictim = pmseDistance;
            this.ClutterType = clutterType;
        }

        /// <summary>
        ///     Gets or sets the coverage area.
        /// </summary>
        /// <value>The coverage area.</value>
        public double CoverageArea { get; set; }

        /// <summary>
        ///     Gets or sets the PATH LOSS.
        /// </summary>
        /// <value>The PATH LOSS.</value>
        public double PathLoss { get; set; }

        /// <summary>
        ///     Gets or sets the height master.
        /// </summary>
        /// <value>The height master.</value>
        public double HeightMaster { get; set; }

        /// <summary>
        ///     Gets or sets the height slave.
        /// </summary>
        /// <value>The height slave.</value>
        public double HeightSlave { get; set; }

        /// <summary>
        ///     Gets or sets the distance to PMSE victim.
        /// </summary>
        /// <value>The distance to PMSE victim.</value>
        public double DistanceToPmseVictim { get; set; }

        /// <summary>
        ///     Gets or sets the frequency.
        /// </summary>
        /// <value>The frequency.</value>
        public double Frequency { get; set; }

        /// <summary>
        /// Gets or sets the type of the clutter.
        /// </summary>
        /// <value>The type of the clutter.</value>
        public int ClutterType { get; set; }
    }
}
