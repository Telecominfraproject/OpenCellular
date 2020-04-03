// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Xml.Serialization;

    /// <summary>
    /// Represents Class Recurrence.
    /// </summary>
    public class EventRecurrence
    {
        /// <summary>
        /// Gets or sets the frequency.
        /// </summary>
        /// <value>The frequency.</value>
        ////[XmlElement("freq")]
        public string Frequency { get; set; }

        /// <summary>
        /// Gets or sets the until.
        /// </summary>
        /// <value>The until.</value>
        public string Until { get; set; }

        /// <summary>
        /// Gets or sets the count.
        /// </summary>
        /// <value>The count.</value>
        public int Count { get; set; }

        /// <summary>
        /// Gets or sets the BYDAY.
        /// </summary>
        /// <value>The BYDAY.</value>
        ////[XmlElement("byday")]
        public string ByDay { get; set; }

        /// <summary>
        /// Gets or sets the BYMINUTE.
        /// </summary>
        /// <value>The BYMINUTE.</value>
        ////[XmlElement("byminute")]
        public string ByMinute { get; set; }

        /// <summary>
        /// Gets or sets the BYHOUR.
        /// </summary>
        /// <value>The BYHOUR.</value>
        ////[XmlElement("byhour")]
        public string ByHour { get; set; }

        /// <summary>
        /// Gets or sets the interval.
        /// </summary>
        /// <value>The interval.</value>
        public string Interval { get; set; }
    }
}
