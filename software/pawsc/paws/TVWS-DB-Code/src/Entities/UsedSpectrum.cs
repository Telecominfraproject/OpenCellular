// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Xml.Serialization;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents the paws notify spectrum parameters.
    /// </summary>
    public class UsedSpectrum : TableEntity
    {
        /// <summary>
        /// Gets or sets the Device Descriptor.
        /// </summary>
        public DeviceDescriptor DeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the master device descriptor.
        /// </summary>
        /// <value>The master device descriptor.</value>
        public DeviceDescriptor MasterDeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the master device location.
        /// </summary>
        /// <value>The master device location.</value>
        public Location MasterDeviceLocation { get; set; }

        /// <summary>
        /// Gets or sets the Location.
        /// </summary>
        public Location Location { get; set; }

        /// <summary>
        /// Gets or sets the Spectra.
        /// </summary>
        public Spectrum Spectra { get; set; }

        /// <summary>
        /// Gets or sets the WSDBA.
        /// </summary>
        public string WSDBA { get; set; }

        /// <summary>
        /// Gets or sets the scheduled event time.
        /// </summary>
        public EventTime EventTime { get; set; }

        /// <summary>
        /// Gets or sets the latitude position.
        /// </summary>
        public double Latitude { get; set; }

        /// <summary>
        /// Gets or sets the longitude position.
        /// </summary>
        public double Longitude { get; set; }

        /// <summary>
        /// Gets or sets the latitude position.
        /// </summary>
        public double MasterDeviceLatitude { get; set; }

        /// <summary>
        /// Gets or sets the longitude position.
        /// </summary>
        public double MasterDeviceLongitude { get; set; }

        /// <summary>
        /// Gets or sets the channel usage parameters.
        /// </summary>
        /// <value>The channel usage parameters.</value>
        public string ChannelUsageParameters { get; set; }

        /// <summary>
        /// Gets or sets the device identifier.
        /// </summary>
        /// <value>The device identifier.</value>
        public string DeviceId { get; set; }

        /// <summary>
        /// Gets or sets the used spectrum notify.
        /// </summary>
        /// <value>The used spectrum notify.</value>
        public string UsedSpectrumNotify { get; set; }

        /// <summary>
        /// Gets or sets the available channels.
        /// </summary>
        /// <value>available channel info at the time Notify spectrum usage is called.</value>
        public string AvaialableSpectrumSchedule { get; set; }

        /// <summary>
        /// Gets or sets the Device Info
        /// </summary>
        /// Serailaized value of Device Description
        public string DeviceInfo { get; set; }

        /// <summary>
        /// Gets or sets the Event Start Time
        /// </summary>      
        public string EventStartTime { get; set; }

        /// <summary>
        /// Gets or sets the Event End Time
        /// </summary>
        public string EventStopTime { get; set; }

        /// <summary>
        /// Gets or sets the Device Location Info        
        /// </summary>
        /// Serialized value of Device Location
        public string LocationInfo { get; set; }
    }
}
