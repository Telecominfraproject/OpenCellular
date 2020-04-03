// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System.Collections.Generic;
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents the Paws Result.
    /// </summary>
    [JsonConverter(typeof(ResultConverter))]
    public class Result
    {
        /// <summary>
        /// Gets or sets the Paws Result Type.
        /// </summary>
        [JsonProperty(Constants.PropertyNameType)]
        public string Type { get; set; }

        /// <summary>
        /// Gets or sets the Paws Result Code.
        /// </summary>
        [JsonProperty(Constants.PawsPropertyNameCode)]
        public string Code { get; set; }

        /// <summary>
        /// Gets or sets the Paws Result Message.
        /// </summary>
        [JsonProperty(Constants.PropertyNameMessage)]
        public string Message { get; set; }

        /// <summary>
        /// Gets or sets the Paws Result Data.
        /// </summary>
        [JsonProperty(Constants.PropertyNameData)]
        public string Data { get; set; }

        /// <summary>
        /// Gets or sets the Paws Result Version.
        /// </summary>
        [JsonProperty(Constants.PropertyNameVersion)]
        public string Version { get; set; }

        /// <summary>
        /// Gets or sets the Paws Result Rule Set Info.
        /// </summary>
        [JsonProperty(Constants.PropertyNameRulesetInfo)]
        public RulesetInfo[] RulesetInfo { get; set; }

        /// <summary>
        /// Gets or sets the Paws Result DB changes.
        /// </summary>
        [JsonProperty(Constants.PropertyNameDatabaseChange)]
        public DbUpdateSpec DatabaseChange { get; set; }

        /// <summary>
        /// Gets or sets the Paws Result  time-stamp.
        /// </summary>
        [JsonProperty(Constants.PropertyNameTimeStamp)]
        public string TimeStamp { get; set; }

        /// <summary>
        /// Gets or sets the Paws Result device descriptor.
        /// </summary>
        [JsonProperty(Constants.PropertyNameDeviceDescriptor)]
        public DeviceDescriptor DeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the Paws Result spectrum schedules.
        /// </summary>
        [JsonProperty(Constants.PropertyNameSpectrumSchedules)]
        public SpectrumSchedule[] SpectrumSchedules { get; set; }

        /// <summary>
        /// Gets or sets the Paws Result spectrum schedules.
        /// </summary>
        [JsonProperty(Constants.PropertyNameGeoSpectrumSpecs)]
        public GeoSpectrumSpec[] GeoSpectrumSpecs { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether a spectrum report is needed.
        /// </summary>
        [JsonProperty(Constants.PropertyNameNeedsSpectrumReport)]
        public bool? NeedsSpectrumReport { get; set; }

        /// <summary>
        /// Gets or sets the spectrum specs.
        /// </summary>
        /// <value>The spectrum specs.</value>
        [JsonProperty(Constants.PropertyNameSpectrumSpecs)]
        public SpectrumSpec[] SpectrumSpecs { get; set; }

        /// <summary>
        /// Gets or sets the Paws Result maximum total bandwidth in Hz.
        /// </summary>
        [JsonProperty(Constants.PropertyNameMaxTotalBwHz)]
        public float? MaxTotalBwHz { get; set; }

        /// <summary>
        /// Gets or sets the Paws maximum contiguous bandwidth in Hz.
        /// </summary>
        [JsonProperty(Constants.PropertyNameMaxContiguousBwHz)]
        public float? MaxContiguousBwHz { get; set; }

        /// <summary>
        /// Gets or sets the Maximum Channel Bandwidth.
        /// </summary>
        [JsonProperty(Constants.PropertyNameMaxNominalChannelBwMhz)]
        public float? MaxNominalChannelBwMhz { get; set; }

        /// <summary>
        /// Gets or sets the Paws Result Location.
        /// </summary>
        [JsonProperty(Constants.PropertyNameLocation)]
        public GeoLocation Location { get; set; }

        /// <summary>
        /// Gets or sets the Paws Result Spectrum Schedules.
        /// </summary>
        [JsonProperty(Constants.PropertyNameGeoSpectrumSchedules)]
        public GeoSpectrumSchedule[] GeoSpectrumSchedules { get; set; }

        /// <summary>
        /// Gets or sets the Paws Result Validities.
        /// </summary>
        [JsonProperty(Constants.PropertyNameDeviveValidities)]
        public DeviceValidity[] DeviceValidities { get; set; }

        /// <summary>
        /// Gets or sets the Region Management ChannelInfo.
        /// </summary>
        [JsonProperty(Constants.PropertyNameChannelInfo)]
        public ChannelInfo[] ChannelInfo { get; set; }

        /// <summary>
        /// Gets or sets the Region Management ChannelInfo.
        /// </summary>
        [JsonProperty(Constants.PropertyNameDeviceList)]
        public ProtectedDevice[] DeviceList { get; set; }

        /// <summary>
        /// Gets or sets the Region Management ChannelInfo.
        /// </summary>
        [JsonProperty(Constants.PropertyNameIncumbentList)]
        public object[] IncumbentList { get; set; }

        /// <summary>
        /// Gets or sets the Region Management ChannelInfo.
        /// </summary>
        [JsonProperty(Constants.PropertyNameUserList)]
        public User User { get; set; }

        /// <summary>
        /// Gets or sets the PMSE Unlicensed Registrations.
        /// </summary>
        [JsonProperty(Constants.PropertyNameLpAuxUnlicensedList)]
        public List<LPAuxRegistration> LPAUXUnlicensedList { get; set; }

        /// <summary>
        ///     Gets or sets the device identifier.
        /// </summary>
        /// <value>The device identifier.</value>
        [JsonProperty(Constants.PropertyNameRegionManagementUniqueId)]
        public string UniqueId { get; set; }

        /// <summary>
        ///     Gets or sets the type of the request.
        /// </summary>
        /// <value>The type of the request.</value>
        [JsonProperty(Constants.PropertyNameRegionManagementRequestType)]
        public string RequestType { get; set; }

        /// <summary>
        ///     Gets or sets the start time.
        /// </summary>
        /// <value>The start time.</value>
        [JsonProperty(Constants.PropertyNameRegionManagementStartTime)]
        public string StartTime { get; set; }

        /// <summary>
        ///     Gets or sets the end time.
        /// </summary>
        /// <value>The end time.</value>
        [JsonProperty(Constants.PropertyNameRegionManagementEndTime)]
        public string EndTime { get; set; }

        /// <summary>
        ///     Gets or sets the channels.
        /// </summary>
        /// <value>The channels.</value>
        [JsonProperty(Constants.PropertyNameRegionManagementChannels)]
        public ChannelInfo[] Channels { get; set; }

        /// <summary>
        /// Gets or sets the LPAUX licenses.
        /// </summary>
        /// <value>The LPAUX licenses.</value>
        public LpAuxLicenseInfo[] LpAuxLicenses { get; set; }

        /// <summary>
        /// Gets or sets the TV stations.
        /// </summary>
        /// <value>The TV stations.</value>
        public MVPDCallSignsInfo[] SearchMVPDCallSigns { get; set; }

        /// <summary>
        /// Gets or sets the TV station.
        /// </summary>
        /// <value>The TV station.</value>
        [JsonProperty(Constants.PropertyNameCallsignInfo)]
        public MVPDCallSignsInfo CallsignInfo { get; set; }

        /// <summary>
        /// Gets or sets the channels in CSV.
        /// </summary>
        /// <value>The channels in CSV.</value>
        public string ChannelsInCSV { get; set; }

        /// <summary>
        /// Gets or sets the intermediate results1.
        /// </summary>
        /// <value>The intermediate results1.</value>
        public string IntermediateResults1 { get; set; }

        /// <summary>
        /// Gets or sets the master operation parameters.
        /// </summary>
        /// <value>The master operation parameters.</value>
        public string MasterOperationParameters { get; set; }

        /// <summary>
        /// Gets or sets the contour data.
        /// </summary>
        /// <value>The contour data.</value>
        public ContourDetailsInfo[] ContourData { get; set; }
    }
}
