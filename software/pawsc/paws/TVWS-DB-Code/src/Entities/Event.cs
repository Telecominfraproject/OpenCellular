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
    /// Represents a Registration Event (PLAuxRegistration or TempBASRegistration).
    /// </summary>
    public class Event
    {
        /// <summary>
        /// holds channels array
        /// </summary>
        private int[] channels;

        /// <summary>
        /// Gets or sets the Times of the event.
        /// </summary>
        public Calendar[] Times { get; set; }

        /// <summary>
        /// Gets or sets the channels of the event.
        /// </summary>
        [XmlArray("eventChannel", Namespace = Constants.RegistrationRecordEnsembleXmlns)]
        [XmlArrayItem("chanNum", Namespace = Constants.RegistrationRecordEnsembleXmlns)]
        public int[] Channels
        {
            get { return this.channels; }
            set { this.channels = value; }
        }
    }
}
