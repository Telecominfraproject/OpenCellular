// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.ComponentModel.DataAnnotations;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;

    /// <summary>
    /// Contains the paws register request parameters.
    /// </summary>
    [Serializable]
    public abstract class RegisterRequestBase : IRegisterRequest
    {
        /// <summary>
        /// Gets or sets the device descriptor.
        /// </summary>
        [ObjectValidator]
        [Required(ErrorMessage = Constants.ErrorMessageInitLocationRequired)]
        public virtual DeviceDescriptor DeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the device location.
        /// </summary>
        [ObjectValidator]
        [Required(ErrorMessage = Constants.ErrorMessageInitDeviceDescriptorRequired)]
        public virtual GeoLocation Location { get; set; }

        /// <summary>
        /// Gets or sets the device owner.
        /// </summary>
        [ObjectValidator]
        [Required(ErrorMessage = Constants.ErrorMessageDeviceOwnerRequired)]
        public virtual DeviceOwner DeviceOwner { get; set; }

        /// <summary>
        /// Gets or sets the antenna characteristics.
        /// </summary>
        public virtual AntennaCharacteristics Antenna { get; set; }

        /// <summary>
        /// Gets or sets the Registration Disposition.
        /// </summary>
        public virtual RegistrationDisposition RegistrationDisposition { get; set; }
    }
}
