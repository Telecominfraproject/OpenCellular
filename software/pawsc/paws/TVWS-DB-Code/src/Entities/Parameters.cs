// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using Newtonsoft.Json;

    /// <summary>
    /// Class used to represent all paws parameters.
    /// </summary>
    [JsonConverter(typeof(ParameterConverter))]
    public class Parameters : ParametersBase
    {
        /// <summary>
        /// Initializes a new instance of the Parameters class.
        /// </summary>
        public Parameters()
        {
            this.UnKnownTypes = new Dictionary<string, string>();
        }

        /// <summary>
        /// Gets or sets the parameter type.
        /// </summary>
        public override string Type { get; set; }

        /// <summary>
        /// Gets or sets the parameter code.
        /// </summary>
        public override string Code { get; set; }

        /// <summary>
        /// Gets or sets the parameter message.
        /// </summary>
        public override string Message { get; set; }

        /// <summary>
        /// Gets or sets the parameter Data.
        /// </summary>
        public override string Data { get; set; }

        /// <summary>
        /// Gets or sets the parameter IncumbentType.
        /// </summary>
        public override string IncumbentType { get; set; }

        /// <summary>
        /// Gets or sets the registration identifier.
        /// </summary>
        /// <value>The registration identifier.</value>
        public override string RegistrationId { get; set; }

        /// <summary>
        /// Gets or sets the parameter UserId.
        /// </summary>
        public override string UserId { get; set; }

        /// <summary>
        /// Gets or sets the parameter UserId.
        /// </summary>
        public override string LPAUXRegId { get; set; }

        /// <summary>
        /// Gets or sets the parameter serial number.
        /// </summary>
        public override string SerialNumber { get; set; }

        /// <summary>
        /// Gets or sets the parameter version.
        /// </summary>
        public override string Version { get; set; }

        /// <summary>
        /// Gets or sets the device descriptor.
        /// </summary>
        public override DeviceDescriptor DeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the device location.
        /// </summary>
        public override GeoLocation Location { get; set; }

        /// <summary>
        /// Gets or sets the master device location.
        /// </summary>
        /// <value>The master device location.</value>
        public override GeoLocation MasterDeviceLocation { get; set; }

        /// <summary>
        /// Gets or sets the device owner.
        /// </summary>
        public override DeviceOwner DeviceOwner { get; set; }

        /// <summary>
        /// Gets or sets the antenna characteristics.
        /// </summary>
        public override AntennaCharacteristics Antenna { get; set; }

        /// <summary>
        /// Gets or sets the registration disposition.
        /// </summary>
        public override RegistrationDisposition RegistrationDisposition { get; set; }

        /// <summary>
        /// Gets or sets the device owner.
        /// </summary>
        public override DeviceOwner Owner { get; set; }

        /// <summary>
        /// Gets or sets the device capabilities.
        /// </summary>
        public override DeviceCapabilities Capabilities { get; set; }

        /// <summary>
        /// Gets or sets the master device descriptors.
        /// </summary>
        public override DeviceDescriptor MasterDeviceDescriptors { get; set; }

        /// <summary>
        /// Gets or sets the TV spectrum.
        /// </summary>
        public override TvSpectrum TvSpectrum { get; set; }

        /// <summary>
        /// Gets or sets the TV spectrum.
        /// </summary>
        public override TvSpectrum[] TvSpectra { get; set; }

        /// <summary>
        /// Gets or sets the parameter registrant.
        /// </summary>
        public override Vcard Registrant { get; set; }

        /// <summary>
        /// Gets or sets the MVPD registrant.
        /// </summary>
        /// <value>The MVPD registrant.</value>
        public override Versitcard.VCard MVPDRegistrant { get; set; }

        /// <summary>
        /// Gets or sets the LPAUX registrant.
        /// </summary>
        /// <value>The LPAUX registrant.</value>
        public override Versitcard.VCard LPAuxRegistrant { get; set; }

        /// <summary>
        /// Gets or sets the temporary BAS registrant.
        /// </summary>
        /// <value>The temporary BAS registrant.</value>
        public override Versitcard.VCard TempBASRegistrant { get; set; }

        /// <summary>
        /// Gets or sets the parameter Contact.
        /// </summary>
        public override Versitcard.VCard Contact { get; set; }

        /// <summary>
        /// Gets or sets the parameter TransmitLocation.
        /// </summary>
        public override Location TransmitLocation { get; set; }

        /// <summary>
        /// Gets or sets the parameter PointArea.
        /// </summary>
        public override Point[] PointsArea { get; set; }

        /// <summary>
        /// Gets or sets the parameter MVPDLocation.
        /// </summary>
        public override Location MVPDLocation { get; set; }

        /// <summary>
        /// Gets or sets the parameter TEMPBASLocation.
        /// </summary>
        public override Location TempBasLocation { get; set; }

        /// <summary>
        /// Gets or sets the parameter QuadrilateralArea.
        /// </summary>
        public override QuadrilateralArea[] QuadrilateralArea { get; set; }

        /// <summary>
        /// Gets or sets the parameter Event.
        /// </summary>
        public override Event Event { get; set; }

        /// <summary>
        /// Gets or sets the request type.
        /// </summary>
        public override string RequestType { get; set; }

        /// <summary>
        /// Gets or sets the venue.
        /// </summary>
        public override string Venue { get; set; }

        /// <summary>
        /// Gets or sets the start time.
        /// </summary>
        public override string StartTime { get; set; }

        /// <summary>
        /// Gets or sets the end time.
        /// </summary>
        public override string EndTime { get; set; }

        /// <summary>
        /// Gets or sets the time stamp.
        /// </summary>
        public override string TimeStamp { get; set; }

        /// <summary>
        /// Gets or sets the requestor.
        /// </summary>
        public override DeviceOwner Requestor { get; set; }

        /// <summary>
        /// Get or sets the device location(s).
        /// </summary>
        public override GeoLocation[] Locations { get; set; }

        /// <summary>
        /// Gets or sets the device spectra.
        /// </summary>
        public override Spectrum[] Spectra { get; set; }

        /// <summary>
        /// Gets or sets the device descriptors.
        /// </summary>
        public override DeviceDescriptor[] DevicesDescriptors { get; set; }

        /// <summary>
        /// Gets or sets the unknown parameter types.
        /// </summary>
        [JsonIgnore]
        public Dictionary<string, string> UnKnownTypes { get; set; }

        /// <summary>
        /// Gets or sets the parameter DeviceId.
        /// </summary>
        public override string DeviceId { get; set; }

        /// <summary>
        /// Gets or sets the REFSENS.
        /// </summary>
        /// <value>The REFSENS.</value>
        public override double Prefsens { get; set; }

        /// <summary>
        /// Gets or sets the maximum P0 master.
        /// </summary>
        /// <value>The maximum P0 master.</value>
        public override double MaxMasterEIRP { get; set; }

        /// <summary>
        /// Gets or sets the ULS file number.
        /// </summary>
        /// <value>The ULS file number.</value>
        public override string ULSFileNumber { get; set; }

        /// <summary>
        /// Gets or sets the unique identifier.
        /// </summary>
        /// <value>The unique identifier.</value>
        public override string UniqueId { get; set; }

        /// <summary>
        /// Gets or sets the contour request call sign.
        /// </summary>
        /// <value>The contour request call sign.</value>
        public override string ContourRequestCallSign { get; set; }

        /// <summary>
        /// Gets or sets the testing stage.
        /// </summary>
        /// <value>The testing stage.</value>
        public override int TestingStage { get; set; }

        /// <summary>
        /// Gets or sets the name of the PMSE assignment table.
        /// </summary>
        /// <value>The name of the PMSE assignment table.</value>
        public override string PMSEAssignmentTableName { get; set; }

        /// <summary>
        /// Gets the paws Init Request parameters.
        /// </summary>
        /// <returns>Returns the InitRequestBase parameter.</returns>
        public override InitRequestBase GetInitRequest()
        {
            return new InitRequest
            {
                DeviceDescriptor = this.DeviceDescriptor,
                Location = this.Location,
                UnKnownTypes = this.UnKnownTypes
            };
        }

        /// <summary>
        /// Gets the paws register request parameters parameters.
        /// </summary>
        /// <returns>Returns the RegisterRequestBase parameter.</returns>
        public override RegisterRequestBase GetRegisterRequest()
        {
            return new RegisterRequest
            {
                DeviceDescriptor = this.DeviceDescriptor,
                Location = this.Location,
                DeviceOwner = this.DeviceOwner,
                Antenna = this.Antenna,
                UnKnownTypes = this.UnKnownTypes
            };
        }

        /// <summary>
        /// Gets the paws interference query request parameters parameters.
        /// </summary>
        /// <returns>Returns the InterferenceQueryRequestBase parameter.</returns>
        public override InterferenceQueryRequestBase GetInterferenceQueryRequest()
        {
            return new InterferenceQueryRequest
            {
                TimeStamp = this.TimeStamp,
                Location = this.Location,
                Requestor = this.Requestor,
                StartTime = this.StartTime,
                EndTime = this.EndTime,
                RequestType = this.RequestType,
                UnKnownTypes = this.UnKnownTypes
            };
        }

        /// <summary>
        /// Gets the paws available spectrum request parameters.
        /// </summary>
        /// <returns>Returns the AvailableSpectrumRequestBase parameter.</returns>
        public override AvailableSpectrumRequestBase GetAvailableSpectrumRequest()
        {
            return new AvailableSpectrumRequest
            {
                DeviceDescriptor = this.DeviceDescriptor,
                Location = this.Location,
                MasterDeviceDescriptors = this.MasterDeviceDescriptors,
                MasterDeviceLocation = this.MasterDeviceLocation,
                Antenna = this.Antenna,
                Capabilities = this.Capabilities,
                Owner = this.Owner,
                RequestType = this.RequestType,
                UnKnownTypes = this.UnKnownTypes
            };
        }

        /// <summary>
        /// Gets the paws available spectrum request parameters.
        /// </summary>
        /// <returns>Returns the BatchAvailableSpectrumRequestBase parameters.</returns>
        public override BatchAvailableSpectrumRequestBase GetBatchAvailableSpectrumRequest()
        {
            return new BatchAvailableSpectrumRequest
            {
                DeviceDescriptor = this.DeviceDescriptor,
                Locations = this.Locations,
                MasterDeviceDescriptors = this.MasterDeviceDescriptors,
                MasterDeviceLocation = this.MasterDeviceLocation,
                Antenna = this.Antenna,
                Capabilities = this.Capabilities,
                Owner = this.Owner,
                RequestType = this.RequestType,
                UnKnownTypes = this.UnKnownTypes
            };
        }

        /// <summary>
        /// Gets the paws notify request parameters.
        /// </summary>
        /// <returns>Returns the NotifyRequestBase parameters.</returns>
        public override NotifyRequestBase GetNotifyRequest()
        {
            return new NotifyRequest
            {
                DeviceDescriptor = this.DeviceDescriptor,
                Location = this.Location,
                Spectra = this.Spectra,
                MasterDeviceDescriptors = this.MasterDeviceDescriptors,
                MasterDeviceLocation = this.MasterDeviceLocation,
                Antenna = this.Antenna,
                UnKnownTypes = this.UnKnownTypes
            };
        }

        /// <summary>
        /// Gets the paws device valid request parameters.
        /// </summary>
        /// <returns>Returns the DeviceValidityRequestBase parameters.</returns>
        public override DeviceValidityRequestBase GetDeviceValidRequest()
        {
            List<DeviceDescriptor> deviceDescriptor = this.DevicesDescriptors.ToList();
            if (this.MasterDeviceDescriptors != null)
            {
                deviceDescriptor.Add(this.MasterDeviceDescriptors);
            }

            return new DeviceValidityRequest
            {
                DevicesDescriptors = deviceDescriptor.ToArray(),
                MasterDeviceDescriptors = this.MasterDeviceDescriptors,
                UnKnownTypes = this.UnKnownTypes
            };
        }
    }
}
