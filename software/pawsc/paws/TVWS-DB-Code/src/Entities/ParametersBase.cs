// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;

    /// <summary>
    /// Base class for all paws parameters.
    /// </summary>
    [Serializable]
    public abstract class ParametersBase : IParameters
    {
        /// <summary>
        /// Gets or sets the parameter type.
        /// </summary>
        public abstract string Type { get; set; }

        /// <summary>
        /// Gets or sets the parameter version.
        /// </summary>
        public abstract string Version { get; set; }

        /// <summary>
        /// Gets or sets the parameter code.
        /// </summary>
        public abstract string Code { get; set; }

        /// <summary>
        /// Gets or sets the parameter message.
        /// </summary>
        public abstract string Message { get; set; }

        /// <summary>
        /// Gets or sets the parameter Data.
        /// </summary>
        public abstract string Data { get; set; }

        /// <summary>
        /// Gets or sets the parameter IncumbentType.
        /// </summary>
        public abstract string IncumbentType { get; set; }

        /// <summary>
        /// Gets or sets the registration identifier.
        /// </summary>
        /// <value>The registration identifier.</value>
        public abstract string RegistrationId { get; set; }

        /// <summary>
        /// Gets or sets the parameter UserId.
        /// </summary>
        public abstract string UserId { get; set; }

        /// <summary>
        /// Gets or sets the parameter UserId.
        /// </summary>
        public abstract string LPAUXRegId { get; set; }

        /// <summary>
        /// Gets or sets the parameter DeviceId.
        /// </summary>
        public abstract string DeviceId { get; set; }

        /// <summary>
        /// Gets or sets the parameter serial number.
        /// </summary>
        public abstract string SerialNumber { get; set; }

        /// <summary>
        /// Gets or sets the parameter registrant.
        /// </summary>
        public abstract Vcard Registrant { get; set; }

        /// <summary>
        /// Gets or sets the LPAUX registrant.
        /// </summary>
        /// <value>The LPAUX registrant.</value>
        public abstract Versitcard.VCard LPAuxRegistrant { get; set; }

        /// <summary>
        /// Gets or sets the parameter Contact.
        /// </summary>
        public abstract Versitcard.VCard Contact { get; set; }

        /// <summary>
        /// Gets or sets the parameter TransmitLocation.
        /// </summary>
        public abstract Location TransmitLocation { get; set; }

        /// <summary>
        /// Gets or sets the parameter PointArea.
        /// </summary>
        public abstract Point[] PointsArea { get; set; }

        /// <summary>
        /// Gets or sets the parameter MVPDLocation.
        /// </summary>
        public abstract Location MVPDLocation { get; set; }

        /// <summary>
        /// Gets or sets the parameter TEMPBASLocation.
        /// </summary>
        public abstract Location TempBasLocation { get; set; }

        /// <summary>
        /// Gets or sets the parameter QuadrilateralArea.
        /// </summary>
        public abstract QuadrilateralArea[] QuadrilateralArea { get; set; }

        /// <summary>
        /// Gets or sets the parameter Event.
        /// </summary>
        public abstract Event Event { get; set; }

        /// <summary>
        /// Gets or sets the device descriptor.
        /// </summary>
        public abstract DeviceDescriptor DeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the device location.
        /// </summary>
        public abstract GeoLocation Location { get; set; }

        /// <summary>
        /// Gets or sets the master device location.
        /// </summary>
        /// <value>The master device location.</value>
        public abstract GeoLocation MasterDeviceLocation { get; set; }

        /// <summary>
        /// Gets or sets the device owner.
        /// </summary>
        public abstract DeviceOwner DeviceOwner { get; set; }

        /// <summary>
        /// Gets or sets the antenna characteristics.
        /// </summary>
        public abstract AntennaCharacteristics Antenna { get; set; }

        /// <summary>
        /// Gets or sets the registration disposition.
        /// </summary>
        public abstract RegistrationDisposition RegistrationDisposition { get; set; }

        /// <summary>
        /// Gets or sets the device owner.
        /// </summary>
        public abstract DeviceOwner Owner { get; set; }

        /// <summary>
        /// Gets or sets the device capabilities.
        /// </summary>
        public abstract DeviceCapabilities Capabilities { get; set; }

        ///// <summary>
        ///// Gets or sets the master device descriptors.
        ///// </summary>
        ////public abstract DeviceDescriptor MasterDevicesDescriptors { get; set; }

        /// <summary>
        /// Gets or sets the master device descriptors of the paws request.
        /// </summary>
        /// <value>The master device descriptors.</value>
        public abstract DeviceDescriptor MasterDeviceDescriptors { get; set; }

        /// <summary>
        /// Gets or sets the TV spectrum.
        /// </summary>
        public abstract TvSpectrum TvSpectrum { get; set; }

        /// <summary>
        /// Gets or sets the TV spectrum.
        /// </summary>
        public abstract TvSpectrum[] TvSpectra { get; set; }

        /// <summary>
        /// Gets or sets the request type.
        /// </summary>
        public abstract string RequestType { get; set; }

        /// <summary>
        /// Gets or sets the venue.
        /// </summary>
        public abstract string Venue { get; set; }

        /// <summary>
        /// Gets or sets the start time.
        /// </summary>
        public abstract string StartTime { get; set; }

        /// <summary>
        /// Gets or sets the end time.
        /// </summary>
        public abstract string EndTime { get; set; }

        /// <summary>
        /// Gets or sets the time stamp.
        /// </summary>
        public abstract string TimeStamp { get; set; }

        /// <summary>
        /// Gets or sets the requestor.
        /// </summary>
        public abstract DeviceOwner Requestor { get; set; }

        /// <summary>
        /// Gets or sets the device location(s).
        /// </summary>
        public abstract GeoLocation[] Locations { get; set; }

        /// <summary>
        /// Gets or sets the device spectra.
        /// </summary>
        public abstract Spectrum[] Spectra { get; set; }

        /// <summary>
        /// Gets or sets the device descriptors.
        /// </summary>
        public abstract DeviceDescriptor[] DevicesDescriptors { get; set; }

        /// <summary>
        /// Gets or sets the REFSENS.
        /// </summary>
        /// <value>The REFSENS.</value>
        public abstract double Prefsens { get; set; }

        /// <summary>
        /// Gets or sets the maximum P0 master.
        /// </summary>
        /// <value>The maximum P0 master.</value>
        public abstract double MaxMasterEIRP { get; set; }

        /// <summary>
        /// Gets or sets the MVPD registrant.
        /// </summary>
        /// <value>The MVPD registrant.</value>
        public abstract Versitcard.VCard MVPDRegistrant { get; set; }

        /// <summary>
        /// Gets or sets the ULS file number.
        /// </summary>
        /// <value>The ULS file number.</value>
        public abstract string ULSFileNumber { get; set; }

        /// <summary>
        /// Gets or sets the temporary BAS registrant.
        /// </summary>
        /// <value>The temporary BAS registrant.</value>
        public abstract Versitcard.VCard TempBASRegistrant { get; set; }

        /// <summary>
        /// Gets or sets the unique identifier.
        /// </summary>
        /// <value>The unique identifier.</value>
        public abstract string UniqueId { get; set; }

        /// <summary>
        /// Gets or sets the contour request call sign.
        /// </summary>
        /// <value>The contour request call sign.</value>
        public abstract string ContourRequestCallSign { get; set; }

        /// <summary>
        /// Gets or sets the testing stage.
        /// </summary>
        /// <value>The testing stage.</value>
        public abstract int TestingStage { get; set; }

        /// <summary>
        /// Gets or sets the name of the PMSE assignment table.
        /// </summary>
        /// <value>The name of the PMSE assignment table.</value>
        public abstract string PMSEAssignmentTableName { get; set; }

        /// <summary>
        /// Gets the paws Init Request parameters.
        /// </summary>
        /// <returns>Returns the InitRequestBase parameter.</returns>
        public abstract InitRequestBase GetInitRequest();

        /// <summary>
        /// Gets the paws register request parameters parameters.
        /// </summary>
        /// <returns>Returns the RegisterRequestBase parameter.</returns>
        public abstract RegisterRequestBase GetRegisterRequest();

        /// <summary>
        /// Gets the paws available spectrum request parameters.
        /// </summary>
        /// <returns>Returns the AvailableSpectrumRequestBase parameter.</returns>
        public abstract AvailableSpectrumRequestBase GetAvailableSpectrumRequest();

        /// <summary>
        /// Gets the paws interference query request parameters.
        /// </summary>
        /// <returns>Returns the InterferenceQueryRequestBase parameter.</returns>
        public abstract InterferenceQueryRequestBase GetInterferenceQueryRequest();

        /// <summary>
        /// Gets the paws available spectrum request parameters.
        /// </summary>
        /// <returns>Returns the BatchAvailableSpectrumRequestBase parameters.</returns>
        public abstract BatchAvailableSpectrumRequestBase GetBatchAvailableSpectrumRequest();

        /// <summary>
        /// Gets the paws notify request parameters.
        /// </summary>
        /// <returns>Returns the NotifyRequestBase parameters.</returns>
        public abstract NotifyRequestBase GetNotifyRequest();

        /// <summary>
        /// Gets the paws device valid request parameters.
        /// </summary>
        /// <returns>Returns the DeviceValidityRequestBase parameters.</returns>
        public abstract DeviceValidityRequestBase GetDeviceValidRequest();
    }
}
