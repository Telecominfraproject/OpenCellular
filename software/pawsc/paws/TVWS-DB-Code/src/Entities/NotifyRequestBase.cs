// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.ComponentModel.DataAnnotations;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;

    /// <summary>
    /// Base class for a paws notify request.
    /// </summary>
    [Serializable]
    public abstract class NotifyRequestBase : INotifyRequest
    {
        /// <summary>
        /// Gets or sets the device descriptor.
        /// </summary>
        /// <value>The device descriptor.</value>
        [Required(ErrorMessage = Constants.ErrorMessageInitDeviceDescriptorRequired)]
        [ObjectValidator]
        public virtual DeviceDescriptor DeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the master device descriptor.
        /// </summary>
        /// <value>The master device descriptor.</value>
        [ObjectValidator]
        public virtual DeviceDescriptor MasterDeviceDescriptors { get; set; }

        /// <summary>
        /// Gets or sets the master device location.
        /// </summary>
        /// <value>The master device location.</value>
        [ObjectValidator]
        public virtual GeoLocation MasterDeviceLocation { get; set; }
       
        /// <summary>
        /// Gets or sets the device location.
        /// </summary>
        /// <value>The location.</value>
        [Required(ErrorMessage = Constants.ErrorMessageInitLocationRequired)]
        [ObjectValidator]
        public virtual GeoLocation Location { get; set; }

        /// <summary>
        /// Gets or sets the spectra.
        /// </summary>
        /// <value>The spectra.</value>
        [Required(ErrorMessage = Constants.ErrorMessageNotifySpectraRequired)]
        [ObjectCollectionValidator]
        public virtual Spectrum[] Spectra { get; set; }

        /// <summary>
        /// Gets or sets the antenna characteristics of the paws request.
        /// </summary>
        [ObjectValidator]
        public virtual AntennaCharacteristics Antenna { get; set; }
    }
}
