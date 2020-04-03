// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.RegionCalculation
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Threading.Tasks;
    using Common.CacheHelpers;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.Whitespace.RegionCalculation.Propagation;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>Performs contour calculations according to FCC rules.</summary>
    public class FccCalculation : IRegionCalculation
    {
        /// <summary>Gets or sets the incumbent DALC.</summary>
        /// <value>The DALC incumbent.</value>
        [Dependency]
        public IDalcIncumbent DalcIncumbent { get; set; }

        /// <summary>
        /// Gets or sets the common DALC.
        /// </summary>
        /// <value>The common DALC.</value>
        [Dependency]
        public IDalcCommon CommonDalc { get; set; }

        /// <summary>Gets or sets the terrain elevation.</summary>
        /// <value>The terrain elevation.</value>
        [Dependency]
        public ITerrainElevation TerrainElevation { get; set; }

        /// <summary>
        /// Gets or sets the logger.
        /// </summary>
        /// <value>The logger.</value>
        [Dependency]
        public ILogger Logger { get; set; }

        /// <summary>Calculates the contour of the specified incumbent.</summary>
        /// <param name="incumbent">Incumbent that is to be calculated.</param>
        /// <returns>Returns the contour of the calculations.</returns>
        public Contour CalculateContour(Incumbent incumbent)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.GenericMessage, string.Format("Calcuating Contour for Incumbent ->{0}, RowKey -> {1}", incumbent.Location, incumbent.RowKey));

            //// Only for contour analysis purpose
            bool buildContourDetails = incumbent.BuildContourItems;
            List<ContourPointItem> contourDetails = null;

            if (buildContourDetails)
            {
                contourDetails = new List<ContourPointItem>();
            }

            List<Location> contourPoints = new List<Location>();

            var radialHat = this.CalculateRadialHAT(incumbent);

            if (incumbent.AntennaPatternData != null)
            {
                incumbent.AntennaPattern = JsonSerialization.DeserializeString<double[]>(incumbent.AntennaPatternData);
            }
            else
            {
                incumbent.AntennaPattern = this.GetDefaultAntennaPattern();
            }

            double[] newPatterns = null;
            if ((incumbent.AntennaType == "C" || incumbent.AntennaType == "D") && incumbent.AntRotation > 0)
            {
                incumbent.AntennaPattern.Patch();
                newPatterns = new double[360];
                var actualAntennaRotation = (int)incumbent.AntRotation;
                for (int rotation = 0; rotation < 360; rotation++, actualAntennaRotation++)
                {
                    if (actualAntennaRotation >= 360)
                    {
                        actualAntennaRotation -= 360;
                    }

                    newPatterns[actualAntennaRotation] = incumbent.AntennaPattern[rotation];
                }
            }
            else
            {
                newPatterns = incumbent.AntennaPattern.Patch();
            }

            foreach (var azimuthRadialHaat in radialHat.AzimuthRadialHaats)
            {
                var antennaHeight = 30.0f;

                // if height is less than 30 then set it to 30m
                // if height is more than 1600 than set it to 1600m
                if (incumbent.Height > 0)
                {
                    antennaHeight = (float)(incumbent.Height - azimuthRadialHaat.Value);
                    if (antennaHeight < 30)
                    {
                        antennaHeight = 30;
                    }
                    else if (antennaHeight > 1600)
                    {
                        antennaHeight = 1600;
                    }
                }

                var antennaFactor = newPatterns[azimuthRadialHaat.Key];
                if (antennaFactor < .001f)
                {
                    antennaFactor = .001f;
                }

                var erpAtAzimuth = incumbent.TxPower * Math.Pow(antennaFactor, 2);

                ////var finalErp = 0.0f;
                ////if (erpAtAzimuth < .001)
                ////{
                ////    finalErp = .001;
                ////}

                double distance = NativeMethods.GetDistance((float)erpAtAzimuth, antennaHeight, incumbent.Channel, (float)Conversion.ChannelToContourDb(incumbent.Channel, incumbent.VsdService), Conversion.IsDigitalService(incumbent.VsdService));
                var contourPoint = GeoCalculations.GetLocationTowardsBearing(incumbent.Location, new Distance(distance, DistanceUnit.KM), azimuthRadialHaat.Key);
                contourPoints.Add(contourPoint);

                //////Only for contour analysis purpose
                if (buildContourDetails)
                {
                    var contourPointItem = new ContourPointItem()
                                           {
                                               Azimuth = azimuthRadialHaat.Key,
                                               Distance = distance,
                                               ERP = erpAtAzimuth,
                                               HAAT = incumbent.Height - azimuthRadialHaat.Value,
                                               Latitude = contourPoint.Latitude,
                                               Longitude = contourPoint.Longitude
                                           };

                    contourDetails.Add(contourPointItem);
                }
            }

            Contour contour = new Contour();
            contour.ContourPoints = contourPoints;
            contour.Point = incumbent.Location;

            if (buildContourDetails)
            {
                contour.ContourPointItems = contourDetails;
            }

            try
            {
                // Adjusting the incumbent that lies outside of the US.
                this.AdjustIncumbentOutsideUS(contour);
            }
            catch (Exception ex)
            {
                string errorMessage = string.Format("Incumbent Type:{0}, Callsign:{1}, Channel:{2}, Latitude:{3}, Longitude:{4} {5} Error Details:{6}", incumbent.IncumbentType, incumbent.CallSign, incumbent.Channel, incumbent.Latitude, incumbent.Longitude, Environment.NewLine, ex.ToString());

                // If something goes wrong with incumbent adjustment, then log the error message and continue the rest of the operation.
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.GenericMessage, errorMessage);
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.GenericMessage, string.Format("Calculated Contour for Incumbent -> Location : {0}, RowKey eq '{1}' and CallSign eq '{2}'", incumbent.Location, incumbent.RowKey, incumbent.CallSign));

            return contour;
        }

        /// <summary>Calculates the Hat of the incumbent.</summary>
        /// <param name="incumbent">The incumbent that is to be calculated.</param>
        /// <returns>Returns the Hat</returns>
        public RadialHAT CalculateRadialHAT(Incumbent incumbent)
        {
            RadialHAT radialHat = new RadialHAT();
            radialHat.AzimuthRadialHaats = new Dictionary<int, double>();
            Location sourceLocation = incumbent.Location;

            bool logRadialHatToFile = ConfigurationManager.AppSettings.AllKeys.Contains("LogRadialHAT") && ConfigurationManager.AppSettings["LogRadialHAT"].ToInt32() == 1;
            StringBuilder loggerbuilder = null;
            if (logRadialHatToFile)
            {
                loggerbuilder = new StringBuilder();
            }

            // calculate for all azimuth from 0 - 360 at 1 degree increment
            for (int azimuth = 0; azimuth < 360; azimuth += 1)
            {
                var interpolatedLocations = this.GetInterpolatedLocations(sourceLocation, azimuth);

                if (logRadialHatToFile)
                {
                    loggerbuilder.AppendFormat("Radial Point information for location ({0}, {1})", sourceLocation.Latitude, sourceLocation.Longitude);
                    loggerbuilder.AppendLine();
                    loggerbuilder.AppendFormat("Radial Angle: {0} degrees ", azimuth);
                    loggerbuilder.AppendLine();
                }

                List<double> haats = new List<double>();
                for (int locationIndex = 0; locationIndex < interpolatedLocations.Count; locationIndex++)
                {
                    Location targetLocation = interpolatedLocations[locationIndex];
                    if (logRadialHatToFile)
                    {
                        loggerbuilder.AppendFormat("Radial Point #{0}:({1} km), @({2}, {3})", locationIndex + 1, 3.1 + ((locationIndex + 1) * 0.1), targetLocation.Latitude, targetLocation.Longitude);
                        loggerbuilder.AppendLine();
                    }

                    FortranDoubleArray d_cor_lat;
                    FortranDoubleArray d_cor_lon;

                    this.Calculate3SecCorners(targetLocation, out d_cor_lat, out d_cor_lon);

                    // Calculate elevations at 4 corners
                    var corner_elevations = new FortranDoubleArray(4);
                    for (int i = 1; i < 5; i++)
                    {
                        var elevationLocation = new Location(d_cor_lat[i], d_cor_lon[i]);
                        corner_elevations[i] = this.TerrainElevation.CalculateElevation(elevationLocation).InMeter();
                        if (logRadialHatToFile)
                        {
                            loggerbuilder.AppendFormat("Corner {0}: File:@({1}, {2}), Elevation : {3}", i, elevationLocation.Latitude, elevationLocation.Longitude, corner_elevations[i]);
                            loggerbuilder.AppendLine();
                        }
                    }

                    double elevation = this.InterpolateForElevations(targetLocation, d_cor_lat, d_cor_lon, corner_elevations);
                    if (logRadialHatToFile)
                    {
                        loggerbuilder.AppendFormat("Elevation {0}", elevation);
                        loggerbuilder.AppendLine(Environment.NewLine);
                    }

                    haats.Add(elevation);
                }

                radialHat.AzimuthRadialHaats.Add(azimuth, haats.Average());
                if (logRadialHatToFile)
                {
                    ////loggerbuilder.AppendFormat("Radial HAAT for {0} degree is : {1}", azimuth, radialHat.AzimuthRadialHaats[azimuth]);
                    ////loggerbuilder.AppendLine(Environment.NewLine);

                    //////////////Only for contour analysis purpose
                    ////string currentFolderPath = ConfigurationManager.AppSettings["LogDirectory"] + "\\" + incumbent.CallSign + "\\";
                    ////if (!Directory.Exists(currentFolderPath))
                    ////{
                    ////    Directory.CreateDirectory(currentFolderPath);
                    ////}

                    ////File.WriteAllText(string.Format("{0}\\RadialHAT_{1}.txt", currentFolderPath, azimuth), loggerbuilder.ToString());
                    loggerbuilder.Length = 0;
                }
            }

            return radialHat;
        }

        /// <summary>
        /// Calculates the station HAAT.
        /// </summary>
        /// <param name="sourceLocation">The source location.</param>
        /// <returns>returns RadialHAAT.</returns>
        public RadialHAT CalculateStationHAAT(Location sourceLocation)
        {
            RadialHAT radialHat = new RadialHAT();
            radialHat.AzimuthRadialHaats = new Dictionary<int, double>();

            // calculate for all azimuth from 0 - 360 at 1 degree increment
            for (int azimuth = 0; azimuth < 360; azimuth += 45)
            {
                var interpolatedLocations = this.GetInterpolatedLocations(sourceLocation, azimuth);

                List<double> haats = new List<double>();
                for (int locationIndex = 0; locationIndex < interpolatedLocations.Count; locationIndex++)
                {
                    Location targetLocation = interpolatedLocations[locationIndex];

                    FortranDoubleArray d_cor_lat;
                    FortranDoubleArray d_cor_lon;

                    this.Calculate3SecCorners(targetLocation, out d_cor_lat, out d_cor_lon);

                    // Calculate elevations at 4 corners
                    var corner_elevations = new FortranDoubleArray(4);
                    for (int i = 1; i < 5; i++)
                    {
                        var elevationLocation = new Location(d_cor_lat[i], d_cor_lon[i]);
                        corner_elevations[i] = this.TerrainElevation.CalculateElevation(elevationLocation).InMeter();
                    }

                    double elevation = this.InterpolateForElevations(targetLocation, d_cor_lat, d_cor_lon, corner_elevations);
                    haats.Add(elevation);
                }

                radialHat.AzimuthRadialHaats.Add(azimuth, haats.Average());
            }

            return radialHat;
        }

        /// <summary>Determines the channel availability for each channel at the specified position.</summary>
        /// <param name="position">Location that is determine channel availability.</param>
        /// <returns>Returns all of the channels and their availability status.</returns>
        public ChannelInfo[] GetChannelAvailabilty(Position position)
        {
            throw new NotImplementedException();
        }

        /// <summary>Returns only the free channels at the specified location.</summary>
        /// <param name="wsdInfo">Incumbent Information that is to determine channel availability.</param>
        /// <returns>Returns only the free channels.</returns>
        public GetChannelsResponse GetFreeChannels(Incumbent wsdInfo)
        {
            const string LogMethodName = "FccCalculation.GetFreeChannels";

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegionCalculationGenericMessage, "Enter " + LogMethodName);

            try
            {
                List<int> blockedChannels = new List<int>();
                List<int> reducedPowerChannels = new List<int>();
                List<int> adjacentChannels = new List<int>();

                this.ExcludeRegions(wsdInfo, blockedChannels);

                // check for point inside radio astronomy if inside block all channels
                if (this.CheckForRadioAstronomy(wsdInfo.Location))
                {
                    for (int i = 2; i <= 51; i++)
                    {
                        blockedChannels.Add(i);
                    }
                }
                else
                {
                    // prohibit channel 3,4 and 37 if Fixed Device Type
                    if (wsdInfo.IncumbentType == IncumbentType.Fixed)
                    {
                        this.AddBlockedChannel(blockedChannels, 3);
                        this.AddBlockedChannel(blockedChannels, 4);
                        this.AddBlockedChannel(blockedChannels, 37);
                    }
                    else if (wsdInfo.IncumbentType == IncumbentType.Mode_1 || wsdInfo.IncumbentType == IncumbentType.Mode_2)
                    {
                        // prohibit channel 37, and 2-21 if personal/portable device
                        this.AddBlockedChannel(blockedChannels, 37);
                        for (int i = 2; i < 21; i++)
                        {
                            this.AddBlockedChannel(blockedChannels, i);
                        }
                    }
                    else if (wsdInfo.IncumbentType == IncumbentType.TBAS || wsdInfo.IncumbentType == IncumbentType.LPAux || wsdInfo.IncumbentType == IncumbentType.UnlicensedLPAux)
                    {
                        // allowed channels 2, 5-36, 38-51
                        this.AddBlockedChannel(blockedChannels, 3);
                        this.AddBlockedChannel(blockedChannels, 4);
                        this.AddBlockedChannel(blockedChannels, 37);
                    }

                    if (wsdInfo.IncumbentType != IncumbentType.TBAS)
                    {
                        SquareArea searchArea = GeoCalculations.BuildSquare(wsdInfo.Location, new Distance(Utils.Configuration[Constants.TvStationSearchDistance].ToDouble(), DistanceUnit.KM));
                        var combinedData = this.DalcIncumbent.GetCombinedIncumbents(Utils.GetRegionalTableName(Constants.MergedCDBSDataTableName), searchArea);

                        if (wsdInfo.IncumbentType == IncumbentType.LPAux || wsdInfo.IncumbentType == IncumbentType.UnlicensedLPAux)
                        {
                            this.FilterTVStationsForLpAux(wsdInfo, blockedChannels, reducedPowerChannels, adjacentChannels);
                        }
                        else
                        {
                            this.FilterTVStations(wsdInfo, blockedChannels, reducedPowerChannels, adjacentChannels);
                        }

                        this.FilterPLCMRSStations(wsdInfo, blockedChannels, null, combinedData);

                        this.FilterTBandProtection(wsdInfo, blockedChannels);

                        if (wsdInfo.IncumbentType != IncumbentType.LPAux && wsdInfo.IncumbentType != IncumbentType.UnlicensedLPAux)
                        {
                            this.FilterOffShoreRadioService(wsdInfo.Location, blockedChannels);

                            this.FilterUsedSpectrum(wsdInfo, blockedChannels);

                            this.FilterBroadcastAuxillaryStations(wsdInfo, blockedChannels, null, combinedData);

                            this.FilterMVPDSites(wsdInfo, blockedChannels);

                            this.FilterTempBASSites(wsdInfo, blockedChannels);

                            this.FilterTVReceiveSites(wsdInfo, blockedChannels);

                            this.FilterLPAux(wsdInfo, blockedChannels);

                            this.FilterTVTranslators(wsdInfo, blockedChannels, null, combinedData);
                        }
                    }
                }

                List<ChannelInfo> freeChannels = new List<ChannelInfo>();

                if (wsdInfo.IncumbentType != IncumbentType.TBAS)
                {
                    this.AddWirelessMicrophoneChannels(blockedChannels, adjacentChannels);
                }

                int defaultMaxPowerDBm = 0;

                if (wsdInfo.IncumbentType == IncumbentType.Fixed)
                {
                    // According to subpart h 15.709 (a) a fixed TVBD has a maximum power of 1 watt (30 DBm).
                    defaultMaxPowerDBm = 30;
                }

                // return final channels
                for (int i = 2; i <= 51; i++)
                {
                    // if channel is in already blocked list skip
                    if (blockedChannels.Contains(i))
                    {
                        continue;
                    }

                    if (wsdInfo.IncumbentType == IncumbentType.Mode_1 || wsdInfo.IncumbentType == IncumbentType.Mode_2)
                    {
                        // According to subpart h 15.709 (a) (2) personal/portable devices have a maximum power of 100 milliwats (20 dBm) or 40 milliwats (16 dBm)
                        if (reducedPowerChannels.Contains(i))
                        {
                            // Reduced since it is adjacent to a TV Tower.
                            defaultMaxPowerDBm = 16;
                        }
                        else
                        {
                            // Set at maximum for a mode 1 or mode 2 device.
                            defaultMaxPowerDBm = 20;
                        }
                    }

                    var minMaxTuple = Conversion.GetMinMaxFreqForChannel(i);
                    ChannelInfo curChannelInfo = new ChannelInfo()
                                                 {
                                                     DeviceType = wsdInfo.IncumbentType.ToString(),
                                                     Bandwidth = 8,
                                                     ChannelId = i,
                                                     StartHz = minMaxTuple.Item1,
                                                     StopHz = minMaxTuple.Item2,
                                                     MaxPowerDBm = defaultMaxPowerDBm
                                                 };

                    freeChannels.Add(curChannelInfo);
                }

                return new GetChannelsResponse()
                       {
                           ChannelsInfo = freeChannels.ToArray()
                       };
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegionCalculationGenericMessage, string.Format("Error in {0}: Message {1}", LogMethodName, ex.ToString()));
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegionCalculationGenericMessage, "Exit " + LogMethodName);

            return null;
        }

        /// <summary>Returns only the devices at the specified location.</summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>Returns the devices.</returns>
        public List<ProtectedDevice> GetDeviceList(Parameters parameters)
        {
            List<int> blockedChannels = new List<int>();
            List<int> reducedPowerChannels = new List<int>();
            List<int> adjacentChannels = new List<int>();

            List<ProtectedDevice> protectedDevices = new List<ProtectedDevice>();

            // check for point inside radio astronomy if inside block all channels
            if (this.CheckForRadioAstronomy(parameters.Location.ToLocation()))
            {
                for (int i = 2; i <= 51; i++)
                {
                    blockedChannels.Add(i);
                }
            }
            else
            {
                // prohibit channel 3,4 and 37 if Fixed Device Type
                if (parameters.IncumbentType.ToLower().ToString() == IncumbentType.Fixed.ToString().ToLower())
                {
                    this.AddBlockedChannel(blockedChannels, 3);
                    this.AddBlockedChannel(blockedChannels, 4);
                    this.AddBlockedChannel(blockedChannels, 37);
                    ProtectedDevice pd = new ProtectedDevice();
                    pd.ProtectedDeviceType = ProtectedDeviceType.LpAuxReserved;
                    pd.Channels = blockedChannels.ToArray();
                    protectedDevices.Add(pd);
                }
                else if (parameters.IncumbentType.ToLower().ToString() == IncumbentType.Mode_1.ToString().ToLower() || parameters.IncumbentType.ToLower().ToString() == IncumbentType.Mode_2.ToString().ToLower())
                {
                    // prohibit channel 37, and 2-21 if personal/portable device
                    this.AddBlockedChannel(blockedChannels, 37);
                    for (int i = 2; i <= 21; i++)
                    {
                        this.AddBlockedChannel(blockedChannels, i);
                        ProtectedDevice pd = new ProtectedDevice();
                        pd.ProtectedDeviceType = ProtectedDeviceType.LpAuxReserved;
                        pd.Channels = blockedChannels.ToArray();
                        protectedDevices.Add(pd);
                    }
                }

                Incumbent wsdInfo = new Incumbent()
                {
                    Location = parameters.Location.ToLocation(),
                    IncumbentType = Conversion.ToIncumbentType(parameters.IncumbentType),
                };

                SquareArea searchArea = GeoCalculations.BuildSquare(wsdInfo.Location, new Distance(Utils.Configuration[Constants.TvStationSearchDistance].ToDouble(), DistanceUnit.KM));
                var combinedData = this.DalcIncumbent.GetCombinedIncumbents(Utils.GetRegionalTableName(Constants.MergedCDBSDataTableName), searchArea);

                var result = Task.Run(() => this.FilterTVStations(wsdInfo, blockedChannels, reducedPowerChannels, adjacentChannels, protectedDevices));

                this.FilterOffShoreRadioService(parameters.Location.ToLocation(), blockedChannels, protectedDevices);

                this.FilterTBandProtection(wsdInfo, blockedChannels, protectedDevices);

                this.FilterUsedSpectrum(wsdInfo, blockedChannels);

                this.FilterBroadcastAuxillaryStations(wsdInfo, blockedChannels, protectedDevices, combinedData);

                this.FilterPLCMRSStations(wsdInfo, blockedChannels, protectedDevices, combinedData);

                this.FilterMVPDSites(wsdInfo, blockedChannels, protectedDevices);

                this.FilterTempBASSites(wsdInfo, blockedChannels, protectedDevices);

                this.FilterLPAux(wsdInfo, blockedChannels, protectedDevices);

                this.FilterTVTranslators(wsdInfo, blockedChannels, protectedDevices, combinedData);

                result.Wait();

                this.AddWirelessMicrophoneChannels(blockedChannels, adjacentChannels, protectedDevices);
            }

            return protectedDevices;
        }

        /// <summary>
        /// Excludes the regions.
        /// </summary>
        /// <param name="incumbentInfo">The incumbent information.</param>
        /// <param name="blockedChannels">The blocked channels.</param>
        private void ExcludeRegions(Incumbent incumbentInfo, List<int> blockedChannels)
        {
            var excludedRegions = this.DalcIncumbent.GetExcludedRegions();

            foreach (var excludedRegion in excludedRegions)
            {
                var locations = JsonSerialization.DeserializeString<GeoLocation[]>(excludedRegion.Properties["Location"].StringValue);
                if (GeoCalculations.IsPointInPolygon(locations.ToLocations(), incumbentInfo.Location))
                {
                    var spectrums = JsonSerialization.DeserializeString<TvSpectrum[]>(excludedRegion.Properties["ChannelList"].StringValue);
                    foreach (var spectrum in spectrums)
                    {
                        this.AddBlockedChannel(blockedChannels, spectrum.Channel.Value);
                    }
                }
            }
        }

        /// <summary>
        /// Checks for offshore radio service.
        /// </summary>
        /// <param name="point">The point.</param>
        /// <param name="blockedChannels">The blocked channels.</param>
        /// <param name="protectedDevices">The protected devices.</param>
        private void FilterOffShoreRadioService(Location point, List<int> blockedChannels, List<ProtectedDevice> protectedDevices = null)
        {
            var offshoreServices = this.CommonDalc.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.OffShoreProtectionTable), null);

            foreach (var astronomyLocation in offshoreServices)
            {
                SquareArea largeArray = new SquareArea(GeoCalculations.GetLocation(astronomyLocation["LatitudeNW"].StringValue, astronomyLocation["LongitudeNW"].StringValue), GeoCalculations.GetLocation(astronomyLocation["LatitudeSE"].StringValue, astronomyLocation["LongitudeSE"].StringValue));
                var channel = astronomyLocation["Channel"].StringValue.ToInt32();
                if (GeoCalculations.IsPointInSquare(largeArray, point))
                {
                    if (protectedDevices != null)
                    {
                        ProtectedDevice offshoreDevice = new ProtectedDevice()
                                                      {
                                                          ProtectedDeviceType = ProtectedDeviceType.OffShoreRadioService,
                                                          Channels = new[] { channel }
                                                      };

                        offshoreDevice.ContourPoints = new List<Location>();
                        offshoreDevice.ContourPoints.Add(GeoCalculations.GetLocation(astronomyLocation["LatitudeNE"].StringValue, astronomyLocation["LongitudeNE"].StringValue));
                        offshoreDevice.ContourPoints.Add(GeoCalculations.GetLocation(astronomyLocation["LatitudeSW"].StringValue, astronomyLocation["LongitudeSW"].StringValue));
                        offshoreDevice.ContourPoints.Add(GeoCalculations.GetLocation(astronomyLocation["LatitudeNW"].StringValue, astronomyLocation["LongitudeNW"].StringValue));
                        offshoreDevice.ContourPoints.Add(GeoCalculations.GetLocation(astronomyLocation["LatitudeSE"].StringValue, astronomyLocation["LongitudeSE"].StringValue));

                        protectedDevices.Add(offshoreDevice);
                    }

                    this.AddBlockedChannel(blockedChannels, channel);
                }
            }
        }

        /// <summary>
        /// Checks for LP-AUX.
        /// </summary>
        /// <param name="incumbentInfo">The incumbent information.</param>
        /// <param name="blockedChannels">The blocked channels.</param>
        /// <param name="protectedDevices">The protected devices.</param>
        private void FilterLPAux(Incumbent incumbentInfo, List<int> blockedChannels, List<ProtectedDevice> protectedDevices = null)
        {
            double searchDistance = 0.0;

            // if portable device then 400 mtrs and if Fixed then 1 km
            if (incumbentInfo.IncumbentType == IncumbentType.Mode_1 || incumbentInfo.IncumbentType == IncumbentType.Mode_2)
            {
                searchDistance = Utils.Configuration[Constants.LpAuxPortableDeviceDistance].ToDouble() * 1000;
            }
            else
            {
                searchDistance = Utils.Configuration[Constants.LpAuxFixedDeviceDistance].ToDouble() * 1000;
            }

            var lpAuxRegistrations = DatabaseCache.ServiceCacheHelper.GetServiceCacheObjects(ServiceCacheObjectType.LPAUX, null) as List<LPAuxRegistration>;

            var validLpauxDevices = from lpauxdevice in lpAuxRegistrations
                                    from lpauxevent in lpauxdevice.Event.Times
                                    where lpauxevent.Start.ToDateTimeOffset() <= DateTime.Now && lpauxevent.End.ToDateTimeOffset() >= DateTime.Now
                                    select lpauxdevice;

            foreach (var incumbent in validLpauxDevices)
            {
                if (GeoCalculations.IsLpauxInCoverage(incumbentInfo.Location, searchDistance, incumbent))
                {
                    if (protectedDevices != null)
                    {
                        ProtectedDevice lpauxDevice = new ProtectedDevice()
                        {
                            ProtectedDeviceType = ProtectedDeviceType.LpAux,
                            Channels = new[] { incumbent.Channel },
                            CallSign = incumbent.CallSign.CallSign
                        };

                        protectedDevices.Add(lpauxDevice);
                    }

                    this.AddBlockedChannel(blockedChannels, incumbent.Channel);
                }
            }
        }

        /// <summary>
        /// Filters the MVPD sites.
        /// </summary>
        /// <param name="wsdInfo">The incumbent information.</param>
        /// <param name="blockedChannels">The blocked channels.</param>
        /// <param name="protectedDevices">The protected devices.</param>
        private void FilterMVPDSites(Incumbent wsdInfo, List<int> blockedChannels, List<ProtectedDevice> protectedDevices = null)
        {
            SquareArea searchArea = GeoCalculations.BuildSquare(wsdInfo.Location, new Distance(Utils.Configuration["MVPDSearchDistance_KM"].ToDouble(), DistanceUnit.KM));

            Distance adjacentChannelKeyHoleDistance = new Distance(Utils.Configuration[Constants.KeyholeOuterAdjChannelDistance].ToDouble(), DistanceUnit.KM);
            Distance sameChannelKeyHoleDistance = new Distance(Utils.Configuration[Constants.KeyholeOuterCoChannelDistance].ToDouble(), DistanceUnit.KM);

            Distance adjacentChannelDistance = new Distance(Utils.Configuration[Constants.KeyholeInnerAdjChannelDistance].ToDouble(), DistanceUnit.KM);
            Distance sameChannelDistance = new Distance(Utils.Configuration[Constants.KeyholeInnerCoChannelDistance].ToDouble(), DistanceUnit.KM);

            var stations = this.DalcIncumbent.GetIncumbents(Utils.GetRegionalTableName(Constants.MVPDRegistrationTableName), searchArea, null);

            foreach (var station in stations)
            {
                var channelsToBeBlocked = new List<int>();

                var wsdToStationDistance = GeoCalculations.GetDistance(wsdInfo.Location, new Location(station.Latitude, station.Longitude));

                //// bearing from trasnmitter to mvpd receiver
                var stationToParentTxBearing = GeoCalculations.CalculateBearing(station.Location, new Location(station.TxLatitude, station.TxLongitude));

                var mvpdChannel = JsonSerialization.DeserializeString<TvSpectrum>(station.MVPDChannel);

                // keyhole calculations
                int keyHoleArcStarting = (int)(stationToParentTxBearing - 30);
                int keyHoleArcEnding = (int)(stationToParentTxBearing + 30);

                if (keyHoleArcStarting < 0)
                {
                    keyHoleArcStarting = 360 - Math.Abs(keyHoleArcStarting);
                }

                if (keyHoleArcEnding > 360)
                {
                    keyHoleArcEnding = keyHoleArcEnding - 360;
                }

                // bearing from mvpd receiver to incumbent in query
                var incumbentBearing = GeoCalculations.CalculateBearing(station.Location, wsdInfo.Location);

                if (wsdToStationDistance.InKm() <= sameChannelDistance.InKm())
                {
                    channelsToBeBlocked.Add(station.ChannelNumber);

                    if (wsdToStationDistance.InKm() <= adjacentChannelDistance.InKm())
                    {
                        channelsToBeBlocked.Add(station.ChannelNumber - 1);
                        channelsToBeBlocked.Add(station.ChannelNumber + 1);
                    }

                    this.AddBlockedChannel(blockedChannels, channelsToBeBlocked.ToArray());
                }
                else
                {
                    if (station.BroadcastStationContour != null && !string.IsNullOrWhiteSpace(station.BroadcastStationContour))
                    {
                        var contour = JsonSerialization.DeserializeString<Contour>(station.BroadcastStationContour);

                        List<Location> polygonPoints = contour.ContourPoints;

                        if (incumbentBearing >= keyHoleArcStarting && incumbentBearing <= keyHoleArcEnding)
                        {
                            var stationToContourDistance = GeoCalculations.GetDistance(station.Location, polygonPoints[(int)incumbentBearing]);
                            if (stationToContourDistance.InKm() <= sameChannelKeyHoleDistance.InKm())
                            {
                                channelsToBeBlocked.Add(station.ChannelNumber);

                                if (stationToContourDistance.InKm() <= adjacentChannelKeyHoleDistance.InKm())
                                {
                                    channelsToBeBlocked.Add(station.ChannelNumber - 1);
                                    channelsToBeBlocked.Add(station.ChannelNumber + 1);
                                }

                                this.AddBlockedChannel(blockedChannels, channelsToBeBlocked.ToArray());
                            }
                        }
                    }
                }

                if (protectedDevices != null)
                {
                    ProtectedDevice mvpdSite = new ProtectedDevice()
                    {
                        ProtectedDeviceType = ProtectedDeviceType.MVPD,
                        Location = station.Location,
                        Channels = channelsToBeBlocked.ToArray(),
                        CallSign = mvpdChannel.CallSign
                    };

                    mvpdSite.ContourPoints = this.CalculateRadialContour(station.Location, sameChannelDistance);

                    mvpdSite.KeyholeArcBearingStart = (int)keyHoleArcStarting;
                    mvpdSite.KeyholeArcBearingEnd = (int)keyHoleArcEnding;

                    var keyholeContourPoints = this.CalculateRadialContour(station.Location, sameChannelKeyHoleDistance);
                    mvpdSite.KeyholeArcPoints = new List<Location>();

                    for (int i = mvpdSite.KeyholeArcBearingStart; i < mvpdSite.KeyholeArcBearingEnd; i++)
                    {
                        mvpdSite.KeyholeArcPoints.Add(keyholeContourPoints[i]);
                    }

                    protectedDevices.Add(mvpdSite);
                }
            }
        }

        /// <summary>
        /// Calculates the key hole contour.
        /// </summary>
        /// <param name="mvpdRegistration">The MVPD registration.</param>
        /// <param name="station">The station.</param>
        /// <returns>returns KeyHoleContourItem.</returns>
        private KeyHoleContourItem CalculateKeyHoleContour(MVPDRegistration mvpdRegistration, Incumbent station)
        {
            KeyHoleContourItem keyHoleContourItem = new KeyHoleContourItem();

            ////var wsdToStationDistance = GeoCalculations.GetDistance(mvpdRegistration.Location, new Location(station.Latitude, station.Longitude));
            var stationToParentTxBearing = GeoCalculations.CalculateBearing(mvpdRegistration.Location, new Location(mvpdRegistration.TxLatitude, mvpdRegistration.TxLongitude));

            // keyhole calculations
            keyHoleContourItem.KeyHoleArcStarting = (int)(stationToParentTxBearing - 30);
            keyHoleContourItem.KeyHoleArcEnding = (int)(stationToParentTxBearing + 30);

            keyHoleContourItem.AdjacentChannelLocations = this.CalculateRadialContour(mvpdRegistration.Location, new Distance(2, DistanceUnit.KM));
            keyHoleContourItem.CoChannelLocations = this.CalculateRadialContour(mvpdRegistration.Location, new Distance(8, DistanceUnit.KM));

            var incumbentBearing = GeoCalculations.CalculateBearing(station.Location, mvpdRegistration.Location);

            if (station.BroadcastStationContour != null && !string.IsNullOrWhiteSpace(station.BroadcastStationContour))
            {
                var contour = JsonSerialization.DeserializeString<Contour>(station.BroadcastStationContour);
                var stationToContourDistance = GeoCalculations.GetDistance(station.Location, contour.ContourPoints[(int)incumbentBearing]);
                keyHoleContourItem.KeyHoleCoChannelLocations = this.CalculateRadialContour(mvpdRegistration.Location, stationToContourDistance);
                keyHoleContourItem.KeyHoleAdjacentChannelLocations = this.CalculateRadialContour(mvpdRegistration.Location, stationToContourDistance, keyHoleContourItem.KeyHoleArcStarting, keyHoleContourItem.KeyHoleArcEnding);
            }

            return keyHoleContourItem;
        }

        /// <summary>
        /// Filters the TempBAS sites.
        /// </summary>
        /// <param name="wsdInfo">The incumbent information.</param>
        /// <param name="blockedChannels">The blocked channels.</param>
        /// <param name="protectedDevices">The protected devices.</param>
        private void FilterTempBASSites(Incumbent wsdInfo, List<int> blockedChannels, List<ProtectedDevice> protectedDevices = null)
        {
            SquareArea searchArea = GeoCalculations.BuildSquare(wsdInfo.Location, new Distance(Utils.Configuration[Constants.KeyholeOuterCoChannelDistance].ToDouble(), DistanceUnit.KM));

            Distance adjacentChannelKeyHoleDistance = new Distance(Utils.Configuration[Constants.KeyholeOuterAdjChannelDistance].ToDouble(), DistanceUnit.KM);
            Distance sameChannelKeyHoleDistance = new Distance(Utils.Configuration[Constants.KeyholeOuterCoChannelDistance].ToDouble(), DistanceUnit.KM);

            Distance adjacentChannelDistance = new Distance(Utils.Configuration[Constants.KeyholeInnerAdjChannelDistance].ToDouble(), DistanceUnit.KM);
            Distance sameChannelDistance = new Distance(Utils.Configuration[Constants.KeyholeInnerCoChannelDistance].ToDouble(), DistanceUnit.KM);

            string[] selectColumns = new[]
                                     {
                                         "Latitude", "Longitude", "ChannelNumber", "TxLatitude", "TxLongitude", "TempBasChannel", "BroadcastStationContour"
                                     };

            var stations = this.DalcIncumbent.GetIncumbents(Utils.GetRegionalTableName(Constants.TempBasRegistrationTableName), searchArea, selectColumns);

            foreach (var station in stations)
            {
                var channelsToBeBlocked = new List<int>();

                var wsdToStationDistance = GeoCalculations.GetDistance(wsdInfo.Location, new Location(station.Latitude, station.Longitude));

                // bearing from trasnmitter to mvpd receiver
                var stationToParentTxBearing = GeoCalculations.CalculateBearing(station.Location, station.TxLocation);

                var tempbasChannel = JsonSerialization.DeserializeString<TvSpectrum>(station.TempBasChannel);

                // keyhole calculations
                var keyHoleArcStarting = stationToParentTxBearing - 30;
                var keyHoleArcEnding = stationToParentTxBearing + 30;

                if (keyHoleArcStarting < 0)
                {
                    keyHoleArcStarting = 360 - Math.Abs(keyHoleArcStarting);
                }

                if (keyHoleArcEnding > 360)
                {
                    keyHoleArcEnding = keyHoleArcEnding - 360;
                }

                // bearing from mvpd receiver to incumbent in query
                var stationToWsdBearing = GeoCalculations.CalculateBearing(station.Location, wsdInfo.Location);

                if (wsdToStationDistance.InKm() <= sameChannelDistance.InKm())
                {
                    channelsToBeBlocked.Add(station.ChannelNumber);

                    if (wsdToStationDistance.InKm() <= adjacentChannelDistance.InKm())
                    {
                        channelsToBeBlocked.Add(station.ChannelNumber - 1);
                        channelsToBeBlocked.Add(station.ChannelNumber + 1);
                    }

                    this.AddBlockedChannel(blockedChannels, channelsToBeBlocked.ToArray());
                }
                else
                {
                    if (station.TxLocation.Latitude == 0 || station.TxLocation.Longitude == 0)
                    {
                        return;
                    }

                    if (tempbasChannel != null && !string.IsNullOrWhiteSpace(tempbasChannel.CallSign))
                    {
                        if (stationToWsdBearing >= keyHoleArcStarting && stationToWsdBearing <= keyHoleArcEnding)
                        {
                            // if incumbent is in keyhole bearings then check for required distance from TempBAS Receiver
                            if (wsdToStationDistance.InKm() <= sameChannelKeyHoleDistance.InKm())
                            {
                                channelsToBeBlocked.Add(station.ChannelNumber);

                                if (wsdToStationDistance.InKm() <= adjacentChannelKeyHoleDistance.InKm())
                                {
                                    channelsToBeBlocked.Add(station.ChannelNumber - 1);
                                    channelsToBeBlocked.Add(station.ChannelNumber + 1);
                                }

                                this.AddBlockedChannel(blockedChannels, channelsToBeBlocked.ToArray());
                            }
                        }
                    }
                }

                if (protectedDevices != null)
                {
                    ProtectedDevice tempBASDevice = new ProtectedDevice()
                    {
                        ProtectedDeviceType = ProtectedDeviceType.TempBAS,
                        Location = station.Location,
                        Channels = channelsToBeBlocked.ToArray(),
                        CallSign = tempbasChannel.CallSign
                    };

                    tempBASDevice.ContourPoints = this.CalculateRadialContour(station.Location, sameChannelDistance);

                    tempBASDevice.KeyholeArcBearingStart = (int)keyHoleArcStarting;
                    tempBASDevice.KeyholeArcBearingEnd = (int)keyHoleArcEnding;

                    var keyholeContourPoints = this.CalculateRadialContour(station.Location, sameChannelKeyHoleDistance);
                    tempBASDevice.KeyholeArcPoints = new List<Location>();

                    for (int i = tempBASDevice.KeyholeArcBearingStart; i < tempBASDevice.KeyholeArcBearingEnd; i++)
                    {
                        tempBASDevice.KeyholeArcPoints.Add(keyholeContourPoints[i]);
                    }

                    protectedDevices.Add(tempBASDevice);
                }
            }
        }

        /// <summary>
        /// Filters the TVReceive sites.
        /// </summary>
        /// <param name="wsdInfo">The incumbent information.</param>
        /// <param name="blockedChannels">The blocked channels.</param>
        /// <param name="protectedDevices">The protected devices.</param>
        private void FilterTVReceiveSites(Incumbent wsdInfo, List<int> blockedChannels, List<ProtectedDevice> protectedDevices = null)
        {
            SquareArea searchArea = GeoCalculations.BuildSquare(wsdInfo.Location, new Distance(Utils.Configuration[Constants.KeyholeOuterCoChannelDistance].ToDouble(), DistanceUnit.KM));

            Distance adjacentChannelKeyHoleDistance = new Distance(Utils.Configuration[Constants.KeyholeOuterAdjChannelDistance].ToDouble(), DistanceUnit.KM);
            Distance sameChannelKeyHoleDistance = new Distance(Utils.Configuration[Constants.KeyholeOuterCoChannelDistance].ToDouble(), DistanceUnit.KM);

            Distance adjacentChannelDistance = new Distance(Utils.Configuration[Constants.KeyholeInnerAdjChannelDistance].ToDouble(), DistanceUnit.KM);
            Distance sameChannelDistance = new Distance(Utils.Configuration[Constants.KeyholeInnerCoChannelDistance].ToDouble(), DistanceUnit.KM);

            string[] selectColumns = new[]
                                     {
                                         "Latitude", "Longitude", "ChannelNumber", "TxLatitude", "TxLongitude", "TVReceiveSiteReceiveCallSign", "BroadcastStationContour"
                                     };

            var stations = this.DalcIncumbent.GetIncumbents(Utils.GetRegionalTableName(Constants.TVReceiveSiteRegistrationTablename), searchArea, selectColumns);

            foreach (var station in stations)
            {
                var channelsToBeBlocked = new List<int>();

                var wsdToStationDistance = GeoCalculations.GetDistance(wsdInfo.Location, new Location(station.Latitude, station.Longitude));

                // bearing from trasnmitter to mvpd receiver
                var stationToParentTxBearing = GeoCalculations.CalculateBearing(station.Location, station.TxLocation);

                // keyhole calculations
                var keyHoleArcStarting = stationToParentTxBearing - 30;
                var keyHoleArcEnding = stationToParentTxBearing + 30;

                if (keyHoleArcStarting > 360)
                {
                    keyHoleArcStarting = keyHoleArcStarting - 360;
                }

                if (keyHoleArcEnding > 360)
                {
                    keyHoleArcEnding = keyHoleArcEnding - 360;
                }

                // bearing from mvpd receiver to incumbent in query
                var stationToWsdBearing = GeoCalculations.CalculateBearing(station.Location, wsdInfo.Location);

                var receiveChannel = JsonSerialization.DeserializeString<TvSpectrum>(station.TVReceiveSiteReceiveCallSign);

                if (wsdToStationDistance.InKm() <= sameChannelDistance.InKm())
                {
                    channelsToBeBlocked.Add(station.ChannelNumber);

                    if (wsdToStationDistance.InKm() <= adjacentChannelDistance.InKm())
                    {
                        channelsToBeBlocked.Add(station.ChannelNumber - 1);
                        channelsToBeBlocked.Add(station.ChannelNumber + 1);
                    }

                    this.AddBlockedChannel(blockedChannels, channelsToBeBlocked.ToArray());
                }
                else
                {
                    if (station.TxLocation.Latitude.CompareTo(0.0) == 0 || station.TxLocation.Longitude.CompareTo(0.0) == 0)
                    {
                        return;
                    }

                    if (stationToWsdBearing >= keyHoleArcStarting && stationToWsdBearing <= keyHoleArcEnding)
                    {
                        // if incumbent is in keyhole bearings then check for required distance from TempBAS Receiver
                        if (wsdToStationDistance.InKm() <= sameChannelKeyHoleDistance.InKm())
                        {
                            channelsToBeBlocked.Add(station.ChannelNumber);

                            if (wsdToStationDistance.InKm() <= adjacentChannelKeyHoleDistance.InKm())
                            {
                                channelsToBeBlocked.Add(station.ChannelNumber - 1);
                                channelsToBeBlocked.Add(station.ChannelNumber + 1);
                            }

                            this.AddBlockedChannel(blockedChannels, channelsToBeBlocked.ToArray());
                        }
                    }
                }

                if (protectedDevices != null)
                {
                    ProtectedDevice tvreceiveSite = new ProtectedDevice()
                    {
                        ProtectedDeviceType = ProtectedDeviceType.TVReceiveSite,
                        Location = station.Location,
                        Channels = channelsToBeBlocked.ToArray(),
                        CallSign = receiveChannel.CallSign
                    };

                    tvreceiveSite.ContourPoints = this.CalculateRadialContour(station.Location, sameChannelDistance);

                    tvreceiveSite.KeyholeArcBearingStart = (int)keyHoleArcStarting;
                    tvreceiveSite.KeyholeArcBearingEnd = (int)keyHoleArcEnding;

                    var keyholeContourPoints = this.CalculateRadialContour(station.Location, sameChannelKeyHoleDistance);
                    tvreceiveSite.KeyholeArcPoints = new List<Location>();

                    for (int i = tvreceiveSite.KeyholeArcBearingStart; i < tvreceiveSite.KeyholeArcBearingEnd; i++)
                    {
                        tvreceiveSite.KeyholeArcPoints.Add(keyholeContourPoints[i]);
                    }

                    protectedDevices.Add(tvreceiveSite);
                }
            }
        }

        /// <summary>
        /// Filters the TV translators.
        /// </summary>
        /// <param name="wsdInfo">The WSD information.</param>
        /// <param name="blockedChannels">The blocked channels.</param>
        /// <param name="protectedDevices">The protected devices.</param>
        /// <param name="combinedData">The combined data.</param>
        private void FilterTVTranslators(Incumbent wsdInfo, List<int> blockedChannels, List<ProtectedDevice> protectedDevices = null, Incumbent[] combinedData = null)
        {
            SquareArea searchArea = GeoCalculations.BuildSquare(wsdInfo.Location, new Distance(Utils.Configuration[Constants.KeyholeOuterCoChannelDistance].ToDouble(), DistanceUnit.KM));

            Distance adjacentChannelKeyHoleDistance = new Distance(Utils.Configuration[Constants.KeyholeOuterAdjChannelDistance].ToDouble(), DistanceUnit.KM);
            Distance sameChannelKeyHoleDistance = new Distance(Utils.Configuration[Constants.KeyholeOuterCoChannelDistance].ToDouble(), DistanceUnit.KM);

            Distance adjacentChannelDistance = new Distance(Utils.Configuration[Constants.KeyholeInnerAdjChannelDistance].ToDouble(), DistanceUnit.KM);
            Distance sameChannelDistance = new Distance(Utils.Configuration[Constants.KeyholeInnerCoChannelDistance].ToDouble(), DistanceUnit.KM);

            var translators = combinedData.Where(obj => obj.MergedDataIdentifier == Constants.CDBSTranslatorDataTableName);
            ////var translators = this.DalcIncumbent.GetIncumbents(Utils.GetRegionalTableName(Constants.CDBSTranslatorDataTableName), searchArea, null);

            foreach (var translator in translators)
            {
                var channelsToBeBlocked = new List<int>();

                var wsdToTranslatorDistance = GeoCalculations.GetDistance(wsdInfo.Location, new Location(translator.Latitude, translator.Longitude));

                // bearing from trasnmitter to mvpd receiver
                var stationToParentTxBearing = GeoCalculations.CalculateBearing(translator.Location, new Location(translator.ParentLatitude, translator.ParentLongitude));

                // keyhole calculations
                var keyHoleArcStarting = stationToParentTxBearing - 30;
                var keyHoleArcEnding = stationToParentTxBearing + 30;

                if (keyHoleArcStarting < 0)
                {
                    keyHoleArcStarting = 360 - Math.Abs(keyHoleArcStarting);
                }

                if (keyHoleArcEnding > 360)
                {
                    keyHoleArcEnding = keyHoleArcEnding - 360;
                }

                // bearing from translator to incumbent in query
                var translatorToWsdBearing = GeoCalculations.CalculateBearing(translator.Location, wsdInfo.Location);

                if (wsdToTranslatorDistance.InKm() <= sameChannelDistance.InKm())
                {
                    channelsToBeBlocked.Add(translator.Channel);

                    if (wsdToTranslatorDistance.InKm() <= adjacentChannelDistance.InKm())
                    {
                        channelsToBeBlocked.Add(translator.Channel - 1);
                        channelsToBeBlocked.Add(translator.Channel + 1);
                    }

                    this.AddBlockedChannel(blockedChannels, channelsToBeBlocked.ToArray());
                }
                else
                {
                    if (translator.ParentLatitude == 0.0 || translator.ParentLongitude == 0.0)
                    {
                        return;
                    }

                    if (translatorToWsdBearing >= keyHoleArcStarting && translatorToWsdBearing <= keyHoleArcEnding)
                    {
                        // if incumbent is in keyhole bearings then check for required distance from TempBAS Receiver
                        if (wsdToTranslatorDistance.InKm() <= sameChannelKeyHoleDistance.InKm())
                        {
                            channelsToBeBlocked.Add(translator.Channel);

                            if (wsdToTranslatorDistance.InKm() <= adjacentChannelKeyHoleDistance.InKm())
                            {
                                channelsToBeBlocked.Add(translator.Channel - 1);
                                channelsToBeBlocked.Add(translator.Channel + 1);
                            }

                            this.AddBlockedChannel(blockedChannels, channelsToBeBlocked.ToArray());
                        }
                    }
                }

                if (protectedDevices != null)
                {
                    ProtectedDevice tvtranslatorSite = new ProtectedDevice()
                    {
                        ProtectedDeviceType = ProtectedDeviceType.TVTranslator,
                        Location = translator.Location,
                        Channels = channelsToBeBlocked.ToArray(),
                        CallSign = translator.CallSign
                    };

                    tvtranslatorSite.ContourPoints = this.CalculateRadialContour(translator.Location, sameChannelDistance);

                    tvtranslatorSite.KeyholeArcBearingStart = (int)keyHoleArcStarting;
                    tvtranslatorSite.KeyholeArcBearingEnd = (int)keyHoleArcEnding;

                    var keyholeContourPoints = this.CalculateRadialContour(translator.Location, sameChannelKeyHoleDistance);
                    tvtranslatorSite.KeyholeArcPoints = new List<Location>();

                    for (int i = tvtranslatorSite.KeyholeArcBearingStart; i < tvtranslatorSite.KeyholeArcBearingEnd; i++)
                    {
                        tvtranslatorSite.KeyholeArcPoints.Add(keyholeContourPoints[i]);
                    }

                    protectedDevices.Add(tvtranslatorSite);
                }
            }
        }

        /// <summary>
        /// Filters the PLCMRS stations.
        /// </summary>
        /// <param name="incumbentInfo">The incumbent information.</param>
        /// <param name="blockedChannels">The blocked channels.</param>
        /// <param name="protectedDevices">The protected devices.</param>
        /// <param name="combinedData">the combined input data</param>
        private void FilterPLCMRSStations(Incumbent incumbentInfo, List<int> blockedChannels, List<ProtectedDevice> protectedDevices = null, Incumbent[] combinedData = null)
        {
            Distance adjacentChannelDistance = new Distance(Utils.Configuration[Constants.CMRSAdjacentChannelDistance].ToDouble(), DistanceUnit.KM);
            Distance sameChannelDistance = new Distance(Utils.Configuration[Constants.CMRSCoChannelDistance].ToDouble(), DistanceUnit.KM);

            ////var stations = this.DalcIncumbent.GetIncumbents(Utils.GetRegionalTableName(Constants.ULSPLCMRSTableName), searchArea, selectColumns);
            var incumbents = combinedData.Where(obj => obj.MergedDataIdentifier == Constants.ULSPLCMRSTableName);

            foreach (var incumbent in incumbents)
            {
                var channelsToBeBlocked = new List<int>();

                if (incumbentInfo.IncumbentType == IncumbentType.LPAux || incumbentInfo.IncumbentType == IncumbentType.UnlicensedLPAux)
                {
                    var contourPoints = this.CalculateRadialContour(incumbent.Location, sameChannelDistance);
                    if (GeoCalculations.CheckIfLpAuxOverlap(contourPoints, incumbent.Location, incumbentInfo.PointsArea, incumbentInfo.QuadrilateralArea))
                    {
                        channelsToBeBlocked.Add(incumbent.Channel);
                    }
                }
                else
                {
                    var distance = GeoCalculations.GetDistance(incumbentInfo.Location, new Location(incumbent.Latitude, incumbent.Longitude));

                    if (distance.InKm() <= sameChannelDistance.InKm())
                    {
                        channelsToBeBlocked.Add(incumbent.Channel);
                        if (distance.InKm() <= adjacentChannelDistance.InKm())
                        {
                            channelsToBeBlocked.Add(incumbent.Channel - 1);
                            channelsToBeBlocked.Add(incumbent.Channel + 1);
                        }
                    }
                }

                this.AddBlockedChannel(blockedChannels, channelsToBeBlocked.ToArray());

                if (protectedDevices != null)
                {
                    ProtectedDevice pmrsStation = new ProtectedDevice()
                    {
                        ProtectedDeviceType = ProtectedDeviceType.PLCMRSStation,
                        Location = incumbent.Location,
                        Channels = channelsToBeBlocked.ToArray(),
                        CallSign = incumbent.CallSign
                    };

                    pmrsStation.ContourPoints = this.CalculateRadialContour(incumbent.Location, sameChannelDistance);
                    protectedDevices.Add(pmrsStation);
                }
            }
        }

        /// <summary>
        /// Filters the broadcast auxiliary stations.
        /// </summary>
        /// <param name="wsdInfo">The incumbent information.</param>
        /// <param name="blockedChannels">The blocked channels.</param>
        /// <param name="protectedDevices">The protected devices.</param>
        /// <param name="combinedData">The combined data.</param>
        private void FilterBroadcastAuxillaryStations(Incumbent wsdInfo, List<int> blockedChannels, List<ProtectedDevice> protectedDevices = null, Incumbent[] combinedData = null)
        {
            SquareArea searchArea = GeoCalculations.BuildSquare(wsdInfo.Location, new Distance(Utils.Configuration[Constants.KeyholeOuterCoChannelDistance].ToDouble(), DistanceUnit.KM));

            Distance adjacentChannelKeyHoleDistance = new Distance(Utils.Configuration[Constants.KeyholeOuterAdjChannelDistance].ToDouble(), DistanceUnit.KM);
            Distance sameChannelKeyHoleDistance = new Distance(Utils.Configuration[Constants.KeyholeOuterCoChannelDistance].ToDouble(), DistanceUnit.KM);

            Distance adjacentChannelDistance = new Distance(Utils.Configuration[Constants.KeyholeInnerAdjChannelDistance].ToDouble(), DistanceUnit.KM);
            Distance sameChannelDistance = new Distance(Utils.Configuration[Constants.KeyholeInnerCoChannelDistance].ToDouble(), DistanceUnit.KM);

            ////var stations = this.DalcIncumbent.GetIncumbents(Utils.GetRegionalTableName(Constants.ULSBroadcastAuxiliaryTableName), searchArea, selectColumns).ToList();
            var stations = combinedData.Where(obj => obj.MergedDataIdentifier == Constants.ULSBroadcastAuxiliaryTableName);

            foreach (var station in stations)
            {
                var channelsToBeBlocked = new List<int>();

                var wsdToStationDistance = GeoCalculations.GetDistance(wsdInfo.Location, new Location(station.Latitude, station.Longitude));

                var trasmitterBearing = GeoCalculations.CalculateBearing(station.Location, new Location(station.TxLatitude, station.TxLongitude));

                // bearing from BroadCast Auxillary receiver to incumbent in query
                var stationToWsdBearing = GeoCalculations.CalculateBearing(station.Location, wsdInfo.Location);

                // keyhole calculations
                int keyHoleArcStarting = (int)(trasmitterBearing - 30);
                int keyHoleArcEnding = (int)(trasmitterBearing + 30);

                if (keyHoleArcStarting < 0)
                {
                    keyHoleArcStarting = 360 - Math.Abs(keyHoleArcStarting);
                }

                if (keyHoleArcEnding > 360)
                {
                    keyHoleArcEnding = keyHoleArcEnding - 360;
                }

                if (wsdToStationDistance.InKm() <= sameChannelDistance.InKm())
                {
                    channelsToBeBlocked.Add(station.Channel);

                    if (wsdToStationDistance.InKm() <= adjacentChannelDistance.InKm())
                    {
                        channelsToBeBlocked.Add(station.Channel - 1);
                        channelsToBeBlocked.Add(station.Channel + 1);
                    }

                    this.AddBlockedChannel(blockedChannels, channelsToBeBlocked.ToArray());
                }
                else
                {
                    if (stationToWsdBearing >= keyHoleArcStarting && stationToWsdBearing <= keyHoleArcEnding)
                    {
                        // if incumbent is in keyhole bearings then check for required distance from  Transmitter
                        if (wsdToStationDistance.InKm() <= sameChannelKeyHoleDistance.InKm())
                        {
                            channelsToBeBlocked.Add(station.Channel);

                            if (wsdToStationDistance.InKm() <= adjacentChannelKeyHoleDistance.InKm())
                            {
                                channelsToBeBlocked.Add(station.Channel - 1);
                                channelsToBeBlocked.Add(station.Channel + 1);
                            }

                            this.AddBlockedChannel(blockedChannels, channelsToBeBlocked.ToArray());
                        }
                    }
                }

                if (protectedDevices != null)
                {
                    ProtectedDevice broadcastAuzillarySite = new ProtectedDevice()
                    {
                        ProtectedDeviceType = ProtectedDeviceType.BroadcastAuxiliary,
                        Location = station.Location,
                        Channels = channelsToBeBlocked.ToArray(),
                        CallSign = station.CallSign
                    };

                    broadcastAuzillarySite.ContourPoints = this.CalculateRadialContour(station.Location, sameChannelDistance);

                    broadcastAuzillarySite.KeyholeArcBearingStart = (int)keyHoleArcStarting;
                    broadcastAuzillarySite.KeyholeArcBearingEnd = (int)keyHoleArcEnding;

                    var keyholeContourPoints = this.CalculateRadialContour(station.Location, sameChannelKeyHoleDistance);
                    broadcastAuzillarySite.KeyholeArcPoints = new List<Location>();

                    for (int i = broadcastAuzillarySite.KeyholeArcBearingStart; i < broadcastAuzillarySite.KeyholeArcBearingEnd; i++)
                    {
                        broadcastAuzillarySite.KeyholeArcPoints.Add(keyholeContourPoints[i]);
                    }

                    protectedDevices.Add(broadcastAuzillarySite);
                }
            }
        }

        /// <summary>
        /// Filters the T BAND protection.
        /// </summary>
        /// <param name="incumbentInfo">The incumbent information.</param>
        /// <param name="blockedChannels">The blocked channels.</param>
        /// <param name="protectedDevices">The protected devices.</param>
        private void FilterTBandProtection(Incumbent incumbentInfo, List<int> blockedChannels, List<ProtectedDevice> protectedDevices = null)
        {
            Distance adjacentChannelDistance = new Distance(Utils.Configuration[Constants.TBandAdjacentChannelDistance].ToDouble(), DistanceUnit.KM);
            Distance sameChannelDistance = new Distance(Utils.Configuration[Constants.TBandSameChannelDistance].ToDouble(), DistanceUnit.KM);

            var distance = 0.0;

            var tbandStations = this.CommonDalc.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.TBandProtectionTable), null);

            foreach (var tbandStation in tbandStations)
            {
                var tbandLocation = GeoCalculations.GetLocation(tbandStation["Latitude"].StringValue, tbandStation["Longitude"].StringValue);
                var channels = tbandStation["Channel"].StringValue.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries).Select(obj => obj.ToInt32()).ToArray();

                if (incumbentInfo.IncumbentType == IncumbentType.LPAux || incumbentInfo.IncumbentType == IncumbentType.UnlicensedLPAux)
                {
                    var contourPoints = this.CalculateRadialContour(tbandLocation, sameChannelDistance);
                    if (GeoCalculations.CheckIfLpAuxOverlap(contourPoints, tbandLocation, incumbentInfo.PointsArea, incumbentInfo.QuadrilateralArea))
                    {
                        this.AddBlockedChannel(blockedChannels, channels);
                    }

                    if (protectedDevices != null)
                    {
                        ProtectedDevice tbandDevice = new ProtectedDevice()
                        {
                            ProtectedDeviceType = ProtectedDeviceType.TBandStation,
                            Location = tbandLocation,
                            Channels = channels
                        };

                        tbandDevice.ContourPoints = contourPoints;
                        protectedDevices.Add(tbandDevice);
                    }
                }
                else
                {
                    distance = GeoCalculations.GetDistance(incumbentInfo.Location, tbandLocation).InKm();

                    if (distance < sameChannelDistance.InKm())
                    {
                        if (protectedDevices != null)
                        {
                            ProtectedDevice tbandDevice = new ProtectedDevice()
                            {
                                ProtectedDeviceType = ProtectedDeviceType.TBandStation,
                                Location = tbandLocation,
                                Channels = channels
                            };

                            tbandDevice.ContourPoints = this.CalculateRadialContour(tbandLocation, sameChannelDistance);

                            protectedDevices.Add(tbandDevice);
                        }

                        this.AddBlockedChannel(blockedChannels, channels);

                        if (distance < adjacentChannelDistance.InKm())
                        {
                            for (int i = 0; i < channels.Length; i++)
                            {
                                this.AddBlockedChannel(blockedChannels, channels[i]);
                            }
                        }

                        return;
                    }
                }
            }
        }

        /// <summary>
        /// Checks for CDBS TV stations.
        /// </summary>
        /// <param name="incumbentInfo">The incumbent information.</param>
        /// <param name="blockedChannels">The blocked channels.</param>
        /// <param name="reducedPowerChannels">The reduced power channels.</param>
        /// <param name="adjacentChannels">The adjacent channels.</param>
        /// <param name="protectedDevices">The protected devices.</param>
        private void FilterTVStations(Incumbent incumbentInfo, List<int> blockedChannels, List<int> reducedPowerChannels, List<int> adjacentChannels, List<ProtectedDevice> protectedDevices = null)
        {
            SquareArea searchArea = null;

            searchArea = GeoCalculations.BuildSquare(incumbentInfo.Location, new Distance(Utils.Configuration[Constants.TvStationSearchDistance].ToDouble(), DistanceUnit.KM));

            Distance adjacentChannelDistance = null;
            Distance sameChannelDistance = null;

            ServiceCacheRequestParameters requestParams = new ServiceCacheRequestParameters
                                                        {
                                                            SearchArea = searchArea
                                                        };

            var incumbents = DatabaseCache.ServiceCacheHelper.GetServiceCacheObjects(ServiceCacheObjectType.TvEngData, requestParams) as Incumbent[];

            ////var incumbents = this.DalcIncumbent.GetIncumbents(Utils.GetRegionalTableName(Constants.CDBSUSMexicoTVEngDataTableName), searchArea);
            if (incumbentInfo.IncumbentType == IncumbentType.Fixed)
            {
                sameChannelDistance = this.GetProtectedDistanceForDevice(incumbentInfo.Height, false);
                adjacentChannelDistance = this.GetProtectedDistanceForDevice(incumbentInfo.Height, true);
            }
            else if (incumbentInfo.IncumbentType == IncumbentType.Mode_1 || incumbentInfo.IncumbentType == IncumbentType.Mode_2)
            {
                // must be portable device then always use height less than 3 mtr
                sameChannelDistance = this.GetProtectedDistanceForDevice(2.9, false);
                adjacentChannelDistance = this.GetProtectedDistanceForDevice(2.9, true);
            }
            else if (incumbentInfo.IncumbentType == IncumbentType.LPAux || incumbentInfo.IncumbentType == IncumbentType.UnlicensedLPAux)
            {
                sameChannelDistance = new Distance(1, DistanceUnit.KM);
                adjacentChannelDistance = new Distance(1, DistanceUnit.KM);
            }

            foreach (var incumbent in incumbents)
            {
                var channelsToBeBlocked = new List<int>();
                var adjacentChannelsToBeBlocked = new List<int>();

                // check for co-channel interference
                if (incumbent.IsInOrAroundContour(incumbentInfo.Location, sameChannelDistance))
                {
                    // incumbent found whose polygon has this incumbent or is in buffer distance of incumbent
                    channelsToBeBlocked.Add(incumbent.Channel);

                    // check for adjacent channel interference
                    if (incumbent.IsInOrAroundContour(incumbentInfo.Location, adjacentChannelDistance))
                    {
                        if (incumbentInfo.IncumbentType == IncumbentType.Fixed)
                        {
                            // incumbent found whose polygon has this incumbent or is in buffer distance of incumbent
                            // block adjacent channels for operation
                            channelsToBeBlocked.Add(incumbent.Channel - 1);
                            channelsToBeBlocked.Add(incumbent.Channel + 1);
                            adjacentChannelsToBeBlocked.Add(incumbent.Channel - 1);
                            adjacentChannelsToBeBlocked.Add(incumbent.Channel + 1);
                        }
                        else if (incumbentInfo.IncumbentType == IncumbentType.Mode_1 || incumbentInfo.IncumbentType == IncumbentType.Mode_2)
                        {
                            reducedPowerChannels.Add(incumbent.Channel - 1);
                            reducedPowerChannels.Add(incumbent.Channel + 1);
                        }
                    }

                    if (protectedDevices != null)
                    {
                        ProtectedDevice protectedDevice = new ProtectedDevice()
                        {
                            ProtectedDeviceType = ProtectedDeviceType.TvStation,
                            Location = incumbent.Location,
                            Channels = channelsToBeBlocked.ToArray(),
                            CallSign = incumbent.CallSign
                        };

                        protectedDevice.ContourPoints = JsonSerialization.DeserializeString<Contour>(incumbent.Contour).ContourPoints;
                        protectedDevices.Add(protectedDevice);
                    }
                }

                this.AddBlockedChannel(blockedChannels, channelsToBeBlocked.ToArray());
                this.AddAdjacentChannel(adjacentChannels, adjacentChannelsToBeBlocked.ToArray());
            }
        }

        /// <summary>
        /// Filters the TV stations for LPAUX
        /// </summary>
        /// <param name="incumbentInfo">The incumbent information.</param>
        /// <param name="blockedChannels">The blocked channels.</param>
        /// <param name="reducedPowerChannels">The reduced power channels.</param>
        /// <param name="adjacentChannels">The adjacent channels.</param>
        /// <param name="protectedDevices">The protected devices.</param>
        private void FilterTVStationsForLpAux(Incumbent incumbentInfo, List<int> blockedChannels, List<int> reducedPowerChannels, List<int> adjacentChannels, List<ProtectedDevice> protectedDevices = null)
        {
            Location incumbentLocation = new Location();

            // as search area is very large and lpaux covering region comparatively very small pickup any one point
            if (incumbentInfo.PointsArea != null)
            {
                incumbentLocation = incumbentInfo.PointsArea[0].ToLocation();
            }
            else if (incumbentInfo.QuadrilateralArea != null)
            {
                incumbentLocation = incumbentInfo.QuadrilateralArea[0].NWPoint.ToLocation();
            }

            SquareArea searchArea = null;

            searchArea = GeoCalculations.BuildSquare(incumbentLocation, new Distance(Utils.Configuration[Constants.TvStationSearchDistance].ToDouble(), DistanceUnit.KM));

            ServiceCacheRequestParameters requestParams = new ServiceCacheRequestParameters
            {
                SearchArea = searchArea
            };

            var incumbents = DatabaseCache.ServiceCacheHelper.GetServiceCacheObjects(ServiceCacheObjectType.TvEngData, requestParams) as Incumbent[];

            ////var incumbents = this.DalcIncumbent.GetIncumbents(Utils.GetRegionalTableName(Constants.CDBSUSMexicoTVEngDataTableName), searchArea);

            foreach (var incumbent in incumbents)
            {
                var channelsToBeBlocked = new List<int>();
                var adjacentChannelsToBeBlocked = new List<int>();

                var contourPoints = JsonSerialization.DeserializeString<Contour>(incumbent.Contour).ContourPoints;

                if (GeoCalculations.CheckIfLpAuxOverlap(contourPoints, incumbent.Location, incumbentInfo.PointsArea, incumbentInfo.QuadrilateralArea))
                {
                    // incumbent found whose polygon has this incumbent or is in buffer distance of incumbent
                    channelsToBeBlocked.Add(incumbent.Channel);
                }

                if (protectedDevices != null)
                {
                    ProtectedDevice protectedDevice = new ProtectedDevice()
                    {
                        ProtectedDeviceType = ProtectedDeviceType.TvStation,
                        Location = incumbent.Location,
                        Channels = channelsToBeBlocked.ToArray(),
                        CallSign = incumbent.CallSign
                    };

                    protectedDevice.ContourPoints = contourPoints;
                    protectedDevices.Add(protectedDevice);
                }

                this.AddBlockedChannel(blockedChannels, channelsToBeBlocked.ToArray());
                this.AddAdjacentChannel(adjacentChannels, adjacentChannelsToBeBlocked.ToArray());
            }
        }

        /// <summary>
        /// Filters the used spectrum.
        /// </summary>
        /// <param name="incumbentInfo">The incumbent information.</param>
        /// <param name="blockedChannels">The blocked channels.</param>
        private void FilterUsedSpectrum(Incumbent incumbentInfo, List<int> blockedChannels)
        {
            ////// search for search Radius 1
            ////var searchArea1 = GeoCalculations.BuildSquare(wsdInfo.Location, new Distance(1.5, DistanceUnit.KM));

            ////// radius of 14.4 km for TVBD devices
            ////var searchArea2 = GeoCalculations.BuildSquare(wsdInfo.Location, new Distance(14.4, DistanceUnit.KM));

            ////string[] selectColumns = new[]
            ////                         {
            ////                             "Latitude", "Longitude", "FccTvbdDeviceType"
            ////                         };

            ////Incumbent[] tvbdDevices = this.DalcIncumbent.GetIncumbents(Utils.GetRegionalTableName(Constants.SpectrumUsageTable), searchArea2, new { FccTvbdDeviceType = "Fixed" });

            ////List<Incumbent> stations = this.DalcIncumbent.GetAllStations(searchArea3).ToList();

            ////foreach (var nearestIncumbent in nearestIncumbents.GroupBy(obj => obj.Channel))
            ////{
            ////    blockedChannels.Add(nearestIncumbent.FirstOrDefault().Channel);
            ////}

            ////Distance sameChannelDistance = null;
            ////Distance adjacentChannelDistance = null;

            ////if (wsdInfo.IncumbentType == IncumbentType.Fixed)
            ////{
            ////    sameChannelDistance = new Distance(14.4, DistanceUnit.KM);
            ////    adjacentChannelDistance = new Distance(1.8, DistanceUnit.KM);
            ////}
            ////else
            ////{
            ////    // must be portable device then
            ////    sameChannelDistance = new Distance(4, DistanceUnit.KM);
            ////    adjacentChannelDistance = new Distance(0.4, DistanceUnit.KM);
            ////}

            ////foreach (var incumbents in stations.GroupBy(obj => obj.Channel))
            ////{
            ////    if (blockedChannels.Contains(incumbents.Key))
            ////    {
            ////        continue;
            ////    }

            ////    // check for co-channel interference
            ////    Incumbent incumbentData = incumbents.CheckInAndAroundContour(wsdInfo, sameChannelDistance);

            ////    if (incumbentData != null)
            ////    {
            ////        // incumbent found whose polygon has this incumbent or is in buffer distance of incumbent
            ////        blockedChannels.Add(incumbentData.Channel);
            ////    }

            ////    // check for adjacent channel interference
            ////    incumbentData = incumbents.CheckInAndAroundContour(wsdInfo, adjacentChannelDistance);

            ////    if (incumbentData != null)
            ////    {
            ////        // incumbent found whose polygon has this incumbent or is in buffer distance of incumbent
            ////        // block adjacent channels for operation
            ////        blockedChannels.Add(incumbentData.Channel == 2 ? 2 : incumbentData.Channel - 1);
            ////        blockedChannels.Add(incumbentData.Channel == 59 ? 59 : incumbentData.Channel + 1);
            ////    }
            ////}
        }

        /// <summary>
        /// Adds the blocked channel.
        /// </summary>
        /// <param name="blockedChannels">The blocked channels.</param>
        /// <param name="channels">The channels.</param>
        private void AddBlockedChannel(List<int> blockedChannels, params int[] channels)
        {
            List<int> actualChannels;
            if (channels.Length == 3)
            {
                actualChannels = new List<int>();

                var midChannel = channels[0];
                actualChannels.Add(midChannel);

                for (int i = 1; i < channels.Length; i++)
                {
                    // if mid channel in lower band then higher band channel should not be added as adjacent channel
                    if ((midChannel == 6 || midChannel == 13) && channels[i] == midChannel + 1)
                    {
                        continue;
                    }
                    else if ((midChannel == 7 || midChannel == 14) && channels[i] == midChannel - 1)
                    {
                        // if mid channel in higher band then lower band channel should not be added as adjacent channel
                        continue;
                    }

                    actualChannels.Add(channels[i]);
                }
            }
            else
            {
                actualChannels = channels.ToList();
            }

            foreach (var channel in actualChannels)
            {
                if (!blockedChannels.Contains(channel))
                {
                    blockedChannels.Add(channel);
                }
            }
        }

        /// <summary>
        /// Adds the adjacent channel.
        /// </summary>
        /// <param name="adjacentChannels">The blocked channels.</param>
        /// <param name="channels">The channels.</param>
        private void AddAdjacentChannel(List<int> adjacentChannels, params int[] channels)
        {
            foreach (var channel in channels)
            {
                adjacentChannels.Add(channel);
            }
        }

        /// <summary>
        /// Adds the wireless microphone channels.
        /// </summary>
        /// <param name="blockedChannels">The blocked channels.</param>
        /// <param name="adjacentChannels">The adjacent channels.</param>
        /// <param name="protectedDevices">The protected devices.</param>
        private void AddWirelessMicrophoneChannels(List<int> blockedChannels, List<int> adjacentChannels, List<ProtectedDevice> protectedDevices = null)
        {
            blockedChannels.Sort();
            List<int> protectedChannels = new List<int>();

            // search for first free channel for wireless microphone in downside
            var channelForMicrophone = 36;
            var firstChannelIndex = blockedChannels.IndexOf(channelForMicrophone);
            var adjacentChannelIndex = 0;

            bool channelFound = false;

            if (firstChannelIndex > 0)
            {
                for (int channelIndex = firstChannelIndex; channelIndex >= 0; channelIndex--, channelForMicrophone--)
                {
                    adjacentChannelIndex = adjacentChannels.IndexOf(channelForMicrophone);
                    if (adjacentChannelIndex > 0)
                    {
                        channelFound = true;
                        break;
                    }

                    if (blockedChannels[channelIndex] != channelForMicrophone)
                    {
                        blockedChannels.Add(channelForMicrophone);
                        channelFound = true;
                        break;
                    }
                }

                if (!channelFound)
                {
                    blockedChannels.Add(channelForMicrophone);
                }
            }
            else
            {
                blockedChannels.Add(channelForMicrophone);
            }

            protectedChannels.Add(channelForMicrophone);

            // search for second free channel for wireless microphone in upwards
            channelForMicrophone = 38;
            var secondChannelIndex = blockedChannels.IndexOf(channelForMicrophone);
            channelFound = false;
            if (secondChannelIndex > 0)
            {
                for (int channelIndex = secondChannelIndex; channelIndex <= blockedChannels.Count; channelIndex++, channelForMicrophone++)
                {
                    adjacentChannelIndex = adjacentChannels.IndexOf(channelForMicrophone);
                    if (adjacentChannelIndex > 0)
                    {
                        channelFound = true;
                        break;
                    }

                    if (blockedChannels[channelIndex] != channelForMicrophone)
                    {
                        blockedChannels.Add(channelForMicrophone);
                        channelFound = true;
                        break;
                    }
                }

                if (!channelFound)
                {
                    blockedChannels.Add(channelForMicrophone);
                }
            }
            else
            {
                blockedChannels.Add(channelForMicrophone);
            }

            protectedChannels.Add(channelForMicrophone);

            if (protectedDevices != null)
            {
                protectedDevices.Add(new ProtectedDevice()
                {
                    Channels = protectedChannels.ToArray(),
                    ProtectedDeviceType = ProtectedDeviceType.WirelessMicrophone
                });
            }
        }

        /// <summary>
        /// Checks for radio astronomy.
        /// </summary>
        /// <param name="point">The point.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        private bool CheckForRadioAstronomy(Location point)
        {
            var radioAstronomyLocations = this.CommonDalc.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.RadioAstronomyProtection), null);

            foreach (var radioAstronomyLocation in radioAstronomyLocations)
            {
                if (radioAstronomyLocation.Properties.ContainsKey("Latitude"))
                {
                    var location = GeoCalculations.GetLocation(radioAstronomyLocation["Latitude"].StringValue, radioAstronomyLocation["Longitude"].StringValue);
                    if (GeoCalculations.GetDistance(point, location).InKm() < Utils.Configuration[Constants.RadioAstronomyDistance].ToDouble())
                    {
                        return true;
                    }
                }
                else
                {
                    SquareArea largeArray = new SquareArea(GeoCalculations.GetLocation(radioAstronomyLocation["LatitudeNE"].StringValue, radioAstronomyLocation["LongitudeNE"].StringValue), GeoCalculations.GetLocation(radioAstronomyLocation["LatitudeSW"].StringValue, radioAstronomyLocation["LongitudeSW"].StringValue));
                    if (GeoCalculations.IsPointInSquare(largeArray, point))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Gets the default antenna pattern.
        /// </summary>
        /// <returns>returns antenna patterns.</returns>
        private double[] GetDefaultAntennaPattern()
        {
            double[] defaultAntennaPattern = new double[361];
            for (int i = 0; i < defaultAntennaPattern.Length; i++)
            {
                defaultAntennaPattern[i] = 1;
            }

            return defaultAntennaPattern;
        }

        /// <summary>
        /// Gets the protected distance for device.
        /// </summary>
        /// <param name="deviceHeight">Height of the device.</param>
        /// <param name="isAdjacent">if set to <c>true</c> [is adjacent].</param>
        /// <returns>returns Distance.</returns>
        private Distance GetProtectedDistanceForDevice(double deviceHeight, bool isAdjacent)
        {
            if (deviceHeight < 3)
            {
                if (!isAdjacent)
                {
                    return new Distance(Utils.Configuration["TvStationDeviceCoChannel_HAAT3_KM"].ToDouble(), DistanceUnit.KM);
                }
                else
                {
                    return new Distance(Utils.Configuration["TvStationDeviceAdjChannel_HAAT3_KM"].ToDouble(), DistanceUnit.KM);
                }
            }
            else if (deviceHeight >= 3 && deviceHeight < 10)
            {
                if (!isAdjacent)
                {
                    return new Distance(Utils.Configuration["TvStationDeviceCoChannel_HAAT10_KM"].ToDouble(), DistanceUnit.KM);
                }
                else
                {
                    return new Distance(Utils.Configuration["TvStationDeviceAdjChannel_HAAT10_KM"].ToDouble(), DistanceUnit.KM);
                }
            }
            else if (deviceHeight >= 10 && deviceHeight < 30)
            {
                if (!isAdjacent)
                {
                    return new Distance(Utils.Configuration["TvStationDeviceCoChannel_HAAT30_KM"].ToDouble(), DistanceUnit.KM);
                }
                else
                {
                    return new Distance(Utils.Configuration["TvStationDeviceAdjChannel_HAAT30_KM"].ToDouble(), DistanceUnit.KM);
                }
            }
            else if (deviceHeight >= 30 && deviceHeight < 50)
            {
                if (!isAdjacent)
                {
                    return new Distance(Utils.Configuration["TvStationDeviceCoChannel_HAAT50_KM"].ToDouble(), DistanceUnit.KM);
                }
                else
                {
                    return new Distance(Utils.Configuration["TvStationDeviceAdjChannel_HAAT50_KM"].ToDouble(), DistanceUnit.KM);
                }
            }

            return new Distance(0, DistanceUnit.KM);
        }

        /// <summary>
        /// Calculates the radial contour.
        /// </summary>
        /// <param name="startLocation">The start location.</param>
        /// <param name="distance">The distance.</param>
        /// <returns>returns List{Location}.</returns>
        private List<Location> CalculateRadialContour(Location startLocation, Distance distance)
        {
            // rotating azimuth at 1 degree intervals
            List<Location> deltaLocations = new List<Location>();

            for (int azimuth = 0; azimuth <= 360; azimuth++)
            {
                var deltaCoord = GeoCalculations.GetLocationTowardsBearing(startLocation, distance, azimuth);
                deltaLocations.Add(deltaCoord);
            }

            return deltaLocations;
        }

        /// <summary>
        /// Calculates the radial contour.
        /// </summary>
        /// <param name="startLocation">The start location.</param>
        /// <param name="distance">The distance.</param>
        /// <param name="startAzimuth">The start azimuth.</param>
        /// <param name="endAzimuth">The end azimuth.</param>
        /// <returns>returns locations.</returns>
        private List<Location> CalculateRadialContour(Location startLocation, Distance distance, int startAzimuth, int endAzimuth)
        {
            // rotating azimuth at 1 degree intervals
            List<Location> deltaLocations = new List<Location>();

            for (int azimuth = startAzimuth; azimuth <= endAzimuth; azimuth++)
            {
                var deltaCoord = GeoCalculations.GetLocationTowardsBearing(startLocation, distance, azimuth);
                deltaLocations.Add(deltaCoord);
            }

            return deltaLocations;
        }

        /// <summary>
        /// Calculates the 3sec corners.
        /// </summary>
        /// <param name="location">The location.</param>
        /// <param name="pt_corner_lat">The PT_CORNER_LAT.</param>
        /// <param name="pt_corner_lon">The PT_CORNER_LON.</param>
        private void Calculate3SecCorners(Location location, out FortranDoubleArray pt_corner_lat, out FortranDoubleArray pt_corner_lon)
        {
            // Convert lat/long to Degree minute seconds format
            var latDegree = GeoCalculations.GetDegreeMinuteSecond(location.Latitude);
            var lonDegree = GeoCalculations.GetDegreeMinuteSecond(location.Longitude);

            List<double> latSecs = new List<double>();
            List<double> lonSecs = new List<double>();

            // Adjust latitude seconds based on whether fraction part of seconds > 0.5
            if ((latDegree.Item3 - (int)latDegree.Item3) > 0.5)
            {
                latSecs.Add((int)latDegree.Item3 + .5);
                latSecs.Add((int)latDegree.Item3 + 1.5);
                latSecs.Add((int)latDegree.Item3 + 1.5);
                latSecs.Add((int)latDegree.Item3 + .5);
            }
            else
            {
                latSecs.Add((int)latDegree.Item3 - .5);
                latSecs.Add((int)latDegree.Item3 + .5);
                latSecs.Add((int)latDegree.Item3 + .5);
                latSecs.Add((int)latDegree.Item3 - .5);
            }

            // Adjust longitude seconds based on whether fraction part of seconds > 0.5
            if ((lonDegree.Item3 - (int)lonDegree.Item3) > 0.5)
            {
                lonSecs.Add((int)lonDegree.Item3 + 1.5);
                lonSecs.Add((int)lonDegree.Item3 + 1.5);
                lonSecs.Add((int)lonDegree.Item3 + .5);
                lonSecs.Add((int)lonDegree.Item3 + .5);
            }
            else
            {
                lonSecs.Add((int)lonDegree.Item3 + .5);
                lonSecs.Add((int)lonDegree.Item3 + .5);
                lonSecs.Add((int)lonDegree.Item3 - .5);
                lonSecs.Add((int)lonDegree.Item3 - .5);
            }

            pt_corner_lat = new FortranDoubleArray(4);
            pt_corner_lon = new FortranDoubleArray(4);

            // Convert latitude to decimal format from degree minute seconds format
            pt_corner_lat[1] = GeoCalculations.GetLattitudeInDegrees(latDegree.Item1, latDegree.Item2, latSecs[0]);
            pt_corner_lat[2] = GeoCalculations.GetLattitudeInDegrees(latDegree.Item1, latDegree.Item2, latSecs[1]);
            pt_corner_lat[3] = GeoCalculations.GetLattitudeInDegrees(latDegree.Item1, latDegree.Item2, latSecs[2]);
            pt_corner_lat[4] = GeoCalculations.GetLattitudeInDegrees(latDegree.Item1, latDegree.Item2, latSecs[3]);

            pt_corner_lon[1] = GeoCalculations.GetLongitudeInDegrees(lonDegree.Item1, lonDegree.Item2, lonSecs[0]);
            pt_corner_lon[2] = GeoCalculations.GetLongitudeInDegrees(lonDegree.Item1, lonDegree.Item2, lonSecs[1]);
            pt_corner_lon[3] = GeoCalculations.GetLongitudeInDegrees(lonDegree.Item1, lonDegree.Item2, lonSecs[2]);
            pt_corner_lon[4] = GeoCalculations.GetLongitudeInDegrees(lonDegree.Item1, lonDegree.Item2, lonSecs[3]);
        }

        /// <summary>
        /// Interpolates for elevations.
        /// </summary>
        /// <param name="location">The location.</param>
        /// <param name="pt_corner_lat">The PT_CORNER_LAT.</param>
        /// <param name="pt_corner_lon">The PT_CORNER_LON.</param>
        /// <param name="corner_el">The CORNER_ELEVATIONS.</param>
        /// <returns>returns System.Double.</returns>
        private double InterpolateForElevations(Location location, FortranDoubleArray pt_corner_lat, FortranDoubleArray pt_corner_lon, FortranDoubleArray corner_el)
        {
            double pt_lat = location.Latitude, pt_lon = location.Longitude;
            const double PTS_PER_DEG_LAT = 3600; // 1.0 / 0.000277777777778;
            const double PTS_PER_DEG_LON = 3600; // 1.0 / 0.000277777777778;

            // Find difference in "distance" from corners
            double y1 = (pt_lat - pt_corner_lat[1]) * PTS_PER_DEG_LAT;
            double x1 = (pt_lon - pt_corner_lon[1]) * PTS_PER_DEG_LON;
            double y2 = 1.0 - y1;
            double x2 = 1.0 - x1;

            // Interpolate
            double elevation = corner_el[1] * x2 * y2;
            elevation = elevation + (corner_el[4] * x1 * y2);
            elevation = elevation + (corner_el[3] * x1 * y1);
            elevation = elevation + (corner_el[2] * x2 * y1);

            return elevation;
        }

        /// <summary>
        /// Gets the interpolated locations.
        /// </summary>
        /// <param name="sourceLocation">The source location.</param>
        /// <param name="azimuth">The azimuth.</param>
        /// <returns>returns List{Location}.</returns>
        private List<Location> GetInterpolatedLocations(Location sourceLocation, int azimuth)
        {
            List<Location> interpolatedLocations = new List<Location>();

            // coordinate at 3.2km from vincenty
            var startingPoint = GeoCalculations.GetLocationTowardsBearing(sourceLocation, 3200, azimuth);

            // coordinate at 16.1km from vincenty
            var endingPoint = GeoCalculations.GetLocationTowardsBearing(sourceLocation, 16100, azimuth);

            double deltaLatDiff = (endingPoint.Latitude - startingPoint.Latitude) / 129.00;
            double deltaLonDiff = (endingPoint.Longitude - startingPoint.Longitude) / 129.00;

            double lastLatPoint = startingPoint.Latitude;
            double lastLonPoint = startingPoint.Longitude;

            for (int i = 0; i < 128; i++)
            {
                lastLatPoint += deltaLatDiff;
                lastLonPoint += deltaLonDiff;
                interpolatedLocations.Add(new Location(lastLatPoint, lastLonPoint));
            }

            interpolatedLocations.Insert(0, startingPoint);
            interpolatedLocations.Add(endingPoint);

            return interpolatedLocations;
        }

        /// <summary>
        /// To clip-off the foreign country region incumbent contours that goes inside the US.
        /// </summary>
        /// <param name="contour">Contour data for an Incumbent.</param>
        private void AdjustIncumbentOutsideUS(Contour contour)
        {
            List<int> replacements = new List<int>();
            bool isDomesticIncumbent = false;

            List<RegionPolygonsCache> subregions = (List<RegionPolygonsCache>)DatabaseCache.ServiceCacheHelper.GetServiceCacheObjects(ServiceCacheObjectType.RegionPolygons, null);

            // If there exist no region polygons contours or generated contour is null then simply return to caller.
            if (subregions == null || contour == null)
            {
                return;
            }

            foreach (RegionPolygonsCache subregion in subregions)
            {
                isDomesticIncumbent = (subregion.Contains(contour.Point.Latitude, contour.Point.Longitude) != null);

                if (isDomesticIncumbent)
                {
                    break;
                }
            }

            if (!isDomesticIncumbent && contour.ContourPoints != null)
            {
                foreach (RegionPolygonsCache subregion in subregions)
                {
                    int i = 0;

                    while (i < contour.ContourPoints.Count)
                    {
                        ////bool replaced = false;
                        List<Location> containingPolygon = null;
                        if (((containingPolygon = subregion.Contains(contour.ContourPoints[i].Latitude, contour.ContourPoints[i].Longitude)) != null) && (i < contour.ContourPoints.Count))
                        {
                            Location startLine = contour.Point;
                            Location endLine = contour.ContourPoints[i];

                            // Get all the points of intersectoin between the polygon and the line segment between the center of the contour and the point that is in the polygon
                            List<Location> intersections = GeoCalculations.GetIntersections(containingPolygon, startLine, endLine);

                            Location minDistancePoint = null;
                            double minDistance = double.MaxValue;

                            // you may end up with multiple points, so take the point closest to the center point of the contour
                            foreach (var location in intersections)
                            {
                                if (minDistance > GeoCalculations.GetDistance(location, contour.Point).Value)
                                {
                                    minDistance = GeoCalculations.GetDistance(location, contour.Point).Value;
                                    minDistancePoint = location;
                                }
                            }

                            contour.ContourPoints[i] = minDistancePoint;
                        }

                        i++;
                    }
                }
            }
        }
    }
}
