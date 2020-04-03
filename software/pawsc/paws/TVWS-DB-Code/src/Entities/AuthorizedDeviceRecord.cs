// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using WindowsAzure.Storage.Table;

    /// <summary>
    ///     Represents Class AuthorizedDeviceRecord.
    /// </summary>
    public class AuthorizedDeviceRecord : TableEntity
    {
        /// <summary>
        ///     Gets or sets the application purpose.
        /// </summary>
        /// <value>The application purpose.</value>
        public string ApplicationPurpose { get; set; }

        /// <summary>
        ///     Gets or sets the equipment class.
        /// </summary>
        /// <value>The equipment class.</value>
        public string EquipmentClass { get; set; }

        /// <summary>
        ///     Gets or sets the FCC identifier.
        /// </summary>
        /// <value>The FCC identifier.</value>
        public string FCCId { get; set; }

        /// <summary>
        ///     Gets or sets the status.
        /// </summary>
        /// <value>The status.</value>
        public string Status { get; set; }

        /// <summary>
        ///     Gets or sets the status date.
        /// </summary>
        /// <value>The status date.</value>
        public string StatusDate { get; set; }
    }
}
