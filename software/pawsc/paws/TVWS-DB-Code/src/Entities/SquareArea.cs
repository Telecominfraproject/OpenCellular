// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    /// <summary>Represents SquareArea.</summary>
    public class SquareArea : Area
    {
        /// <summary>The size</summary>
        private Distance size;

        /// <summary>The bottom right point</summary>
        private Location bottomRightPoint;

        /// <summary>The top left point</summary>
        private Location topLeftPoint;

        /// <summary>Initializes a new instance of the <see cref="SquareArea" /> class.</summary>
        /// <param name="centreLocation">The centre location.</param>
        /// <param name="squareSize">Size of the square.</param>
        public SquareArea(Location centreLocation, Distance squareSize)
            : base(centreLocation)
        {
            this.size = squareSize;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SquareArea"/> class.
        /// </summary>
        /// <param name="topLeft">The top left.</param>
        /// <param name="bottomRight">The bottom right.</param>
        public SquareArea(Location topLeft, Location bottomRight)
        {
            this.topLeftPoint = topLeft;
            this.bottomRightPoint = bottomRight;
        }

        /// <summary>Gets the size.</summary>
        /// <value>The size.</value>
        public Distance Size
        {
            get { return this.size; }
        }

        /// <summary>Gets the bottom right point.</summary>
        /// <value>The bottom right point.</value>
        public Location BottomRightPoint
        {
            get { return this.bottomRightPoint; }
        }

        /// <summary>Gets the top left point.</summary>
        /// <value>The top left point.</value>
        public Location TopLeftPoint
        {
            get { return this.topLeftPoint; }
        }

        /// <summary>
        /// Sets the area.
        /// </summary>
        /// <param name="topLeft">The top left.</param>
        /// <param name="bottomRight">The bottom right.</param>
        public void SetArea(Location topLeft, Location bottomRight)
        {
            this.topLeftPoint = topLeft;
            this.bottomRightPoint = bottomRight;
        }
    }
}
