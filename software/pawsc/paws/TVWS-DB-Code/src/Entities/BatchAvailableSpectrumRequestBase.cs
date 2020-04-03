// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.ComponentModel.DataAnnotations;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;

    /// <summary>
    /// Base class for all paws batch available request.
    /// </summary>
    [Serializable]
    public abstract class BatchAvailableSpectrumRequestBase : IBatchAvailableSpectrumRequest
    {
        /// <summary>
        /// Gets or sets the Device Descriptor.
        /// </summary>
        [ObjectValidator]
        public virtual DeviceDescriptor DeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the Device Locations.
        /// </summary>   
        [ObjectValidator]
        public virtual GeoLocation[] Locations { get; set; }

        /// <summary>
        /// Gets or sets the master device location.
        /// </summary>
        /// <value>The master device location.</value>
        [ObjectValidator]
        public virtual GeoLocation MasterDeviceLocation { get; set; }

        /// <summary>
        /// Gets or sets the device antenna characteristics.
        /// </summary>
        [ObjectValidator]
        public virtual AntennaCharacteristics Antenna { get; set; }

        /// <summary>
        /// Gets or sets the device owner.
        /// </summary>
        [ObjectValidator]
        public virtual DeviceOwner Owner { get; set; }

        /// <summary>
        /// Gets or sets the device capabilities..
        /// </summary>
        [ObjectValidator]
        public virtual DeviceCapabilities Capabilities { get; set; }

        /// <summary>
        /// Gets or sets the device master device descriptors.
        /// </summary>
        [ObjectValidator]
        public virtual DeviceDescriptor MasterDeviceDescriptors { get; set; }

        /// <summary>
        /// Gets or sets the device request type.
        /// </summary>
        public virtual string RequestType { get; set; }
    }
}
