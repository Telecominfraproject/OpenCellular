// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System.Collections.Generic;

    /// <summary>Represents Class ProtectedDevice.</summary>
    public class ProtectedDevice
    {
        /// <summary>Gets or sets the location.</summary>
        /// <value>The location.</value>
        public Location Location { get; set; }

        /// <summary>
        /// Gets or sets the channels.
        /// </summary>
        /// <value>The channels.</value>
        public int[] Channels { get; set; }

        /// <summary>
        /// Gets or sets the call sign.
        /// </summary>
        /// <value>The call sign.</value>
        public string CallSign { get; set; }

        /// <summary>Gets or sets the contour points.</summary>
        /// <value>The contour points.</value>
        public List<Location> ContourPoints { get; set; }

        /// <summary>Gets or sets the type of the protected device.</summary>
        /// <value>The type of the protected device.</value>
        public ProtectedDeviceType ProtectedDeviceType { get; set; }

        /// <summary>
        /// Gets or sets the keyhole arc points.
        /// </summary>
        /// <value>The keyhole arc points.</value>
        public List<Location> KeyholeArcPoints { get; set; }

        /// <summary>
        /// Gets or sets the keyhole arc bearing start.
        /// </summary>
        /// <value>The keyhole arc bearing start.</value>
        public int KeyholeArcBearingStart { get; set; }

        /// <summary>
        /// Gets or sets the keyhole arc bearing end.
        /// </summary>
        /// <value>The keyhole arc bearing end.</value>
        public int KeyholeArcBearingEnd { get; set; }

        /// <summary>Gets the name of the protected device type.</summary>
        /// <value>The name of the protected device type.</value>
        public string ProtectedDeviceTypeName
        {
            get
            {
                return ProtectedDeviceType.ToString();
            }
        }
    }
}
