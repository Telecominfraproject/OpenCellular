// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.ConstrainedExecution;
    using System.Text;
    using System.Threading.Tasks;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents a Channel Info class.
    /// </summary>
    public class ChannelInfo
    {
        /// <summary>
        /// Gets or sets the channel contour.
        /// </summary>
        public Contour Contonour { get; set; }

        /// <summary>
        /// Gets or sets the StartHz.
        /// </summary>
        public double StartHz { get; set; }

        /// <summary>
        /// Gets or sets the StopHz.
        /// </summary>
        public double StopHz { get; set; }

        /// <summary>
        /// Gets or sets the MaxPower.
        /// </summary>
        public double MaxPowerDBm { get; set; }

        /// <summary>
        /// Gets or sets the ChannelId.
        /// </summary>
        public int ChannelId { get; set; }

        /// <summary>
        /// Gets or sets the spectrum bandwidth.
        /// </summary>
        public double Bandwidth { get; set; }

        /// <summary>
        /// Gets or sets the device type.
        /// </summary>
        public string DeviceType { get; set; }
    }
}
