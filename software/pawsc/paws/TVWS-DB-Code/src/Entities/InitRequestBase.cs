// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.ComponentModel.DataAnnotations;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;

    /// <summary>
    /// Base class for handling Init Request.
    /// </summary>
    [Serializable]
    public abstract class InitRequestBase : IInitRequest
    {
        /// <summary>
        /// Gets or sets the Device Descriptor.
        /// </summary>
        [ObjectValidator]
        [Required(ErrorMessage = Constants.ErrorMessageInitDeviceDescriptorRequired)]
        public virtual DeviceDescriptor DeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the Device Location.
        /// </summary>
        [ObjectValidator]
        [Required(ErrorMessage = Constants.ErrorMessageInitLocationRequired)]
        public virtual GeoLocation Location { get; set; }     
    }
}
