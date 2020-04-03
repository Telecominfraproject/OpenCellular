// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.ComponentModel.DataAnnotations;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;
 
    /// <summary>
    /// Base class that represents a paws available spectrum request.
    /// </summary>
    [Serializable]
    public abstract class AvailableSpectrumRequestBase : IAvailableSpectrumRequest
    {
       /// <summary>
       /// Gets or sets the device descriptor of the paws request.
       /// </summary>
       [ObjectValidator]
       public virtual DeviceDescriptor DeviceDescriptor { get; set; }

       /// <summary>
       /// Gets or sets the location of the paws request.
       /// </summary>
       [ObjectValidator]
       public virtual GeoLocation Location { get; set; }

       /// <summary>
       /// Gets or sets the master device location.
       /// </summary>
       /// <value>The master device location.</value>
       [ObjectValidator]
       public virtual GeoLocation MasterDeviceLocation { get; set; }
       
       /// <summary>
       /// Gets or sets the antenna characteristics of the paws request.
       /// </summary>
       [ObjectValidator]
       public virtual AntennaCharacteristics Antenna { get; set; }

       /// <summary>
       /// Gets or sets the owner of the paws request.
       /// </summary>
       [ObjectValidator]
       public virtual DeviceOwner Owner { get; set; }

       /// <summary>
       /// Gets or sets the Capabilities of the paws request device.
       /// </summary>
       [ObjectValidator]
       public virtual DeviceCapabilities Capabilities { get; set; }

       /// <summary>
       /// Gets or sets the master device descriptors of the paws request.
       /// </summary>
       [ObjectValidator]
       public virtual DeviceDescriptor MasterDeviceDescriptors { get; set; }

       /// <summary>
       /// Gets or sets the request type of the paws request.
       /// </summary>
       public virtual string RequestType { get; set; }

       /// <summary>
       /// Gets or sets the device owner.
       /// </summary>
       /// <value>The device owner.</value>
       public virtual DeviceOwner DeviceOwner { get; set; }
    }
}
