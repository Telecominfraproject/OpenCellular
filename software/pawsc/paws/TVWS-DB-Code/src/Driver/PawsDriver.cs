// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Driver
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Net.Mail;
    using System.Runtime.Remoting.Messaging;
    using System.Text;
    using System.Threading.Tasks;
    using System.Xml;
    using System.Xml.Linq;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Common.ValueProviders;
    using Microsoft.Whitespace.Dalc;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents Class PawsDriver.
    /// </summary>
    public class PawsDriver : IDriverPaws
    {
        /// <summary>
        /// Stream Writer
        /// </summary>
        private StreamWriter sw = null;

        /// <summary>
        /// private variable to have web service URL for device validation
        /// </summary>
        private string deviceIdListUrl = Utils.Configuration["PawsDeviceValidationUrl"];

        /// <summary>
        /// private variable to have email for interference query file
        /// </summary>
        private string emailAddress;

        /// <summary>
        /// private variable to have the path for the file to get created and sent to the email
        /// </summary>
        private string csvPath;

        /// <summary>
        /// Gets or sets the IDALCPaws.
        /// </summary>
        [Dependency]
        public IDalcPaws PawsDalc { get; set; }

        /// <summary>
        /// Gets or sets the IDALCIncumbent.
        /// </summary>
        [Dependency]
        public IDalcIncumbent IncumbentDalc { get; set; }

        /// <summary>
        /// Gets or sets the common DALC.
        /// </summary>
        /// <value>The common DALC.</value>
        [Dependency]
        public IDalcCommon CommonDalc { get; set; }

        /// <summary>
        /// Gets or sets ILogger Interface
        /// </summary>
        [Dependency]
        public ILogger PawsLogger { get; set; }

        /// <summary>
        /// Gets or sets ILogger Interface
        /// </summary>
        [Dependency]
        public PawsRegionalValueProvider PawsValueProvider { get; set; }

        /// <summary>
        /// Gets or sets the paws validator.
        /// </summary>
        /// <value>The paws validator.</value>
        [Dependency]
        public IPawsValidator PawsValidator { get; set; }

        /// <summary>
        /// Gets or sets IRegionCalculation Interface
        /// </summary>
        [Dependency]
        public IRegionCalculation RegionCalculation { get; set; }

        #region IDriverPaws

        /// <summary>
        /// Return Settings Date
        /// </summary>
        /// <returns>Settings Date</returns>
        public int GetSequenceFromSettingsTable()
        {
            return this.CommonDalc.GetSequenceFromSettingsTable();
        }

        /// <summary>
        /// Return RuleSetInfo
        /// </summary>
        /// <param name="deviceDescriptor">spectrum Device Descriptor</param>
        /// <returns>Rule Set Info</returns>
        public RulesetInfo[] GetRuleSetInfo(DeviceDescriptor deviceDescriptor)
        {
            List<RulesetInfo> ruleSetInfo = this.PawsValueProvider.GetRuleSetInfo(deviceDescriptor);
            return ruleSetInfo.ToArray();
        }

        /// <summary>
        /// Returns Available Spectrums
        /// </summary>
        /// <param name="spectrumRequest">spectrum Request Query</param>
        /// <returns>Spectrum Schedules</returns>
        public SpectrumSchedule[] GetAvailableSpectrum(IAvailableSpectrumRequest spectrumRequest)
        {
            List<SpectrumSchedule> spectrumSchedules = new List<SpectrumSchedule>();
            const string LogMethodName = "PawsDriver.GetAvailableSpectrum(IAvailableSpectrumRequest spectrumRequest)";

            try
            {
                // Begin Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + LogMethodName);

                EventTime deviceTime = new EventTime()
                {
                    // Set Start Time to Now
                    StartTime = DateTime.UtcNow.ToString("u"),

                    // Set Stop Time at 1 day from Now
                    StopTime = DateTime.UtcNow.AddDays(1).ToString("u")
                };

                var spectrumList = new List<Spectrum>();

                // Pass stubbed incumbent information created from Request.
                var freeChannelsResp = this.RegionCalculation.GetFreeChannels(this.PawsValueProvider.GetWSDInfo<IAvailableSpectrumRequest>(spectrumRequest)[0]);
                var freeChannels = freeChannelsResp.ChannelsInfo;

                // Only one Spectrum Object for Each Bandwidth
                var bandwidthGroup = from channelInfo in freeChannels
                                     group channelInfo by channelInfo.Bandwidth
                                         into grpBandwidth
                                         select grpBandwidth;

                foreach (var bandwidth in bandwidthGroup)
                {
                    var currentSpectrum = new Spectrum();
                    currentSpectrum.ResolutionBwHz = Convert.ToSingle(bandwidth.Key);
                    var profiles = new List<SpectrumProfile>();
                    foreach (var profile in bandwidth.Select(obj => obj))
                    {
                        var currentFrequencyRange = new SpectrumProfile()
                        {
                            ChannelId = profile.ChannelId.ToString(),
                            Hz = (float)profile.StartHz,
                            DBm = (float)profile.MaxPowerDBm
                        };

                        profiles.Add(currentFrequencyRange);
                    }

                    currentSpectrum.Profiles = profiles.ToArray();
                    spectrumList.Add(currentSpectrum);
                }

                var specSchedule = new SpectrumSchedule
                {
                    EventTime = deviceTime,
                    Spectra = spectrumList.ToArray()
                };

                spectrumSchedules.Add(specSchedule);

                // End Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + LogMethodName);

                // Return the response
                return spectrumSchedules.ToArray();
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                return null;
            }
        }

        /// <summary>
        /// Gets the available batch spectrum.
        /// </summary>
        /// <param name="spectrumRequest">The spectrum request.</param>
        /// <returns>returns GeoSpectrumSchedule[][].</returns>
        public GeoSpectrumSpec[] GetAvailableSpectrumBatch(IBatchAvailableSpectrumRequest spectrumRequest)
        {
            List<GeoSpectrumSpec> spectrumSpec = new List<GeoSpectrumSpec>();
            const string LogMethodName = "PawsDriver.GetAvailableSpectrumBatch(IBatchAvailableSpectrumRequest spectrumRequest)";

            try
            {
                // Begin Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + LogMethodName);

                EventTime deviceTime = new EventTime()
                {
                    StartTime = DateTime.UtcNow.ToString("u"),
                    StopTime = DateTime.UtcNow.AddDays(1).ToString("u")
                };

                var incumbents = this.PawsValueProvider.GetWSDInfo<IBatchAvailableSpectrumRequest>(spectrumRequest);
                RulesetInfo[] ruleSetInfo = this.GetRuleSetInfo(spectrumRequest.DeviceDescriptor);
                object thisLock = new object();
                ////Action for Parallel Execution of GetFreeChannelsMethod
                Action<Incumbent> actionGetFreeChannels = (incumbentInfo) =>
                {
                    var freeChannelsResp = this.RegionCalculation.GetFreeChannels(incumbentInfo);
                    if (freeChannelsResp == null)
                    {
                        return;
                    }

                    var freeChannels = freeChannelsResp.ChannelsInfo;
                    if (freeChannels.Length <= 0)
                    {
                        return;
                    }

                    var spectrumList = new List<Spectrum>();
                    //// Only one Spectrum for Each Bandwidth
                    var bandwidthGroup = from channelInfo in freeChannels
                                         group channelInfo by channelInfo.Bandwidth
                                             into grpBandwidth
                                             select grpBandwidth;

                    foreach (var bandwidth in bandwidthGroup)
                    {
                        var currentSpectrum = new Spectrum();
                        currentSpectrum.ResolutionBwHz = Convert.ToSingle(bandwidth.Key);
                        var profiles = new List<SpectrumProfile>();
                        foreach (var profile in bandwidth.Select(obj => obj))
                        {
                            var currentFrequencyRange = new SpectrumProfile()
                            {
                                ChannelId = profile.ChannelId.ToString(),
                                Hz = (float)profile.StartHz,
                                DBm = (float)profile.MaxPowerDBm
                            };

                            profiles.Add(currentFrequencyRange);
                        }

                        currentSpectrum.Profiles = profiles.ToArray();
                        spectrumList.Add(currentSpectrum);
                    }

                    List<SpectrumSpec> SpectrumSpecArray = new List<SpectrumSpec>();
                    foreach (RulesetInfo ruleSet in ruleSetInfo)
                    {
                        float? maxTotalBwKhz = null;
                        float? maxNominalChannelBwKhz = null;
                        if (ruleSet.MaxTotalBwMhz.HasValue)
                        {
                            maxTotalBwKhz = (float)ruleSet.MaxTotalBwMhz;
                        }

                        if (ruleSet.MaxNominalChannelBwMhz.HasValue)
                        {
                            maxNominalChannelBwKhz = (float)ruleSet.MaxNominalChannelBwMhz;
                        }

                        SpectrumSpec spectrumSpecification = new SpectrumSpec();
                        spectrumSpecification.RulesetInfo = ruleSet;
                        spectrumSpecification.SpectrumSchedules = spectrumList.Select(spectrumInfo => new SpectrumSchedule()
                        {
                            EventTime = deviceTime,
                            Spectra = spectrumList.ToArray()
                        }).ToArray();
                        spectrumSpecification.NeedsSpectrumReport = true;
                        spectrumSpecification.MaxTotalBwHz = maxTotalBwKhz;
                        spectrumSpecification.MaxContiguousBwHz = maxNominalChannelBwKhz;
                        SpectrumSpecArray.Add(spectrumSpecification);
                    }

                    var specSchedule = new GeoSpectrumSpec()
                    {
                        Location = incumbentInfo.Location.ToGeoLocation(),
                        SpectrumSpecs = SpectrumSpecArray.ToArray()
                    };
                    lock (thisLock)
                    {
                        spectrumSpec.Add(specSchedule);
                    }
                };

                ////incumbents.AsParallel().ForAll(actionGetFreeChannels);
                Task[] tasks = new Task[incumbents.Length];
                for (int i = 0; i < incumbents.Length; i++)
                {
                    Task task;
                    task = Task.Run(() => actionGetFreeChannels(incumbents[i]));
                    task.Wait();
                    Task.WaitAll(task);
                    tasks[i] = task;
                }

                ////Task.WaitAll(tasks);

                // End Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + LogMethodName);

                // Return the response
                return spectrumSpec.ToArray();
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                return null;
            }
        }

        /// <summary>
        /// Return registration response
        /// </summary>
        /// <param name="registerrequest">Register information</param>
        /// <returns>Registration response</returns>
        public int Register(IRegisterRequest registerrequest)
        {
            int resp = 0;
            try
            {
                string registrationTableName = Utils.CurrentRegionPrefix + Constants.FixedTVBDRegistrationTable;
                string logMethodName = "PawsDriver.Register(IRegisterRequest registerrequest)";

                // Begin Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

                DateTime settingsDate = this.CommonDalc.GetDateSequenceFromSettingsTable();
                int sequenceNumber = this.GetSequenceFromSettingsTable();
                string regSeqNumber = sequenceNumber.ToString("0000000");
                string wsdba = Utils.RegistrationIdOrg;
                int rowCount = Convert.ToInt32(this.CommonDalc.FetchEntity(registrationTableName, registerrequest.DeviceDescriptor.SerialNumber));
                RegistrationDisposition regDisposition = new RegistrationDisposition();
                regDisposition.RegDate = settingsDate.ToString("yyyy'-'MM'-'dd'T'HH':'mm':'ss'Z'", DateTimeFormatInfo.InvariantInfo);
                if (rowCount == 0)
                {
                    regDisposition.Action = 1;
                }
                else
                {
                    regDisposition.Action = 2;
                }

                regDisposition.RegId = string.Format("{0:yyMMdd}", settingsDate) + Utils.RegistrationIdOrg + regSeqNumber;

                FixedTVBDRegistration fixedTvbdReg = new FixedTVBDRegistration()
                {
                    PartitionKey = registerrequest.DeviceDescriptor.SerialNumber,
                    RowKey = regDisposition.RegId,
                    SerialNumber = registerrequest.DeviceDescriptor.SerialNumber,
                    DeviceDescriptor = JsonSerialization.SerializeObject(registerrequest.DeviceDescriptor),
                    DeviceOwner = JsonSerialization.SerializeObject(registerrequest.DeviceOwner),
                    Location = JsonSerialization.SerializeObject(registerrequest.Location),
                    Antenna = JsonSerialization.SerializeObject(registerrequest.Antenna),
                    RegistrationDisposition = JsonSerialization.SerializeObject(regDisposition),
                    WSDBA = wsdba,
                    UniqueDeviceId = this.PawsValueProvider.GetDeviceId(registerrequest)
                };

                // Route to RegisterDevice method in AzureDalc
                resp = this.PawsDalc.RegisterDevice(registrationTableName, fixedTvbdReg);

                // End Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);

                // Return the response
                return resp;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                return -32000;
            }
        }

        /// <summary>
        /// Returns Device Information of the specified Id.
        /// </summary>
        /// <param name="id">The device Id.</param>
        /// <returns>Device Descriptor information.</returns>
        public DeviceDescriptor GetDeviceInfo(string id)
        {
            // ToDo: implement
            throw new NotImplementedException();
        }

        /// <summary>
        /// Returns all of the device descriptors.
        /// </summary>
        /// <returns>All of the device descriptors.</returns>
        public DeviceDescriptor[] GetDevices()
        {
            // ToDo: implement
            throw new NotImplementedException();
        }

        /// <summary>
        /// Return Initialization response
        /// </summary>
        /// <param name="initRequest">Init information</param>
        /// <returns>Init response</returns>
        public int Initialize(IInitRequest initRequest)
        {
            int resp = 0;
            try
            {
                string initializationTableName = Utils.CurrentRegionPrefix + Constants.InitializedDeviceDetailsTable;
                string registrationTableName = Utils.CurrentRegionPrefix + Constants.FixedTVBDRegistrationTable;
                string wsdba = Utils.RegistrationIdOrg;
                string logMethodName = "PawsDriver.Initialize(IInitRequest initRequest)";
                DeviceDescriptor device = initRequest.DeviceDescriptor;
                GeoLocation location = initRequest.Location;

                // Begin Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

                InitializedDeviceDetails deviceDetails = new InitializedDeviceDetails
                {
                    PartitionKey = device.SerialNumber,
                    RowKey = this.PawsValueProvider.GetDeviceId(initRequest),
                    DeviceDescriptor = JsonSerialization.SerializeObject(initRequest.DeviceDescriptor),
                    Location = JsonSerialization.SerializeObject(initRequest.Location),
                    WSDBA = wsdba
                };

                // Route to RegisterDevice method in AzureDalc
                resp = this.PawsDalc.Initialize(initializationTableName, deviceDetails);

                // End Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);

                // Return the response
                return resp;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                return -32000;
            }
        }

        /// <summary>
        /// Return device validation response
        /// </summary>
        /// <param name="deviceValidRequest">Device information</param>
        /// <param name="deviceIndex">Device Index</param>
        /// <returns>Device Validate response</returns>
        public int ValidateDevice(IDeviceValidityRequest deviceValidRequest, int deviceIndex)
        {
            int resp = 1;
            try
            {
                string deviceValidationTableName = Utils.GetRegionalTableName(Constants.RegisteredDeviceValidationTable);
                const string LogMethodName = "PawsDriver.ValidateDevice(IDeviceValidityRequest deviceValidRequest)";
                DeviceDescriptor[] device = deviceValidRequest.DevicesDescriptors;
                string deviceId = this.PawsValueProvider.GetDeviceId(device[deviceIndex]);

                // Begin Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + LogMethodName);

                var devices = this.CommonDalc.FetchEntity<RegisteredDevice>(deviceValidationTableName, new { DeviceId = deviceId });
                var isValidDevice = false;

                if (devices.Count > 0)
                {
                    var registeredDevice = devices[0];
                    isValidDevice = !(DateTime.Now > registeredDevice.Timestamp.DateTime.AddDays(1));
                }

                if (!isValidDevice)
                {
                    if (this.PawsValidator.ValidatePawsDevice(deviceId))
                    {
                        ////XDocument reply = XDocument.Parse(xmlOutput);
                        ////if (reply.Descendants(Utils.Configuration["deviceQueryParam"]).Any())
                        ////{
                        isValidDevice = true;
                        RegisteredDevice newDevice = new RegisteredDevice()
                        {
                            PartitionKey = Utils.RegistrationIdOrg,
                            RowKey = this.PawsValueProvider.GetDeviceId(device[deviceIndex]),
                            DeviceId = deviceId
                        };
                        this.PawsDalc.UpdateValidatedDevice(deviceValidationTableName, newDevice);
                        //// }
                    }
                }

                //// if device is valid return 1 else UNAUTHORIZED reponse code
                resp = isValidDevice ? 1 : Constants.ErrorCodeUnAuthorized;

                //// End Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + LogMethodName);

                //// Return the response
                return resp;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                return -32000;
            }
        }

        /// <summary>
        /// Saves the specified spectrum usage.
        /// </summary>
        /// <param name="notifyRequest">Contains the Paws spectrum usage notification request information.</param>
        /// <param name="deviceIndex">Contains the Device Index.</param>
        /// <returns>Spectrum Usage response</returns>
        public int NotifySpectrumUsage(INotifyRequest notifyRequest, out string errorMessage)
        {
            int resp = 0;
            errorMessage = string.Empty;

            try
            {
                string spectrumUsageNotifyTableName = Utils.CurrentRegionPrefix + Constants.SpectrumUsageTable;
                string wsdba = Utils.RegistrationIdOrg;
                string logMethodName = "PawsDriver.NotifySpectrumUsage(INotifyRequest notifyRequest)";

                // Begin Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

                // Get Available spectrum
                AvailableSpectrumRequest request = new AvailableSpectrumRequest
                {
                    DeviceDescriptor = notifyRequest.DeviceDescriptor,
                    Location = notifyRequest.Location,
                    Antenna = notifyRequest.Antenna
                };

                //GetAvailableSpectrum gives always only one record
                SpectrumSchedule spectrumSchedule = this.GetAvailableSpectrum(request).FirstOrDefault();

                //validate request
                bool isvalid = true;

                foreach (Spectrum spectra in notifyRequest.Spectra)
                {
                    var validSpectra = spectrumSchedule.Spectra.Where(x => x.ResolutionBwHz == spectra.ResolutionBwHz).FirstOrDefault();

                    if (validSpectra != null)
                    {
                        foreach (SpectrumProfile profile in spectra.Profiles)
                        {
                            var validProfile = validSpectra.Profiles.Where(x => x.DBm == profile.DBm && x.Hz == profile.Hz).FirstOrDefault();

                            if (validProfile == null)
                            {
                                errorMessage = "Spectrum with resolution Band Width " + spectra.ResolutionBwHz + " and profile with hz " + profile.Hz + " is unavailable";
                                isvalid = false;
                                break;
                            }
                        }
                    }
                    else
                    {
                        errorMessage = "Spectrum with resolution Band Width " + spectra.ResolutionBwHz + " is unavailable";
                        isvalid = false;
                        break;
                    }
                }


                if (isvalid)
                {
                    foreach (Spectrum spectrum in notifyRequest.Spectra)
                    {
                        UsedSpectrum spectrumDetails = new UsedSpectrum();
                        spectrumDetails.DeviceDescriptor = new DeviceDescriptor();
                        spectrumDetails.Location = new Location();
                        spectrumDetails.MasterDeviceDescriptor = new DeviceDescriptor();
                        spectrumDetails.MasterDeviceLocation = new Location();
                        spectrumDetails.Spectra = new Spectrum();
                        spectrumDetails.EventTime = new EventTime();
                        spectrumDetails.DeviceId = this.PawsValueProvider.GetDeviceId(notifyRequest);
                        spectrumDetails.ChannelUsageParameters = JsonSerialization.SerializeObject(spectrum);
                        spectrumDetails.EventStartTime = DateTime.UtcNow.ToString("yyyy'-'MM'-'dd'T'HH':'mm':'ss'Z'", DateTimeFormatInfo.InvariantInfo);
                        spectrumDetails.EventStopTime = DateTime.UtcNow.AddDays(1).ToString("yyyy'-'MM'-'dd'T'HH':'mm':'ss'Z'", DateTimeFormatInfo.InvariantInfo);                       

                        if (notifyRequest.MasterDeviceDescriptors != null)
                        {
                            spectrumDetails.MasterDeviceDescriptor.SerialNumber = notifyRequest.MasterDeviceDescriptors.SerialNumber;
                            spectrumDetails.MasterDeviceDescriptor.FccId = notifyRequest.MasterDeviceDescriptors.FccId;
                            spectrumDetails.MasterDeviceDescriptor.EtsiDeviceCategory = notifyRequest.MasterDeviceDescriptors.EtsiDeviceCategory;
                            spectrumDetails.MasterDeviceDescriptor.EtsiEnDeviceEmissionsClass = notifyRequest.MasterDeviceDescriptors.EtsiEnDeviceEmissionsClass;
                            spectrumDetails.MasterDeviceDescriptor.EtsiEnDeviceType = notifyRequest.MasterDeviceDescriptors.EtsiEnDeviceType;
                            spectrumDetails.MasterDeviceDescriptor.EtsiEnTechnologyId = notifyRequest.MasterDeviceDescriptors.EtsiEnTechnologyId;
                            spectrumDetails.MasterDeviceDescriptor.ManufacturerId = notifyRequest.MasterDeviceDescriptors.ManufacturerId;
                            spectrumDetails.MasterDeviceDescriptor.FccTvbdDeviceType = notifyRequest.MasterDeviceDescriptors.FccTvbdDeviceType;
                            spectrumDetails.MasterDeviceDescriptor.ModelId = notifyRequest.MasterDeviceDescriptors.ModelId;
                            spectrumDetails.MasterDeviceDescriptor.RulesetIds = notifyRequest.MasterDeviceDescriptors.RulesetIds;
                            spectrumDetails.MasterDeviceDescriptor.UnKnownTypes = notifyRequest.MasterDeviceDescriptors.UnKnownTypes;
                        }

                        if (notifyRequest.MasterDeviceLocation != null)
                        {
                            if (notifyRequest.MasterDeviceLocation.Region != null && notifyRequest.MasterDeviceLocation.Point == null)
                            {
                                foreach (Point exterior in notifyRequest.MasterDeviceLocation.Region.Exterior.ToList())
                                {
                                    spectrumDetails.MasterDeviceLatitude = Convert.ToDouble(exterior.Latitude);
                                    spectrumDetails.MasterDeviceLongitude = Convert.ToDouble(exterior.Longitude);
                                    spectrumDetails.MasterDeviceLocation.Latitude = Convert.ToDouble(exterior.Latitude);
                                    spectrumDetails.MasterDeviceLocation.Longitude = Convert.ToDouble(exterior.Longitude);
                                }
                            }
                            else if (notifyRequest.MasterDeviceLocation.Region == null && notifyRequest.MasterDeviceLocation.Point != null)
                            {
                                spectrumDetails.MasterDeviceLatitude = Convert.ToDouble(notifyRequest.MasterDeviceLocation.Point.Center.Latitude);
                                spectrumDetails.MasterDeviceLongitude = Convert.ToDouble(notifyRequest.MasterDeviceLocation.Point.Center.Longitude);

                                spectrumDetails.MasterDeviceLocation.Latitude = Convert.ToDouble(notifyRequest.MasterDeviceLocation.Point.Center.Latitude);
                                spectrumDetails.MasterDeviceLocation.Longitude = Convert.ToDouble(notifyRequest.MasterDeviceLocation.Point.Center.Longitude);
                            }
                        }

                        if (notifyRequest.Location.Region != null && notifyRequest.Location.Point == null)
                        {
                            foreach (Point exterior in notifyRequest.Location.Region.Exterior.ToList())
                            {
                                spectrumDetails.Latitude = Convert.ToDouble(exterior.Latitude);
                                spectrumDetails.Longitude = Convert.ToDouble(exterior.Longitude);
                                spectrumDetails.Location.Latitude = Convert.ToDouble(exterior.Latitude);
                                spectrumDetails.Location.Longitude = Convert.ToDouble(exterior.Longitude);
                            }
                        }
                        else if (notifyRequest.Location.Region == null && notifyRequest.Location.Point != null)
                        {
                            spectrumDetails.Latitude = Convert.ToDouble(notifyRequest.Location.Point.Center.Latitude);
                            spectrumDetails.Longitude = Convert.ToDouble(notifyRequest.Location.Point.Center.Longitude);
                            spectrumDetails.Location.Latitude = Convert.ToDouble(notifyRequest.Location.Point.Center.Latitude);
                            spectrumDetails.Location.Longitude = Convert.ToDouble(notifyRequest.Location.Point.Center.Longitude);
                        }

                        spectrumDetails.WSDBA = wsdba;
                        spectrumDetails.PartitionKey = wsdba;
                        spectrumDetails.RowKey = System.Guid.NewGuid().ToString();
                        spectrumDetails.UsedSpectrumNotify = JsonSerialization.SerializeObject(spectrumDetails);
                        spectrumDetails.AvaialableSpectrumSchedule = JsonSerialization.SerializeObject(spectrumSchedule);
                        spectrumDetails.DeviceInfo = JsonSerialization.SerializeObject(notifyRequest.DeviceDescriptor);
                        spectrumDetails.LocationInfo = JsonSerialization.SerializeObject(notifyRequest.Location);

                        resp = this.PawsDalc.NotifySpectrumUsage(spectrumUsageNotifyTableName, spectrumDetails);
                    }

                }
                else
                {
                    //Indicating spectrum is not available. Number is given as per paws protocol(sec 5.17)
                    resp = -202;
                }


                // End Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);

                // Return the response
                return resp;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                return -32000;
            }
        }

        /// <summary>
        /// Return interference query response
        /// </summary>
        /// <param name="interferenceRequest">Interference Query information</param>
        /// <returns>Interference Query response</returns>
        public int InterferenceQuery(IInterferenceQueryRequest interferenceRequest)
        {
            int resp = 0;
            try
            {
                string logMethodName = "PawsDriver.InterferenceQuery(IInterferenceQueryRequest interferenceRequest)";
                string spectrumUsageNotifyTableName = Utils.CurrentRegionPrefix + Constants.SpectrumUsageTable;
                this.emailAddress = interferenceRequest.Requestor.Owner.Email.Text;

                // Begin Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

                // Get all WSD devices that comes in given radial distance
                var whiteSpaceDevices = this.PawsDalc.GetIncumbents(GeoCalculations.BuildSquare(interferenceRequest.Location.ToLocation(), new Distance(Convert.ToDouble(interferenceRequest.Location.Point.SemiMajorAxis), DistanceUnit.Meter)));
                if (whiteSpaceDevices != null && whiteSpaceDevices.Count() > 0)
                {
                    for (int whiteSpaceDeviceIndex = 0; whiteSpaceDeviceIndex < whiteSpaceDevices.Count(); whiteSpaceDeviceIndex++)
                    {
                        Location incumbentsLocation = new Location();
                        incumbentsLocation.Latitude = whiteSpaceDevices[whiteSpaceDeviceIndex].Latitude;
                        incumbentsLocation.Longitude = whiteSpaceDevices[whiteSpaceDeviceIndex].Longitude;

                        UsedSpectrum wsd = new UsedSpectrum();

                        // Get distance of WSD device from victim
                        var distance = GeoCalculations.GetDistance(incumbentsLocation, interferenceRequest.Location.ToLocation());

                        // Check if WSD device comes in circular region of victim
                        if (distance.Value <= interferenceRequest.Location.Point.SemiMajorAxis)
                        {
                            List<string> requiredColumns = new List<string> { "ChannelUsageParameters", "AvaialableSpectrumSchedule", "LocationInfo", "DeviceInfo", "EventStartTime", "EventStopTime" };
                            Dictionary<string, string> filters = new Dictionary<string, string>();
                            filters.Add("RowKey", whiteSpaceDevices[whiteSpaceDeviceIndex].RowKey);

                            var result = this.CommonDalc.FetchEntity<DynamicTableEntity>(spectrumUsageNotifyTableName, filters, requiredColumns).FirstOrDefault();
                            SpectrumSchedule avaialableSpectrumSchedule = null;
                            Spectrum usedSpectrum = null;
                            StringBuilder usedChannels = new StringBuilder();

                            if (result != null)
                            {
                                string avaialableSpectrumScheduleData = result.Properties["AvaialableSpectrumSchedule"].StringValue;
                                string channelUsageParametersData = result.Properties["ChannelUsageParameters"].StringValue;
                                string deviceInfo = result.Properties["DeviceInfo"].StringValue;
                                string locationInfo = result.Properties["LocationInfo"].StringValue;
                                string startTime = result.Properties["EventStartTime"].StringValue;
                                string stopTime = result.Properties["EventStopTime"].StringValue;


                                if (!string.IsNullOrEmpty(avaialableSpectrumScheduleData))
                                {
                                    avaialableSpectrumSchedule = JsonSerialization.DeserializeString<SpectrumSchedule>(avaialableSpectrumScheduleData);
                                }

                                if (!string.IsNullOrEmpty(channelUsageParametersData))
                                {
                                    usedSpectrum = JsonSerialization.DeserializeString<Spectrum>(channelUsageParametersData);
                                }

                                if (!string.IsNullOrEmpty(deviceInfo))
                                {
                                    wsd.DeviceDescriptor = JsonSerialization.DeserializeString<DeviceDescriptor>(deviceInfo);
                                }

                                if (!string.IsNullOrEmpty(locationInfo))
                                {
                                    GeoLocation location = JsonSerialization.DeserializeString<GeoLocation>(locationInfo);
                                    wsd.Location = new Location
                                    {
                                        Latitude = Convert.ToDouble(location.Point.Center.Latitude),
                                        Longitude = Convert.ToDouble(location.Point.Center.Longitude),
                                        SemiMajorAxis = Convert.ToDouble(location.Point.SemiMajorAxis),
                                        SemiMinorAxis = Convert.ToDouble(location.Point.SemiMajorAxis)
                                    };
                                }

                                if (!string.IsNullOrEmpty(startTime) && !string.IsNullOrEmpty(stopTime))
                                {
                                    wsd.EventTime = new EventTime
                                    {
                                        StartTime = startTime,
                                        StopTime = stopTime
                                    };
                                }

                                if (usedSpectrum != null && avaialableSpectrumSchedule != null)
                                {
                                    wsd.Spectra = usedSpectrum;
                                    Spectrum requiredinAvaialbleSpectrum = avaialableSpectrumSchedule.Spectra.Where(x => x.ResolutionBwHz == usedSpectrum.ResolutionBwHz).FirstOrDefault();

                                    if (requiredinAvaialbleSpectrum != null)
                                    {
                                        string[] channels = usedSpectrum.Profiles.Select(x => x.ChannelId).ToArray();
                                        usedChannels.Append(string.Join(",", channels));

                                        // extract the CSV file with the values
                                        this.ExportToCSVfile(wsd, interferenceRequest, requiredinAvaialbleSpectrum.Profiles, usedChannels.ToString());
                                    }
                                }
                            }
                        }
                    }

                    // Send CSV in an email
                    this.Email_send(this.emailAddress);
                    // End Log transaction
                    this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);

                    return resp;
                }

                return 200;
            }
            catch (Exception e)
            {
                // Close the stream writer
                if (this.sw != null)
                {
                    this.sw.Close();
                }

                // Log transaction failure
                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                return -32000;
            }
        }

        /// <summary>
        /// Send emails
        /// </summary>
        /// <param name="email">The email address.</param>
        public void Email_send(string email)
        {
            MailMessage mail = new MailMessage();
            SmtpClient smtpServer = new SmtpClient(Utils.Configuration[Constants.HostValue]);
            mail.From = new MailAddress(Utils.Configuration[Constants.FromAddress]);
            mail.To.Add(email);
            mail.Subject = Utils.Configuration[Constants.Subject];
            mail.Body = Utils.Configuration[Constants.Body];
            System.Net.Mail.Attachment attachment;
            attachment = new System.Net.Mail.Attachment(this.csvPath);
            mail.Attachments.Add(attachment);
            smtpServer.Port = Utils.Configuration[Constants.PortValue].ToInt32();
            smtpServer.Credentials = new System.Net.NetworkCredential(Utils.Configuration[Constants.UserName], Utils.Configuration[Constants.Password]);
            smtpServer.EnableSsl = true;
            smtpServer.Send(mail);
        }

        #endregion IDriverPaws methods

        /// <summary>
        /// Extracts CSV file
        /// </summary>
        /// <param name="device">The device.</param>
        /// <param name="interferenceRequest">Interference Request information</param>
        private void ExportToCSVfile(UsedSpectrum device, IInterferenceQueryRequest interferenceRequest, SpectrumProfile[] availableSpectrumProfiles, string usedChannels)
        {
            // Creates the CSV file as a stream, using the given encoding.
            List<string> columns = new List<string>();
            columns.Add("Unique_ID");
            columns.Add("Device_Type");
            columns.Add("Device_Category");
            columns.Add("Device_Class");
            columns.Add("Device_TechID");
            columns.Add("Location_X");
            columns.Add("Location_Y");
            columns.Add("Location Uncertainty");
            columns.Add("Distance");
            columns.Add("Channel_BW");
            columns.Add("Total_BS");
            columns.Add("Start_Time");
            columns.Add("End_Time");
            columns.Add("Location_Validity");
            columns.Add("Update_Timer");
            columns.Add("Channel21_P0");
            columns.Add("Channel21_P1");
            columns.Add("Channel22_P0");
            columns.Add("Channel22_P1");
            columns.Add("Channel23_P0");
            columns.Add("Channel23_P1");
            columns.Add("Channel24_P0");
            columns.Add("Channel24_P1");
            columns.Add("Channel25_P0");
            columns.Add("Channel25_P1");
            columns.Add("Channel26_P0");
            columns.Add("Channel26_P1");
            columns.Add("Channel27_P0");
            columns.Add("Channel27_P1");
            columns.Add("Channel28_P0");
            columns.Add("Channel28_P1");
            columns.Add("Channel29_P0");
            columns.Add("Channel29_P1");
            columns.Add("Channel30_P0");
            columns.Add("Channel30_P1");
            columns.Add("Channel31_P0");
            columns.Add("Channel31_P1");
            columns.Add("Channel32_P0");
            columns.Add("Channel32_P1");
            columns.Add("Channel33_P0");
            columns.Add("Channel33_P1");
            columns.Add("Channel34_P0");
            columns.Add("Channel34_P1");
            columns.Add("Channel35_P0");
            columns.Add("Channel35_P1");
            columns.Add("Channel36_P0");
            columns.Add("Channel36_P1");
            columns.Add("Channel37_P0");
            columns.Add("Channel37_P1");
            columns.Add("Channel38_P0");
            columns.Add("Channel38_P1");
            columns.Add("Channel39_P0");
            columns.Add("Channel39_P1");
            columns.Add("Channel40_P0");
            columns.Add("Channel40_P1");
            columns.Add("Channel41_P0");
            columns.Add("Channel41_P1");
            columns.Add("Channel42_P0");
            columns.Add("Channel42_P1");
            columns.Add("Channel43_P0");
            columns.Add("Channel43_P1");
            columns.Add("Channel44_P0");
            columns.Add("Channel44_P1");
            columns.Add("Channel45_P0");
            columns.Add("Channel45_P1");
            columns.Add("Channel46_P0");
            columns.Add("Channel46_P1");
            columns.Add("Channel47_P0");
            columns.Add("Channel47_P1");
            columns.Add("Channel48_P0");
            columns.Add("Channel48_P1");
            columns.Add("Channel49_P0");
            columns.Add("Channel49_P1");
            columns.Add("Channel50_P0");
            columns.Add("Channel50_P1");
            columns.Add("Channel51_P0");
            columns.Add("Channel51_P1");
            columns.Add("Channel52_P0");
            columns.Add("Channel52_P1");
            columns.Add("Channel53_P0");
            columns.Add("Channel53_P1");
            columns.Add("Channel54_P0");
            columns.Add("Channel54_P1");
            columns.Add("Channel55_P0");
            columns.Add("Channel55_P1");
            columns.Add("Channel56_P0");
            columns.Add("Channel56_P1");
            columns.Add("Channel57_P0");
            columns.Add("Channel57_P1");
            columns.Add("Channel58_P0");
            columns.Add("Channel58_P1");
            columns.Add("Channel59_P0");
            columns.Add("Channel59_P1");
            columns.Add("Channel60_P0");
            columns.Add("Channel60_P1");
            columns.Add("WSD_operational_Channel");

            // represents a full row
            StringBuilder strRow;

            // Path to save the CSV that is created
            this.csvPath = Utils.GetFilePathForConfigKey(Constants.CSVFileName, DateTime.Now.ToString("yyyyMMddHHmmss", DateTimeFormatInfo.InvariantInfo) + ".csv");
            ////this.csvPath = Utils.Configuration[Constants.CSVFilePath] + Utils.Configuration[Constants.CSVFileName] + DateTime.Now.ToString("yyyyMMddHHmmss", DateTimeFormatInfo.InvariantInfo) + ".csv";

            // Traverse the datase CSV file to
            using (this.sw = new StreamWriter(this.csvPath, false, Encoding.Unicode))
            {
                this.sw.WriteLine(this.ColumnNames(columns, "\t"));

                // Reads the rows one by one and transfers them to a string with the given separator character and writes it to the file.
                strRow = new StringBuilder();
                strRow.Append(string.Empty);
                strRow.Append(device.DeviceDescriptor.SerialNumber + device.DeviceDescriptor.ModelId + device.DeviceDescriptor.ManufacturerId);
                strRow.Append("\t");
                strRow.Append(device.DeviceDescriptor.EtsiEnDeviceType);
                strRow.Append("\t");
                strRow.Append(device.DeviceDescriptor.EtsiDeviceCategory);
                strRow.Append("\t");
                strRow.Append(device.DeviceDescriptor.EtsiEnDeviceEmissionsClass);
                strRow.Append("\t");
                strRow.Append(device.DeviceDescriptor.EtsiEnTechnologyId);
                strRow.Append("\t");
                strRow.Append(device.Location.Longitude);
                strRow.Append("\t");
                strRow.Append(device.Location.Latitude);
                strRow.Append("\t");
                strRow.Append(Math.Sqrt(Convert.ToDouble(device.Location.ToGeoLocation().Point.SemiMajorAxis * device.Location.ToGeoLocation().Point.SemiMajorAxis) + Convert.ToDouble(device.Location.ToGeoLocation().Point.SemiMinorAxis * device.Location.ToGeoLocation().Point.SemiMinorAxis)));
                strRow.Append("\t");
                var distance = GeoCalculations.GetDistance(device.Location, interferenceRequest.Location.ToLocation());
                strRow.Append(distance.Value);
                strRow.Append("\t");
                strRow.Append(device.Spectra.ResolutionBwHz);
                strRow.Append("\t");
                strRow.Append(8);
                strRow.Append("\t");
                strRow.Append(device.EventTime.StartTime);
                strRow.Append("\t");
                strRow.Append(device.EventTime.StopTime);
                strRow.Append("\t");
                strRow.Append(50);
                strRow.Append("\t");
                strRow.Append(15);
                strRow.Append("\t");

                //// The WSDB shall use the reported horizontal location and location uncertainty of the Master WSD to define a geographical area within which the WSD might be located (the area of potential locations).
                //// The 100 metres x 100 metres pixels which totally or partially overlap with the area of potential locations will be designated as WSD candidate pixels.
                //// For each candidate pixel and each channel in the list of available DTT channels, the WSDB shall look up (from datasets provided by Ofcom) the maximum permitted in-block EIRP in dBm/(8 MHz).
                //// Since we do not have the uncertainty data available and we need more time to analyse how to create the area of potential location and map it over the NGR grid, this is stubbed out of this iteration.
                OSGLocation eastingnorthings = device.Location.ToEastingNorthing();

                int eastingnorthingsKey = (int)eastingnorthings.Easting;
                int eastingnorthingsValue = (int)eastingnorthings.Northing;

                string[] Channel_P0 = new string[Constants.PmseTotalChannels];

                for (int i = Constants.PmseStartChannel; i <= Constants.PmseEndChannel; i++)
                {
                    var profile = availableSpectrumProfiles.FirstOrDefault(obj => Convert.ToInt16(obj.ChannelId) == i);
                    if (profile == null)
                    {
                        Channel_P0[i-21] = "N/A";
                    }
                    else
                    {
                        Channel_P0[i-21] = profile.DBm.ToString();
                    }
                }

                List<int> valuesP1 = new List<int>();
                StringBuilder str = null;
                List<OSGLocation> coordinatePairsForWSD = GeoCalculations.GenerateOverlappingCoordinatesOf100Mtrs(eastingnorthings.Easting, eastingnorthings.Northing, device.Location.SemiMajorAxis, device.Location.SemiMinorAxis, str);

                List<DTTQueryParams> queryParameters = new List<DTTQueryParams>();
                var dttSourceTable = this.GetDttTableName(Conversion.ConvertUsageSpectrumToIncumbent(device));
                List<DttData> dttValues = this.IncumbentDalc.GetDTTDatasetValues(dttSourceTable, coordinatePairsForWSD);

                for (int i = 0; i < dttValues.Count; i++)
                {
                    this.MapDttDatasetValues(valuesP1, dttValues[i].DataValues);
                }

                for (int i = 0; i < Constants.PmseTotalChannels; i++)
                {
                    strRow.Append(Channel_P0[i]);
                    strRow.Append("\t");

                    if (valuesP1[i] == 0)
                    {
                        Channel_P0[i] = "N/A";
                    }
                    else
                    {
                        strRow.Append(valuesP1[i].ToString());
                    }
                    strRow.Append("\t");
                }

                strRow.Append(usedChannels);
                strRow.Append("\t");

                this.sw.WriteLine(strRow.ToString());
            }

            this.sw.Close();
        }

        /// <summary>
        /// Maps the DTT dataset values.
        /// </summary>
        /// <param name="existingPS0Values">The existing p s0 values.</param>
        /// <param name="dttValuesFromDataset">The DTT values from dataset.</param>
        private void MapDttDatasetValues(List<int> existingPS0Values, int[] dttValuesFromDataset)
        {
            if (dttValuesFromDataset.Length > 40)
            {
                if (existingPS0Values.Count == 0)
                {
                    existingPS0Values.AddRange(dttValuesFromDataset.Skip(2));
                }
                else
                {
                    for (int i = 0; i < existingPS0Values.Count; i++)
                    {
                        existingPS0Values[i] = Math.Min(existingPS0Values[i], dttValuesFromDataset[i + 2]);
                    }
                }
            }
            else
            {
                if (existingPS0Values.Count == 0)
                {
                    existingPS0Values.AddRange(dttValuesFromDataset);
                }
                else
                {
                    for (int i = 0; i < existingPS0Values.Count; i++)
                    {
                        existingPS0Values[i] = Math.Min(existingPS0Values[i], dttValuesFromDataset[i]);
                    }
                }
            }
        }

        /// <summary>
        /// Gets the name of the DTT table.
        /// </summary>
        /// <param name="wsdInfo">The WSD information.</param>
        /// <returns>returns System.String.</returns>
        private string GetDttTableName(Incumbent wsdInfo)
        {
            double heightWSD = wsdInfo.Height;
            string height = string.Empty;
            string tableName = null;

            string emissionClass = Utils.Configuration[Constants.ConfigSettingDefaultDeviceEmissionClassDeviceParam];

            if (wsdInfo.EmissionClass.ToInt32() > 0)
            {
                emissionClass = wsdInfo.EmissionClass;
            }

            if (heightWSD == 0)
            {
                if (wsdInfo.IncumbentType == IncumbentType.TypeA)
                {
                    height = "DDDD";
                }
                else
                {
                    height = "0150";
                }
            }
            else
            {
                if (heightWSD < 2.5)
                {
                    height = "0150";
                }
                else if (heightWSD >= 2.5 && heightWSD < 7.5)
                {
                    height = "0500";
                }
                else if (heightWSD >= 7.5 && heightWSD < 20)
                {
                    height = "1000";
                }
                else if (heightWSD >= 20)
                {
                    height = "3000";
                }
            }

            tableName = string.Concat(Utils.CurrentRegionName, Utils.Configuration.CurrentRegionId, "p", emissionClass, "h", height);
            ////partitionKey = this.GetPartitionKeyForDTT(height, emissionClass, easting, northing);
            return tableName;
        }

        /// <summary>
        /// Column Names
        /// </summary>
        /// <param name="columns">Columns details</param>
        /// <param name="delimiter">Delimiter information</param> 
        /// <returns>Column Name values</returns>
        private string ColumnNames(List<string> columns, string delimiter)
        {
            string strOut = string.Empty;
            delimiter = "\t";
            for (int i = 0; i < columns.Count; i++)
            {
                strOut += columns[i];
                if (i < columns.Count - 1)
                {
                    strOut += delimiter;
                }
            }

            return strOut;
        }
    }
}
