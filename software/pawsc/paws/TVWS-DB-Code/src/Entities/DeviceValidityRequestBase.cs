// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.ComponentModel.DataAnnotations;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;

    /// <summary>
    /// Represents the base class for a paws device validity request.
    /// </summary>
    [Serializable]
    public abstract class DeviceValidityRequestBase : IDeviceValidityRequest
    {
        /// <summary>
        /// Gets or sets the device descriptors.
        /// </summary>
        [Required(ErrorMessage = Constants.ErrorMessageDeviceDescriptorsRequired)]
        [ObjectCollectionValidator]
        public virtual DeviceDescriptor[] DevicesDescriptors { get; set; }

        /// <summary>
        /// Gets or sets the device descriptors.
        /// </summary>
        /// <value>The master device descriptors.</value>
         [ObjectCollectionValidator]
        public DeviceDescriptor MasterDeviceDescriptors { get; set; }
    }
}
