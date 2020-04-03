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
    /// Represents iCalendar data: http://tools.ietf.org/html/draft-daboo-et-al-icalendar-in-xml-09
    /// </summary>
    [XmlRoot("properties", Namespace = Constants.ICalXmlns)]
    public class Calendar
    {
        // ToDo: Need to double-check what fields are required from iCal -- for instance, not sure if Stamp is needed.

        /// <summary>
        /// Gets or sets the Stamp time.
        /// </summary>
        public string Stamp { get; set; }

        /// <summary>
        /// Gets or sets the start time.
        /// </summary>
        public string Start { get; set; }

        /// <summary>
        /// Gets or sets the end time.
        /// </summary>
        public string End { get; set; }

        /// <summary>
        /// Gets or sets the recurrence.
        /// </summary>
        /// <value>The recurrence.</value>
        public EventRecurrence Recurrence { get; set; }

        /// <summary>
        /// Gets or sets the Calendar Id.
        /// </summary>
        public string UId { get; set; }
    }
}
