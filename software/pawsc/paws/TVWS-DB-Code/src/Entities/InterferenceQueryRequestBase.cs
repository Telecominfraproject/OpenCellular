// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.ComponentModel.DataAnnotations;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;

    /// <summary>
    /// Contains the Interference Query Request parameters.
    /// </summary>
    [Serializable]
    public abstract class InterferenceQueryRequestBase : IInterferenceQueryRequest
    {
        /// <summary>
        /// Gets or sets the device descriptor.
        /// </summary>
        [ObjectValidator]
        [Required(ErrorMessage = Constants.ErrorMessageTimeStampRequired)]
        public virtual string TimeStamp { get; set; }

        /// <summary>
        /// Gets or sets the device location.
        /// </summary>
        [ObjectValidator]
        [Required(ErrorMessage = Constants.ErrorMessageInitLocationRequired)]
        public virtual GeoLocation Location { get; set; }

        /// <summary>
        /// Gets or sets the device owner.
        /// </summary>
        [ObjectValidator]
        [Required(ErrorMessage = Constants.ErrorMessageDeviceOwnerRequired)]
        public virtual DeviceOwner Requestor { get; set; }

        /// <summary>
        /// Gets or sets the Start Time.
        /// </summary>
        [ObjectValidator]
        [Required(ErrorMessage = Constants.ErrorMessageInterferenceQueryStartTimeRequired)]
        public virtual string StartTime { get; set; }

        /// <summary>
        /// Gets or sets the End Time.
        /// </summary>
        [ObjectValidator]
        [Required(ErrorMessage = Constants.ErrorMessageEndTimeRequired)]
        public virtual string EndTime { get; set; }

        /// <summary>
        /// Gets or sets the Request Type.
        /// </summary>
        public virtual string RequestType { get; set; }
    }
}
