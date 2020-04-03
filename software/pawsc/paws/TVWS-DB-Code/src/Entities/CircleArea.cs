// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System.Collections.Generic;

    /// <summary>Represents CircleArea.
    /// </summary>
    public class CircleArea : Area
    {
        /// <summary>
        /// The radius
        /// </summary>
        private Distance radius;

        /// <summary>
        /// The points
        /// </summary>
        private List<Location> points;

        /// <summary>
        /// The arc angle
        /// </summary>
        private int arcAngle;

        /// <summary>
        /// Initializes a new instance of the <see cref="CircleArea" /> class.
        /// </summary>
        /// <param name="centreLocation">The centre location.</param>
        /// <param name="squareRadius">The square radius.</param>
        public CircleArea(Location centreLocation, Distance squareRadius)
            : base(centreLocation)
        {
            this.radius = squareRadius;
        }

        /// <summary>
        /// Gets the radius.
        /// </summary>
        /// <value>The radius.</value>
        public Distance Radius
        {
            get { return this.radius; }
        }

        /// <summary>
        /// Gets the points.
        /// </summary>
        /// <value>The points.</value>
        public List<Location> Points
        {
            get
            {
                return this.points;
            }
        }

        /// <summary>
        /// Gets the arc angle.
        /// </summary>
        /// <value>The arc angle.</value>
        public int ArcAngle
        {
            get { return this.arcAngle; }
        }

        /// <summary>
        /// Sets the points.
        /// </summary>
        /// <param name="points">The points.</param>
        /// <param name="arcAngle">The arc angle in degree.</param>
        public void SetPoints(List<Location> points, int arcAngle)
        {
            this.points = points;
            this.arcAngle = arcAngle;
        }
    }
}
