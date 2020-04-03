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
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents a registration action.
    /// </summary>
    public enum RegistrationDispositionAction
    {
        /// <summary>
        /// Deletes the specified registration.
        /// </summary>
        Delete = 0,

        /// <summary>
        /// Adds the specified registration.
        /// </summary>
        Add = 1,

        /// <summary>
        /// Modifies the specified registration.
        /// </summary>
        Modify = 2
    }

    /// <summary>
    /// Represents a registration disposition.
    /// </summary>
    [XmlInclude(typeof(RegistrationDisposition))]
    [XmlRoot("RegistrationDisposition", Namespace = Constants.RegistrationRecordEnsembleXmlns)]
    public class RegistrationDisposition : TableEntity
    {
        /// <summary>
        /// Gets or sets the date of the registration.
        /// </summary>
        [XmlElement("RegistrationDate")]
        public string RegDate { get; set; }

        /// <summary>
        /// Gets or sets the Id of the registration.
        /// </summary>
        [XmlElement("RegID")]
        public string RegId { get; set; }

        /// <summary>
        /// Gets or sets the Action of the registration.
        /// </summary>
        [XmlElement("Action")]
        public int Action { get; set; }
    }
}
