// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System.Collections.Generic;

    /// <summary>
    ///     Represents Class KeyHoleContourItem.
    /// </summary>
    public class KeyHoleContourItem
    {
        /// <summary>
        ///     Gets or sets the key hole arc starting.
        /// </summary>
        /// <value>The key hole arc starting.</value>
        public int KeyHoleArcStarting { get; set; }

        /// <summary>
        ///     Gets or sets the key hole arc ending.
        /// </summary>
        /// <value>The key hole arc ending.</value>
        public int KeyHoleArcEnding { get; set; }

        /// <summary>
        ///     Gets or sets the co channel location.
        /// </summary>
        /// <value>The co channel location.</value>
        public List<Location> CoChannelLocations { get; set; }

        /// <summary>
        ///     Gets or sets the adjacent channel location.
        /// </summary>
        /// <value>The adjacent channel location.</value>
        public List<Location> AdjacentChannelLocations { get; set; }

        /// <summary>
        ///     Gets or sets the key hole co channel locations.
        /// </summary>
        /// <value>The key hole co channel locations.</value>
        public List<Location> KeyHoleCoChannelLocations { get; set; }

        /// <summary>
        ///     Gets or sets the key hole adjacent channel locations.
        /// </summary>
        /// <value>The key hole adjacent channel locations.</value>
        public List<Location> KeyHoleAdjacentChannelLocations { get; set; }
    }
}
