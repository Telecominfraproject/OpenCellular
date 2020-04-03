// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents the Contour of an incumbent.
    /// </summary>
    public class Contour
    {
        /// <summary>Initializes a new instance of the <see cref="Contour" /> class.</summary>
        public Contour()
        {
        }
        
        /// <summary>
        /// Gets or sets the contour points.
        /// </summary>
        /// <value>The contour points.</value>
        public List<Location> ContourPoints { get; set; }

        /// <summary>
        /// Gets or sets the contour point items.
        /// </summary>
        /// <value>The contour point items.</value>
        public List<ContourPointItem> ContourPointItems { get; set; }

        /// <summary>
        /// Gets or sets the point.
        /// </summary>
        /// <value>The point.</value>
        public Location Point { get; set; }

        /// <summary>
        /// Locations in the string.
        /// </summary>
        /// <returns>returns System.String.</returns>
        public string LocationsStr()
        {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < this.ContourPoints.Count; i++)
            {
                sb.AppendLine(this.ContourPoints[i].ToLocationString());
            }

            sb.AppendLine(this.Point.ToLocationString());

            return sb.ToString();
        }
    }
}
