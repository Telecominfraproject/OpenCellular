// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.RegionCalculation
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.IO;
    using System.Linq;
    using System.Security.Cryptography;
    using System.Text;
    using System.Threading.Tasks;
    using Common.CacheHelpers;
    using Data.OData.Query.SemanticAst;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.Whitespace.RegionCalculation.Propagation;
    using Microsoft.WindowsAzure.Storage.Table;
    using WindowsAzure.ServiceRuntime;

    /// <summary>
    /// Performs contour calculations according to FCC rules.
    /// </summary>
    public class OfcomCalculation : IRegionCalculation
    {
        private const string PMSEDateTimeFormat = "dd/MM/yyyy HH:mm";

        /// <summary>Gets or sets the incumbent DALC.</summary>
        /// <value>The DALC incumbent.</value>
        [Dependency]
        public IDalcIncumbent DalcIncumbent { get; set; }

        /// <summary>Gets or sets the terrain elevation.</summary>
        /// <value>The terrain elevation.</value>
        [Dependency]
        public IClutterDatasetReader ClutterReader { get; set; }

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

        /// <summary>
        /// Gets or sets the DALC common.
        /// </summary>
        /// <value>The DALC common.</value>
        [Dependency]
        public IDalcCommon DalcCommon { get; set; }

        /// <summary>
        /// Calculates path loss using free space equation
        /// </summary>
        /// <param name="distance">Distance in km</param>
        /// <param name="frequency">frequency in MHz</param>
        /// <param name="heightMaster">master height in meters</param>
        /// <param name="heightSlave">slave height in meters</param>
        /// <returns>returns path loss</returns>
        public static double CalculateFreePathloss(double distance, double frequency, double heightMaster, double heightSlave)
        {
            double pathLoss = 0;
            double hb = heightMaster;
            double hm = heightSlave;
            pathLoss = 32.4 + (20 * Math.Log10(frequency)) + (10 * Math.Log10(Math.Pow(distance, 2) + (Math.Pow(hb - hm, 2) / Math.Pow(10, 6))));

            return pathLoss;
        }

        /// <summary>
        /// Calculates path loss using open clutter equation
        /// </summary>
        /// <param name="distance">Distance in km</param>
        /// <param name="frequency">frequency in MHz</param>
        /// <param name="heightMaster">master height in meters</param>
        /// <param name="heightSlave">slave height in meters</param>
        /// <returns>returns path loss</returns>
        public static double CalculateOpenPathloss(double distance, double frequency, double heightMaster, double heightSlave)
        {
            double pathLoss = 0;

            double logMinMax150 = Math.Log10(Math.Min(Math.Max(150.0, frequency), 2000.0));
            pathLoss = CalculateUrbanPathloss(distance, frequency, heightMaster, heightSlave) - (4.78 * Math.Pow(logMinMax150, 2)) + (18.33 * logMinMax150) - 40.94;

            return pathLoss;
        }

        /// <summary>
        /// Calculates path loss using urban clutter equation
        /// </summary>
        /// <param name="distance">Distance in km</param>
        /// <param name="frequency">frequency in MHz</param>
        /// <param name="heightMaster">master height in meters</param>
        /// <param name="heightSlave">slave height in meters</param>
        /// <returns>returns path loss</returns>
        public static double CalculateUrbanPathloss(double distance, double frequency, double heightMaster, double heightSlave)
        {
            double pathLoss = 0;
            double alpha = 1;

            double hb = heightMaster;
            double hm = heightSlave;
            double aHm = (((1.1 * Math.Log10(frequency)) - 0.7) * Math.Min(10, hm)) - ((1.56 * Math.Log10(frequency)) - 0.8) + Math.Max(0.0, 20.0 * Math.Log10(hm / 10.0));
            double bHb = Math.Min(0.0, 20.0 * Math.Log10(hb / 30));

            if (distance > 20)
            {
                alpha = 1 + ((0.14 + ((1.87 * Math.Pow(10, -4) * frequency) + (1.07 * Math.Pow(10, -3) * hb))) * Math.Pow(Math.Log10(distance / 20), 0.8));
            }

            pathLoss = 69.6 + (26.2 * Math.Log10(frequency)) - (13.82 * Math.Log10(Math.Max(30.0, hb))) + ((44.9 - (6.55 * Math.Log10(Math.Max(30.0, hb)))) * Math.Pow(Math.Log10(distance), alpha)) - aHm - bHb;

            return pathLoss;
        }

        /// <summary>
        /// Calculates the sub urban path loss.
        /// </summary>
        /// <param name="distance">Distance in km</param>
        /// <param name="frequency">frequency in MHz</param>
        /// <param name="heightMaster">master height in meters</param>
        /// <param name="heightSlave">slave height in meters</param>
        /// <returns>returns path loss</returns>
        public static double CalculateSubUrbanPathloss(double distance, double frequency, double heightMaster, double heightSlave)
        {
            double pathLossUrban = CalculateUrbanPathloss(distance, frequency, heightMaster, heightSlave);
            double pathLoss = pathLossUrban - (2 * Math.Pow(Math.Log10(Math.Min(Math.Max(150, frequency), 2000) / 28.0), 2)) - 5.4;
            return pathLoss;
        }

        /// <summary>
        /// Calculates path loss using fifth equation
        /// </summary>
        /// <param name="distance">Distance in km</param>
        /// <param name="frequency">frequency in MHz</param>
        /// <param name="heightMaster">master height in meters</param>
        /// <param name="heightSlave">slave height in meters</param>
        /// <returns>returns path loss</returns>
        public static double Calculate40to100Pathloss(double distance, double frequency, double heightMaster, double heightSlave)
        {
            // calculate distance using equation when distance between 0.04 to 0.1 km
            double pathLoss = 0;
            double freePathLoss = CalculateFreePathloss(0.04, frequency, heightMaster, heightSlave);
            double openPathLoss = CalculateOpenPathloss(0.1, frequency, heightMaster, heightSlave);
            double nummerator, denominator, multiplier;
            nummerator = Math.Log10(distance) - Math.Log10(0.04);
            denominator = Math.Log10(0.1) - Math.Log10(0.04);
            multiplier = openPathLoss - freePathLoss;

            ////pathLoss = freePathLoss + ((Math.Log10(distance) - Math.Log10(0.04) * (openPathLoss - freePathLoss)) / (Math.Log10(0.1) - Math.Log10(0.04)));
            pathLoss = freePathLoss + ((nummerator / denominator) * multiplier);

            return pathLoss;
        }

        /// <summary>
        /// Calculates path loss using fifth equation
        /// </summary>
        /// <param name="distance">Distance in km</param>
        /// <param name="frequency">frequency in MHz</param>
        /// <param name="heightMaster">master height in meters</param>
        /// <param name="heightSlave">slave height in meters</param>
        /// /// <param name="clutterEnvironment">type of environment (Suburban, Urban, Open...)</param>
        /// <returns>returns path loss</returns>
        public static double CalculateMidPathloss(double distance, double frequency, double heightMaster, double heightSlave, ClutterEnvironment clutterEnvironment)
        {
            // calculate distance using equation when distance between 0.04 to 0.1 km
            double pathLoss = 0;

            double freePathLoss = CalculateFreePathloss(0.04, frequency, heightMaster, heightSlave);
            double openPathLoss = CalculatePathLossUsingClutter(0.1, frequency, heightMaster, heightSlave, clutterEnvironment);

            double nummerator, denominator, multiplier;
            nummerator = Math.Log10(distance) - Math.Log10(0.04);
            denominator = Math.Log10(0.1) - Math.Log10(0.04);
            multiplier = openPathLoss - freePathLoss;

            ////pathLoss = freePathLoss + ((Math.Log10(distance) - Math.Log10(0.04) * (openPathLoss - freePathLoss)) / (Math.Log10(0.1) - Math.Log10(0.04)));
            pathLoss = freePathLoss + ((nummerator / denominator) * multiplier);

            return pathLoss;
        }

        /// <summary>
        /// Calculates the contour of the specified incumbent.
        /// </summary>
        /// <param name="incumbent">Incumbent that is to be calculated.</param>
        /// <returns>Returns the contour of the calculations.</returns>
        [ExcludeFromCodeCoverage]
        public Contour CalculateContour(Incumbent incumbent)
        {
            return null;
        }

        /// <summary>
        /// Calculates the Hat of the incumbent.
        /// </summary>
        /// <param name="incumbent">The incumbent that is to be calculated.</param>
        /// <returns>Returns the Hat</returns>
        [ExcludeFromCodeCoverage]
        public RadialHAT CalculateRadialHAT(Incumbent incumbent)
        {
            return null;
        }

        /// <summary>
        /// Calculates the station HAAT.
        /// </summary>
        /// <param name="sourceLocation">The source location.</param>
        /// <returns>returns RadialHAT.</returns>
        [ExcludeFromCodeCoverage]
        public RadialHAT CalculateStationHAAT(Location sourceLocation)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Determines the channel availability for each channel at the specified position.
        /// </summary>
        /// <param name="position">Location that is determine channel availability.</param>
        /// <returns>Returns all of the channels and their availability status.</returns>
        [ExcludeFromCodeCoverage]
        public ChannelInfo[] GetChannelAvailabilty(Position position)
        {
            return null;
        }

        /// <summary>
        /// Returns only the free channels at the specified location.
        /// </summary>
        /// <param name="wsdInfo">The WSD information.</param>
        /// <returns>Returns only the free channels.</returns>
        public GetChannelsResponse GetFreeChannels(Incumbent wsdInfo)
        {
            Stopwatch watch = new Stopwatch();
            watch.Start();
            List<int> blockedChannels = new List<int>();

            wsdInfo.LogDetails = false;

            //// kill switch for no further processing
            if (Utils.Configuration["WSDBEnabled"].ToLower() == "false")
            {
                return new GetChannelsResponse() { ChannelsInfo = new ChannelInfo[0] };
            }

            ////this.ExcludeRegions(wsdInfo, blockedChannels);

            var wsdEastingNorthing = GeoCalculations.GetEastingNorthingWithDoublePrecision(wsdInfo.Location);
            wsdInfo.OSGLocation = wsdEastingNorthing;

            if ((wsdInfo.HeightType == HeightType.ASL || wsdInfo.HeightType == HeightType.AMSL) && wsdInfo.Height > 0)
            {
                var terrainElev = this.TerrainElevation.CalculateElevation(wsdEastingNorthing.Easting, wsdEastingNorthing.Northing);
                wsdInfo.Height = Math.Max(wsdInfo.Height - terrainElev, 1.5);
            }

            Tuple<int[], double[]> result = null;

            if (!string.IsNullOrEmpty(wsdInfo.RequestType) && wsdInfo.RequestType.ToLower() == "generic")
            {
                result = this.CalculateGenericParameters(wsdInfo);
            }
            else
            {
                result = this.CalculateSpecificParameters(wsdInfo);
            }

            wsdInfo.TestingOutput = new IntermediateResults2(result.Item1, result.Item2);

            List<ChannelInfo> channelList = new List<ChannelInfo>();

            // Add channel information for 8 Mhz bandwidth for DTT
            for (int i = 21; i <= 60; i++)
            {
                var minMaxFreq = Conversion.GetMinMaxFreqForDTTChannel(i, 8);

                double maxPower = result.Item1[i - 21];
                if (blockedChannels.Contains(i))
                {
                    maxPower = double.NaN;
                }

                ChannelInfo channelInfo = new ChannelInfo()
                {
                    ChannelId = i,
                    StartHz = minMaxFreq.Item1,
                    StopHz = minMaxFreq.Item2,
                    Bandwidth = 8,
                    MaxPowerDBm = maxPower
                };

                channelList.Add(channelInfo);
            }

            // Add channel information for .1 Mhz bandwidth i.e. PMSE
            for (int i = 21; i <= 60; i++)
            {
                double maxPower = result.Item2[i - 21];
                if (blockedChannels.Contains(i))
                {
                    maxPower = double.NaN;
                }

                var minMaxFreq = Conversion.GetMinMaxFreqForDTTChannel(i, .1);

                ChannelInfo channelInfo = new ChannelInfo()
                {
                    ChannelId = i,
                    DeviceType = wsdInfo.IncumbentType.ToString(),
                    StartHz = minMaxFreq.Item1,
                    StopHz = minMaxFreq.Item2,
                    Bandwidth = .1,
                    MaxPowerDBm = maxPower
                };

                channelList.Add(channelInfo);
            }

            watch.Stop();

            var testingResponse = this.ProcessChannelsInCSVFormat(wsdInfo);
            var response = new GetChannelsResponse()
            {
                ChannelsInfo = channelList.ToArray(),
                ChannelsInCSVFormat = testingResponse.Item1
            };

            response.IntermediateResult1 = testingResponse.Item2;
            response.IntermediateResult2 = testingResponse.Item3;

            return response;
        }

        /// <summary>Returns only the devices at the specified location.</summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>Returns the devices.</returns>
        [ExcludeFromCodeCoverage]
        public List<ProtectedDevice> GetDeviceList(Parameters parameters)
        {
            return null;
        }

        /// <summary>
        /// Reads the PMSE file.
        /// </summary>
        /// <param name="fileName">Name of the file.</param>
        /// <returns>returns PMSE list.</returns>
        [ExcludeFromCodeCoverage]
        public List<PmseAssignment> ReadPMSEFile(string fileName)
        {
            ////todo only for ofcom testing phase. remove after that
            string fileFormat = @"D:\whitespace\{0}.csv";
            List<PmseAssignment> lstPmseAssignments = new List<PmseAssignment>();
            try
            {
                OSGLocation assignmentLocation;
                PmseAssignment pmseAssignment;
                string[] values;

                foreach (var line in File.ReadAllLines(string.Format(fileFormat, fileName)))
                {
                    values = line.Split(',');
                    if (values[0] == "Assignment_ID" || string.IsNullOrEmpty(values[0]))
                    {
                        continue;
                    }

                    assignmentLocation = Conversion.RoundTo10(values[2].ToDouble(), values[3].ToDouble());
                    pmseAssignment = new PmseAssignment();
                    pmseAssignment.PartitionKey = "PMSE";
                    pmseAssignment.RowKey = values[0];
                    pmseAssignment.Assignment_ID = values[0];
                    pmseAssignment.Equipment_Type_ID = values[1];
                    pmseAssignment.OriginalEasting = values[2].ToDouble();
                    pmseAssignment.OriginalNorthing = values[3].ToDouble();
                    pmseAssignment.Easting = assignmentLocation.Easting;
                    pmseAssignment.Northing = assignmentLocation.Northing;
                    pmseAssignment.AntennaHeightMetres = values[4].ToDouble();
                    pmseAssignment.FrequencyMHz = values[5].ToDouble();
                    pmseAssignment.BandwidthMHz = values[6].ToDouble();
                    pmseAssignment.Start = values[7];
                    pmseAssignment.Finish = values[8];
                    pmseAssignment.SituationID = values[9];
                    pmseAssignment.Channel = Conversion.DTTFrequencyToChannel(pmseAssignment.FrequencyMHz);
                    if (values.Length > 10)
                    {
                        pmseAssignment.ClutterValue = values[10].ToInt32();
                    }
                    else
                    {
                        pmseAssignment.ClutterValue = 1;
                    }

                    lstPmseAssignments.Add(pmseAssignment);
                }
            }
            catch (Exception)
            {
            }

            return lstPmseAssignments;
        }

        /// <summary>
        /// Calculates the coverage distance.
        /// </summary>
        /// <param name="wsdInfo">The WSD information.</param>
        /// <returns>returns System.Double.</returns>
        public double CalculateCoverageDistance(Incumbent wsdInfo)
        {
            var masterOperationalParams = this.CalculateMasterOperationalParameters(wsdInfo);
            wsdInfo.Channel = masterOperationalParams.Item1;

            // default 
            var maxMasterEirp = masterOperationalParams.Item2;
            var prefsensSlave = 0.0;

            prefsensSlave = Utils.Configuration[Constants.ConfigSettingDefaultPREFSENSDeviceParam].ToDouble();

            if (wsdInfo.PREFSENS != 0.0)
            {
                prefsensSlave = wsdInfo.PREFSENS;
            }

            var slaveGA = 0.0;

            if (wsdInfo.IncumbentType == IncumbentType.TypeA)
            {
                slaveGA = Utils.Configuration[Constants.ConfigSettingAntennaGainTypeA].ToDouble();
            }
            else
            {
                slaveGA = Utils.Configuration[Constants.ConfigSettingAntennaGainTypeB].ToDouble();
            }

            var medianGainMin = prefsensSlave - maxMasterEirp;
            var pathLoss = medianGainMin - slaveGA;

            // master transmitter height
            var heightMaster = 0.0;
            if (wsdInfo.Height != 0.0)
            {
                heightMaster = wsdInfo.Height;
            }
            else
            {
                if (wsdInfo.IncumbentType == IncumbentType.TypeA)
                {
                    heightMaster = 30;
                }
                else
                {
                    heightMaster = 1.5;
                }
            }

            // slave receiver height
            var heightSlave = 0.0;
            if (wsdInfo.IncumbentType == IncumbentType.TypeA)
            {
                heightSlave = 10;
            }
            else
            {
                heightSlave = 1.5;
            }

            // startFrequecy + 4
            double centerFrequency = Conversion.DTTChannelToFrequency(wsdInfo.Channel) + 4;

            double medianPathGainOfd0 = -CalculateFreePathloss(0.0, centerFrequency, heightMaster, heightSlave);

            if (pathLoss > medianPathGainOfd0)
            {
                return 0;
            }

            double coverageDistance = 0.0;
            if (!double.IsInfinity(pathLoss))
            {
                if (wsdInfo.TestingStage >= 5 || wsdInfo.TestingStage == 0)
                {
                    wsdInfo.WsdClutterType = this.ClutterReader.CalculateClutter(wsdInfo.OSGLocation.Easting, wsdInfo.OSGLocation.Northing);
                    coverageDistance = this.CalculateDistanceFromPathLossUsingClutter(Math.Abs(pathLoss), heightMaster, heightSlave, centerFrequency, wsdInfo.WsdClutterType).InMeter();
                }
                else
                {
                    coverageDistance = this.CalculateDistanceFromPathLoss(Math.Abs(pathLoss), heightMaster, heightSlave, centerFrequency).InMeter();
                }
            }

            return coverageDistance;
        }

        [ExcludeFromCodeCoverage]
        public void DumpFile<T>(List<T> table, string fileName, string seperator = ",")
        {
            StringBuilder headerBuilder = new StringBuilder();

            if (table.Count == 0)
            {
                return;
            }

            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(table[0]);

            foreach (PropertyDescriptor entityProperty in properties)
            {
                headerBuilder.AppendFormat("{0}{1}", entityProperty.Name, seperator);
            }

            List<string> rows = new List<string>();

            rows.Add(headerBuilder.ToString());
            foreach (var item in table)
            {
                StringWriter writer = new StringWriter();
                foreach (PropertyDescriptor entityProperty in properties)
                {
                    object itemValue = null;

                    if (entityProperty.PropertyType.BaseType != null && entityProperty.PropertyType.BaseType == typeof(Array))
                    {
                        if (entityProperty.PropertyType == typeof(Int32[]))
                        {
                            itemValue = entityProperty.GetValue(item);
                            if (itemValue != null)
                            {
                                var itemArray = itemValue as Int32[];
                                itemValue = itemArray.Select(obj => obj.ToString()).Aggregate((a, b) => string.Concat(a, ",", b));
                            }
                            else
                            {
                                itemValue = string.Empty;
                            }
                        }
                    }
                    else
                    {
                        itemValue = entityProperty.GetValue(item);
                    }

                    if (itemValue != null)
                    {
                        itemValue = itemValue.ToString().Replace(',', '|');
                    }

                    writer.Write(itemValue);
                    writer.Write(seperator);
                }

                rows.Add(writer.ToString());
            }
        }

        private static void CalculatePMSEToCandidateLocationDistance(OSGLocation pmseLocation, OSGLocation candidateLocation, ref double minDistance)
        {
            double distance = Math.Sqrt(Math.Pow(Math.Abs(candidateLocation.Easting - pmseLocation.OriginalEasting), 2) + Math.Pow(Math.Abs(candidateLocation.Northing - pmseLocation.OriginalNorthing), 2));

            if (minDistance == -1.0 || distance < minDistance)
            {
                minDistance = distance;
            }
        }

        /// <summary>
        /// Converts integer Database clutter class to clutter profile.
        /// </summary>
        /// <param name="clutterType">Integer database clutter class.</param>
        /// <returns>Clutter profile.</returns>
        private static ClutterEnvironment GetClutterEnvironment(int clutterType)
        {
            ClutterEnvironment clutterEnviroment = ClutterEnvironment.None;

            if (clutterType >= 1 && clutterType <= 21)
            {
                clutterEnviroment = ClutterEnvironment.OpenArea;
            }
            else if (clutterType == 22)
            {
                clutterEnviroment = ClutterEnvironment.Urban;
            }
            else if (clutterType == 23)
            {
                clutterEnviroment = ClutterEnvironment.SubUrban;
            }

            return clutterEnviroment;
        }

        /// <summary>
        /// Calculates PathLoss using clutter profile.
        /// </summary>
        /// <param name="distance">Distance in KM.</param>
        /// <param name="frequency">Frequency in MHz.</param>
        /// <param name="heightMaster">Master WSD height in meter.</param>
        /// <param name="heightSlave">Slave WSD height in meter.</param>
        /// <param name="clutterProfile">Clutter profile.</param>
        /// <returns>path loss</returns>
        private static double CalculatePathLossUsingClutter(double distance, double frequency, double heightMaster, double heightSlave, ClutterEnvironment clutterProfile)
        {
            double pathLoss = 0.0;

            switch (clutterProfile)
            {
                case ClutterEnvironment.Urban:
                    pathLoss = CalculateUrbanPathloss(distance, frequency, heightMaster, heightSlave);
                    break;
                case ClutterEnvironment.SubUrban:
                    pathLoss = CalculateSubUrbanPathloss(distance, frequency, heightMaster, heightSlave);
                    break;
                case ClutterEnvironment.OpenArea:
                case ClutterEnvironment.None:
                    pathLoss = CalculateOpenPathloss(distance, frequency, heightMaster, heightSlave);
                    break;
            }

            return pathLoss;
        }

        /// <summary>
        /// Excludes the regions.
        /// </summary>
        /// <param name="incumbentInfo">The incumbent information.</param>
        /// <param name="blockedChannels">The blocked channels.</param>
        [ExcludeFromCodeCoverage]
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
                        blockedChannels.Add(spectrum.Channel.Value);
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
        /// Gets the height for device.
        /// </summary>
        /// <param name="deviceHeight">Height of the device.</param>
        /// <param name="isPMSE">if set to <c>true</c> [is PMSE].</param>
        /// <param name="incumbentType">Type of the incumbent.</param>
        /// <returns>returns System.Double.</returns>
        private double GetHeightForDevice(double deviceHeight, bool isPMSE, IncumbentType incumbentType)
        {
            double heightWSD = 0.0;

            if (isPMSE)
            {
                if (deviceHeight == 0)
                {
                    heightWSD = 5;
                }
                else
                {
                    heightWSD = deviceHeight;
                }

                return heightWSD;
            }

            if (deviceHeight > 0)
            {
                if (deviceHeight < 3.25)
                {
                    heightWSD = 1.5;
                }
                else if (deviceHeight >= 3.25 && deviceHeight < 7.5)
                {
                    heightWSD = 5;
                }
                else if (deviceHeight >= 7.5 && deviceHeight < 12.5)
                {
                    heightWSD = 10;
                }
                else if (deviceHeight >= 12.5 && deviceHeight <= 17.5)
                {
                    heightWSD = 15;
                }
                else if (deviceHeight >= 17.5 && deviceHeight < 25)
                {
                    heightWSD = 20;
                }
                else if (deviceHeight >= 25)
                {
                    heightWSD = 30;
                }
            }
            else
            {
                if (incumbentType == IncumbentType.TypeA)
                {
                    heightWSD = 30;
                }
                else
                {
                    heightWSD = 1.5;
                }
            }

            return heightWSD;
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
        /// Gets the maximum in block spectral density.
        /// </summary>
        /// <param name="wsd">The WSD.</param>
        /// <param name="pmse">The PMSE.</param>
        /// <param name="operationalParameter">The Operational parameter type.</param>
        /// <returns>returns System.Double.</returns>
        private double GetMaxInBlockSpectralDensity(Incumbent wsd, PmseAssignment pmse, OperationalParameter operationalParameter)
        {
            //// pIB maximum permitted WSD in-block EIRP spectral density, PIB in dBm/(100 kHz) 
            //// PS0 is the received wanted PMSE signal power (over bandwidth B),
            //// B is the nominal channel bandwidth of the PMSE device in kHz,
            //// mG is the WSD-PMSE median coupling gain,
            //// protectionRatio -> r(ΔF) is the WSD-PMSE protection ratio defined as the ratio of received wanted PMSE signal power 
            //// (in bandwidth B) over received unwanted WSD signal power (in 8 MHz) at the point of PMSE receiver failure,

            //// mP is the median path gain (< 0 dB),
            //// GW is the building penetration (wall) gain (<= 0 dB), and
            //// GA,PMSE is the PMSE receiver antenna gain (>= 0 dB).

            double pIB = 0.0, ps0 = 0.0, mG = 0.0, lambda = 0.0;
            double mP = 0.0, gW = 0.0, gainPMSE = 0.0;

            PMSEEquipmentType pmseUseCase = PMSEEquipmentType.None;
            Enum.TryParse<PMSEEquipmentType>(pmse.Equipment_Type_ID, true, out pmseUseCase);

            int wsdEmissionClass = wsd.EmissionClass.ToInt32();

            ps0 = OFCOMMatrix.PS0[pmseUseCase];

            var deltaF = Math.Min(Math.Abs(wsd.Channel - pmse.Channel), Math.Abs(wsd.Channel - pmse.CoChannel));
            var protectionRatio = OFCOMMatrix.GetProtectionRatio(pmseUseCase)[Math.Abs(deltaF)][wsdEmissionClass - 1];

            double heightWSD = this.GetHeightForDevice(wsd.Height, false, wsd.IncumbentType);

            double heightPMSE = this.GetHeightForDevice(pmse.AntennaHeightMetres, true, IncumbentType.None);
            double distance = wsd.MinimumDistance / 1000.0;
            double frequency = Conversion.DTTChannelToFrequency(pmse.Channel) + 4;
            ////double frequency = pmse.FrequencyMHz;

            ClutterEnvironment clutterEnvironment = ClutterEnvironment.None;

            if (wsd.TestingStage == 2)
            {
                clutterEnvironment = ClutterEnvironment.OpenArea;
            }
            else
            {
                if (pmse.ClutterValue >= 1 && pmse.ClutterValue <= 21)
                {
                    clutterEnvironment = ClutterEnvironment.OpenArea;
                }
                else if (pmse.ClutterValue == 22)
                {
                    clutterEnvironment = ClutterEnvironment.Urban;
                }
                else if (pmse.ClutterValue == 23)
                {
                    clutterEnvironment = ClutterEnvironment.SubUrban;
                }
                else
                {
                    clutterEnvironment = ClutterEnvironment.None;
                }
            }

            PMSEUsageNature pmseUsageNature = PMSEUsageNature.None;
            PMSEUsageNature wsdUsageNature = PMSEUsageNature.None;
            wsdUsageNature = this.GetWsdUsageNature(wsd, heightWSD);

            if (pmse.SituationID == "I")
            {
                pmseUsageNature = PMSEUsageNature.Indoor;
            }
            else if (pmse.SituationID == "E")
            {
                pmseUsageNature = PMSEUsageNature.Outdoor;
            }

            if (wsd.WSDUsageNature == (int)PMSEUsageNature.Indoor)
            {
                wsdUsageNature = PMSEUsageNature.Indoor;
            }
            else if (wsd.WSDUsageNature == (int)PMSEUsageNature.Outdoor)
            {
                wsdUsageNature = PMSEUsageNature.Outdoor;
            }

            //// table 2.3 Schedule 2
            if (distance <= .01 && operationalParameter == OperationalParameter.Generic)
            {
                gW = 0.0;
            }
            else
            {
                if (pmseUsageNature == PMSEUsageNature.Outdoor && wsdUsageNature == PMSEUsageNature.Outdoor)
                {
                    gW = 0;
                }
                else if ((pmseUsageNature == PMSEUsageNature.Indoor && wsdUsageNature == PMSEUsageNature.Outdoor) || (pmseUsageNature == PMSEUsageNature.Outdoor && wsdUsageNature == PMSEUsageNature.Indoor))
                {
                    gW = -7;
                }
                else if (pmseUsageNature == PMSEUsageNature.Indoor && wsdUsageNature == PMSEUsageNature.Indoor)
                {
                    gW = -14;
                }
            }

            ////if (pmse.Assignment_ID == "50081" && wsd.Channel == 50)
            ////{

            ////}

            if (distance <= .04 || wsdUsageNature == PMSEUsageNature.Airborne)
            {
                mP = -CalculateFreePathloss(distance, frequency, heightWSD, heightPMSE);
            }
            else if (distance >= 0.1)
            {
                var freePathloss = CalculateFreePathloss(distance, frequency, heightWSD, heightPMSE);

                if (wsd.TestingStage == 0 || wsd.TestingStage >= 5)
                {
                    var clutterPathLoss = 0.0;
                    if (clutterEnvironment == ClutterEnvironment.OpenArea)
                    {
                        clutterPathLoss = CalculateOpenPathloss(distance, frequency, heightWSD, heightPMSE);
                    }
                    else if (clutterEnvironment == ClutterEnvironment.SubUrban)
                    {
                        clutterPathLoss = CalculateSubUrbanPathloss(distance, frequency, heightWSD, heightPMSE);
                    }
                    else if (clutterEnvironment == ClutterEnvironment.Urban)
                    {
                        clutterPathLoss = CalculateUrbanPathloss(distance, frequency, heightWSD, heightPMSE);
                    }

                    ////mP = clutterPathLoss;
                    if (clutterPathLoss < freePathloss)
                    {
                        mP = -freePathloss;
                    }
                    else
                    {
                        mP = -clutterPathLoss;
                    }
                }
                else
                {
                    var openPathLoss = CalculateOpenPathloss(distance, frequency, heightWSD, heightPMSE);
                    if (openPathLoss < freePathloss)
                    {
                        mP = -freePathloss;
                    }
                    else
                    {
                        mP = -openPathLoss;
                    }
                }
            }
            else
            {
                var freePathloss = CalculateFreePathloss(distance, frequency, heightWSD, heightPMSE);
                var midPathLoss = CalculateMidPathloss(distance, frequency, heightWSD, heightPMSE, clutterEnvironment);

                if (midPathLoss < freePathloss)
                {
                    mP = -freePathloss;
                }
                else
                {
                    mP = -midPathLoss;
                }
            }

            mG = mP + gW + gainPMSE;
            pIB = ps0 - protectionRatio - mG - lambda - (10 * Math.Log10(80));

            return pIB;
        }

        /// <summary>
        /// Calculates the distance from path loss.
        /// </summary>
        /// <param name="pathLoss">The path loss.</param>
        /// <param name="masterHeight">Height of the master.</param>
        /// <param name="slaveHeight">Height of the slave.</param>
        /// <param name="frequency">The frequency.</param>
        /// <returns>returns Distance.</returns>
        private Distance CalculateDistanceFromPathLoss(double pathLoss, double masterHeight, double slaveHeight, double frequency)
        {
            double startDistance = 0;
            double endDistance = 100;
            double currentDistance = 0;
            double currentPathloss = 0;
            double freeSpacePathloss = 0.0;

            // Note; instead of just looping for 30, could go until (endDistance - startDistance) is smaller then some precision value.
            for (int i = 0; i < 30; i++)
            {
                currentPathloss = 0.0;

                // Pick a point halfway between start and end
                currentDistance = startDistance + ((endDistance - startDistance) / 2);
                freeSpacePathloss = CalculateFreePathloss(currentDistance, frequency, masterHeight, slaveHeight);

                if (currentDistance > 0.04 && currentDistance < 0.1)
                {
                    currentPathloss = Calculate40to100Pathloss(currentDistance, frequency, masterHeight, slaveHeight);
                }
                else
                {
                    currentPathloss = CalculateOpenPathloss(currentDistance, frequency, masterHeight, slaveHeight);
                }

                // When L is below the free space attenuation for the same distance, the free space attenuation should be used instead
                // i.e. when currentPathloss is below the freeSpacePathloss for the same distance, the freeSpacePathloss should be used instead
                currentPathloss = Math.Max(currentPathloss, freeSpacePathloss);

                // (currentPathloss == pathLoss)
                if (Math.Abs(currentPathloss - pathLoss) < 0.0001)
                {
                    break;
                }

                if (currentPathloss > pathLoss)
                {
                    endDistance = currentDistance;
                }
                else
                {
                    startDistance = currentDistance;
                }
            }

            return new Distance(currentDistance, DistanceUnit.KM);
        }

        /// <summary>
        /// Calculates the distance from path loss.
        /// </summary>
        /// <param name="pathLoss">The path loss.</param>
        /// <param name="masterHeight">Height of the master.</param>
        /// <param name="slaveHeight">Height of the slave.</param>
        /// <param name="frequency">The frequency.</param>
        /// <param name="clutterType">Type of the clutter.</param>
        /// <returns>returns Distance.</returns>
        private Distance CalculateDistanceFromPathLossUsingClutter(double pathLoss, double masterHeight, double slaveHeight, double frequency, int clutterType)
        {
            double endDistance = 100;
            double currentDistance = 0;
            double startDistance = 0;
            double currentPathloss = 0;
            double freeSpacePathloss = 0.0;

            ClutterEnvironment clutterEnviroment = GetClutterEnvironment(clutterType);

            //// Note; instead of just looping for 30, could go until (endDistance - startDistance) is smaller then some precision value.
            for (int i = 0; i < 30; i++)
            {
                currentPathloss = 0.0;

                // Pick a point halfway between start and end
                currentDistance = startDistance + ((endDistance - startDistance) / 2);

                freeSpacePathloss = CalculateFreePathloss(currentDistance, frequency, masterHeight, slaveHeight);

                if (currentDistance >= 0.1)
                {
                    currentPathloss = CalculatePathLossUsingClutter(currentDistance, frequency, masterHeight, slaveHeight, clutterEnviroment);
                }
                else if (currentDistance > 0.04 && currentDistance < 0.1)
                {
                    currentPathloss = CalculateMidPathloss(currentDistance, frequency, masterHeight, slaveHeight, clutterEnviroment);
                }

                currentPathloss = Math.Max(freeSpacePathloss, currentPathloss);

                // (currentPathloss == pathLoss)
                if (Math.Abs(currentPathloss - pathLoss) < 0.0001)
                {
                    break;
                }

                if (currentPathloss > pathLoss)
                {
                    endDistance = currentDistance;
                }
                else
                {
                    startDistance = currentDistance;
                }
            }

            return new Distance(currentDistance, DistanceUnit.KM);
        }

        /// <summary>
        /// Calculates the generic parameters.
        /// </summary>
        /// <param name="wsdInfo">The WSD information.</param>
        /// <returns>returns parameters.</returns>
        private Tuple<int[], double[]> CalculateGenericParameters(Incumbent wsdInfo)
        {
            StringBuilder resultCheck = null;
            var coverageArea = this.CalculateCoverageDistance(wsdInfo);
            var wsdOsgLocation = wsdInfo.OSGLocation;
            var logFactor = 10 * Math.Log10(80);
            dynamic adjustmentData = null;
            string unscheduledTableName = string.Empty;

            if (wsdInfo.TestingStage == 4)
            {
                unscheduledTableName = "Stage4UnscheduledAdjustments";
            }
            else if (wsdInfo.TestingStage == 0)
            {
                unscheduledTableName = Constants.UnscheduledAdjustmentsTableName;
            }

            //// for max in block eirp for 100khz
            List<double> valuesP0 = new List<double>();

            List<OSGLocation> coordinatePairsForWSD = GeoCalculations.GenerateOverlappingCoordinatesOf100MtrsForCoverageAreaUsingSquare(wsdInfo.Location, wsdInfo.Location.SemiMajorAxis, wsdInfo.Location.SemiMinorAxis, resultCheck, coverageArea);

            // for max in block eirp for 8 Mhz
            List<int> valuesP1 = new List<int>();

            if (wsdInfo.TestingStage == 1 || wsdInfo.TestingStage >= 3 || wsdInfo.TestingStage == 0)
            {
                Incumbent slaveWsdInfo = wsdInfo.Copy();
                if (wsdInfo.IncumbentType == IncumbentType.TypeA)
                {
                    slaveWsdInfo.Height = 0.0;
                }
                else if (wsdInfo.IncumbentType == IncumbentType.TypeB)
                {
                    slaveWsdInfo.Height = Utils.Configuration[Constants.ConfigSettingDefaultTypeBHeightDeviceParam].ToDouble();
                }

                slaveWsdInfo.EmissionClass = "5";

                var dttSourceTable = this.GetDttTableName(slaveWsdInfo);
                List<DttData> unscheduledAdjustments = null;
                if (wsdInfo.TestingStage == 4 || wsdInfo.TestingStage == 0)
                {
                    unscheduledAdjustments = this.DalcCommon.GetUnscheduledAdjustments(unscheduledTableName);
                }

                List<DttData> dttValues = this.DalcIncumbent.GetDTTDatasetValues(dttSourceTable, coordinatePairsForWSD);
                ////this.DumpFile(dttValues, "DttDatt_" + wsdInfo.UniqueId);

                if (unscheduledAdjustments != null && unscheduledAdjustments.Count > 0)
                {
                    adjustmentData = from datasetRow in dttValues
                                     join unschedule in unscheduledAdjustments on new { datasetRow.Easting, datasetRow.Northing } equals new { unschedule.Easting, unschedule.Northing }
                                     select new { objunscheduledRow = unschedule, objDatasetRow = datasetRow };

                    foreach (var adjustment in adjustmentData)
                    {
                        for (int i = 0; i < 40; i++)
                        {
                            adjustment.objDatasetRow.DataValues[i + 2] = adjustment.objDatasetRow.DataValues[i + 2] + adjustment.objunscheduledRow.DataValues[i];
                        }
                    }
                }

                for (int i = 0; i < dttValues.Count; i++)
                {
                    this.MapDttDatasetValues(valuesP1, dttValues[i].DataValues);
                }

                /* ////// Adjustment of +7db not to be done in case of Generic parameters /////////
                 * 
                bool checkHeightOfTypeB = wsdInfo.IncumbentType == IncumbentType.TypeB && wsdInfo.Height > 2;
                if (checkHeightOfTypeB)
                {
                    for (int i = 0; i < valuesP1.Count; i++)
                    {
                        if (valuesP1[i] != -998)
                        {
                            valuesP1[i] += 7;
                            valuesP1[i] = Math.Min(valuesP1[i], 36);
                        }
                    }
                }
                */

                if (wsdInfo.IsTestingStage)
                {
                    if (wsdInfo.TestingStage == 1 || (wsdInfo.TestingStage == 5 && (wsdInfo.PMSEAssignmentTable.Contains("11") || wsdInfo.PMSEAssignmentTable.Contains("12"))))
                    {
                        if (valuesP1.Count == 0)
                        {
                            valuesP1 = new List<int>[40].Select(obj => 36).ToList();
                        }

                        valuesP0 = valuesP1.Select(obj => (double)obj).ToList();
                    }
                }
            }

            List<Tuple<double, PmseAssignment>> pmseWithDistance = new List<Tuple<double, PmseAssignment>>();
            if (wsdInfo.TestingStage >= 2 || wsdInfo.TestingStage == 0)
            {
                if (wsdInfo.TestingStage == 2)
                {
                    valuesP1 = new List<int>[40].Select(obj => 36).ToList();
                }

                string pmseTableName = wsdInfo.PMSEAssignmentTable;
                List<PmseAssignment> pmseDevices = null;
                if (wsdInfo.IsTestingStage)
                {
                    if (RoleEnvironment.IsAvailable)
                    {
                        pmseDevices = this.DalcCommon.FetchEntity<PmseAssignment>(pmseTableName, null);
                    }
                    else
                    {
                        if ((wsdInfo.TestingStage == 2 && pmseTableName.Contains("8B")) || wsdInfo.TestingStage == 3)
                        {
                            pmseDevices = this.ReadPMSEFile(pmseTableName);
                        }
                        else if (wsdInfo.TestingStage == 6 && pmseTableName.Contains("17"))
                        {
                            pmseDevices = this.ReadPMSEFile(pmseTableName);
                        }
                        else
                        {
                            pmseDevices = this.DalcCommon.FetchEntity<PmseAssignment>(pmseTableName, null);
                        }
                    }
                }
                else
                {
                    pmseDevices = DatabaseCache.ServiceCacheHelper.GetServiceCacheObjects(ServiceCacheObjectType.PMSEAssignment, null) as List<PmseAssignment>;
                }

                var slaveWsdInfo = wsdInfo.Copy();
                if (wsdInfo.IncumbentType == IncumbentType.TypeA)
                {
                    slaveWsdInfo.Height = 30;
                }
                else if (wsdInfo.IncumbentType == IncumbentType.TypeB)
                {
                    slaveWsdInfo.Height = 1.5;
                }

                slaveWsdInfo.EmissionClass = "5";

                // (TODO): As Offline staging (i.e From Stage 1  to 6) PMSE assignments has stored date time in the format "dd/MM/yyyy HH:mm" and the simulated tests
                // PMSEassignments date time are stored in the format "yyyy-MM-dd hh:mm". We suppose to store date time in one common format                  
                string dateTimeFormat = (wsdInfo.TestingStage > 0 && wsdInfo.TestingStage <= 6) ? OfcomCalculation.PMSEDateTimeFormat : null;

                var validpmseDevices = pmseDevices.Where(obj => wsdInfo.StartTime >= obj.Start.ToDateTime(dateTimeFormat) && wsdInfo.StartTime < obj.Finish.ToDateTime(dateTimeFormat)).ToList();

                List<OSGLocation>[] candidateLocationSectors = null;
                OSGLocation wsdRoundTo10 = Conversion.RoundTo10(wsdInfo.OSGLocation);

                // Skip candidate location generation if there are no PMSE Victims to deal with.
                if (validpmseDevices.Count != 0)
                {
                    candidateLocationSectors = GeoCalculations.GenerateWsdCandidateLocationOf10MtrsForCoverageAreaBoundary(wsdInfo.Location, wsdInfo.SemiMajorAxis, wsdInfo.SemiMinorAxis, coverageArea);
                }

                Dictionary<int, double> channelValues = new Dictionary<int, double>();
                List<dynamic> recordPMSE = new List<dynamic>();
                HashSet<int> unscheduledCoordinates = new HashSet<int>();
                List<OSGLocation> candidatePixels = new List<OSGLocation>();

                double radius = Math.Sqrt(Math.Pow(wsdInfo.SemiMajorAxis, 2) + Math.Pow(wsdInfo.SemiMinorAxis, 2)) + coverageArea;

                if (adjustmentData != null)
                {
                    foreach (var adjustment in adjustmentData)
                    {
                        candidatePixels.Add(new OSGLocation(adjustment.objDatasetRow.DataValues[0], adjustment.objDatasetRow.DataValues[1]));
                        unscheduledCoordinates.Add(string.Concat(adjustment.objDatasetRow.DataValues[0], ",", adjustment.objDatasetRow.DataValues[1]).GetHashCode());
                    }
                }
                else
                {
                    candidatePixels.Add(wsdOsgLocation);
                }

                foreach (var candidatePixel in candidatePixels)
                {
                    int[] unscheduledData = null;
                    if (wsdInfo.TestingStage == 4 || wsdInfo.TestingStage == 0)
                    {
                        if (adjustmentData != null)
                        {
                            foreach (var adjustment in adjustmentData)
                            {
                                var datasetData = adjustment.objDatasetRow;
                                if (datasetData.DataValues[0] == candidatePixel.Easting && datasetData.DataValues[1] == candidatePixel.Northing)
                                {
                                    unscheduledData = adjustment.objunscheduledRow.DataValues;
                                    break;
                                }
                            }
                        }
                    }

                    Dictionary<int, double> pixelPmseValues = new Dictionary<int, double>();
                    Dictionary<int, double> channelToVictimPmseDistance = new Dictionary<int, double>();

                    foreach (var pmseDevice in validpmseDevices)
                    {
                        double mindistance = -1.0;
                        OSGLocation pmseLocation = new OSGLocation(pmseDevice.OriginalEasting, pmseDevice.OriginalNorthing);

                        double distanceFromWsd = Math.Sqrt(Math.Pow(Math.Abs(wsdOsgLocation.Easting - pmseDevice.OriginalEasting), 2) + Math.Pow(Math.Abs(wsdOsgLocation.Northing - pmseDevice.OriginalNorthing), 2));

                        if (distanceFromWsd > radius)
                        {
                            int sectorIndex = GeoCalculations.FindCandidateLocationSectorIndex(pmseLocation, wsdRoundTo10);

                            // When coverage radius is 0, all the candidate locations i.e 4 point of 10 offset belong to 2nd quarter of circle.
                            if (radius == 0.0)
                            {
                                sectorIndex = 1;
                            }

                            if (candidateLocationSectors != null)
                            {
                                foreach (OSGLocation candiateLocation in candidateLocationSectors[sectorIndex])
                                {
                                    CalculatePMSEToCandidateLocationDistance(pmseLocation, candiateLocation, ref mindistance);
                                }
                            }
                        }
                        else
                        {
                            mindistance = 10;
                        }

                        if (mindistance == 0.0 || mindistance < 10)
                        {
                            mindistance = 10;
                        }

                        pmseWithDistance.Add(new Tuple<double, PmseAssignment>(mindistance, pmseDevice));
                        pmseDevice.CoChannel = Conversion.CalculateCoChannelForPMSE(pmseDevice);
                        slaveWsdInfo.MinimumDistance = mindistance;

                        // NB: This condition is not required if the PMSE Assignment table has pre-computed clutter environment from the terrain data.
                        if (wsdInfo.TestingStage >= 5 || wsdInfo.TestingStage == 0)
                        {
                            pmseDevice.ClutterValue = this.ClutterReader.CalculateClutter(pmseDevice.Easting, pmseDevice.Northing);
                        }

                        for (int channel = 21; channel <= 60; channel++)
                        {
                            var tempChannel = Math.Min(Math.Abs(channel - pmseDevice.Channel), Math.Abs(channel - pmseDevice.CoChannel));
                            slaveWsdInfo.Channel = channel;

                            if (tempChannel > 10)
                            {
                                continue;
                            }

                            // skip if adjacent channel ratio is greater than 10. then no protection needed
                            var tempps1Value = this.GetMaxInBlockSpectralDensity(slaveWsdInfo, pmseDevice, OperationalParameter.Generic);
                            if (pixelPmseValues.ContainsKey(channel))
                            {
                                pixelPmseValues[channel] = Math.Min(pixelPmseValues[channel], tempps1Value);
                            }
                            else
                            {
                                pixelPmseValues.Add(channel, tempps1Value);
                            }
                        }
                    }

                    this.AddChannelValues(pixelPmseValues, channelValues, unscheduledData);
                }

                ////this.DumpFile(recordPMSE, "testOutput_" + wsdInfo.UniqueId);

                valuesP0 = channelValues.OrderBy(obj => obj.Key).Select(obj => obj.Value).ToList();
            }

            if (wsdInfo.TestingStage == 1 || (wsdInfo.TestingStage == 5 && (wsdInfo.PMSEAssignmentTable.Contains("11") || wsdInfo.PMSEAssignmentTable.Contains("12"))))
            {
                wsdInfo.TestingIntermediateResults1 = new IntermediateResults1(coverageArea, 0);
            }
            else if (wsdInfo.TestingStage >= 2 || !wsdInfo.IsTestingStage)
            {
                wsdInfo.TestingIntermediateResults1 = new IntermediateResults1(coverageArea, pmseWithDistance.Count > 0 ? pmseWithDistance.Min(obj => obj.Item1) : 0, wsdInfo.WsdClutterType);
            }

            return new Tuple<int[], double[]>(valuesP1.ToArray(), valuesP0.ToArray());
        }

        /// <summary>
        /// Calculates the specific parameters.
        /// </summary>
        /// <param name="wsdInfo">The WSD information.</param>
        /// <returns>returns parameters.</returns>
        private Tuple<int[], double[]> CalculateSpecificParameters(Incumbent wsdInfo)
        {
            StringBuilder str = null;
            wsdInfo.LogDetails = false;

            var wsdEastingNorthing = GeoCalculations.GetEastingNorthingWithDoublePrecision(wsdInfo.Location);
            string unscheduledTableName = string.Empty;

            if (wsdInfo.TestingStage == 4)
            {
                unscheduledTableName = "Stage4UnscheduledAdjustments";
            }
            else if (wsdInfo.TestingStage == 0)
            {
                unscheduledTableName = Constants.UnscheduledAdjustmentsTableName;
            }

            // for max in block eirp for 100khz i.e. PMSE
            List<double> valuesP0 = new List<double>();

            //// for max in block eirp for 8 Mhz
            List<int> valuesP1 = new List<int>();

            var logFactor = 10 * Math.Log10(80);
            dynamic adjustmentData = null;
            List<OSGLocation> coordinatePairsForWSD = GeoCalculations.GenerateOverlappingCoordinatesOf100Mtrs(wsdEastingNorthing.Easting, wsdEastingNorthing.Northing, wsdInfo.Location.SemiMajorAxis, wsdInfo.Location.SemiMinorAxis, str);

            if (wsdInfo.TestingStage == 1 || wsdInfo.TestingStage >= 3 || wsdInfo.TestingStage == 0)
            {
                var dttSourceTable = this.GetDttTableName(wsdInfo);
                List<DttData> dttValues = this.DalcIncumbent.GetDTTDatasetValues(dttSourceTable, coordinatePairsForWSD);
                List<DttData> unscheduledAdjustments = null;
                if (wsdInfo.TestingStage == 4 || wsdInfo.TestingStage == 0)
                {
                    unscheduledAdjustments = this.DalcCommon.GetUnscheduledAdjustments(unscheduledTableName);
                }

                if (unscheduledAdjustments != null && unscheduledAdjustments.Count > 0)
                {
                    adjustmentData = from datasetRow in dttValues
                                     join unschedule in unscheduledAdjustments on new { datasetRow.Easting, datasetRow.Northing } equals new { unschedule.Easting, unschedule.Northing }
                                     select new { objunscheduledRow = unschedule, objDatasetRow = datasetRow };

                    foreach (var adjustment in adjustmentData)
                    {
                        for (int i = 0; i < 40; i++)
                        {
                            adjustment.objDatasetRow.DataValues[i + 2] = adjustment.objDatasetRow.DataValues[i + 2] + adjustment.objunscheduledRow.DataValues[i];
                        }
                    }
                }

                for (int i = 0; i < dttValues.Count; i++)
                {
                    this.MapDttDatasetValues(valuesP1, dttValues[i].DataValues);
                }

                bool checkHeightOfTypeB = wsdInfo.IncumbentType == IncumbentType.TypeB && wsdInfo.Height > 2;
                if (checkHeightOfTypeB)
                {
                    for (int i = 0; i < valuesP1.Count; i++)
                    {
                        if (valuesP1[i] != -998)
                        {
                            valuesP1[i] += 7;
                            if (wsdInfo.TestingStage != 4)
                            {
                                valuesP1[i] = Math.Min(valuesP1[i], 36);
                            }
                        }
                    }
                }

                if (valuesP1.Count == 0)
                {
                    valuesP1 = new List<int>[40].Select(obj => 36).ToList();
                }

                if (wsdInfo.IsTestingStage && wsdInfo.TestingStage == 1)
                {
                    valuesP0 = valuesP1.Select(obj => (double)obj).ToList();
                }
            }

            List<Tuple<double, PmseAssignment>> pmseWithDistance = new List<Tuple<double, PmseAssignment>>();
            if (wsdInfo.TestingStage >= 2 || wsdInfo.TestingStage == 0)
            {
                if (wsdInfo.TestingStage == 2)
                {
                    valuesP1 = new List<int>[40].Select(obj => 36).ToList();
                }

                List<OSGLocation> wsdcandidateLocations = GeoCalculations.GenerateWsdCandidateLocationOf10Mtrs(wsdEastingNorthing, wsdInfo.Location.SemiMajorAxis, wsdInfo.Location.SemiMinorAxis, str);
                string pmseTableName = wsdInfo.PMSEAssignmentTable;
                List<PmseAssignment> pmseDevices = null;
                if (wsdInfo.IsTestingStage)
                {
                    if (RoleEnvironment.IsAvailable)
                    {
                        pmseDevices = this.DalcCommon.FetchEntity<PmseAssignment>(pmseTableName, null);
                    }
                    else
                    {
                        if ((wsdInfo.TestingStage == 2 && pmseTableName.Contains("8B")) || wsdInfo.TestingStage == 3)
                        {
                            pmseDevices = this.ReadPMSEFile(pmseTableName);
                        }
                        else
                        {
                            pmseDevices = this.DalcCommon.FetchEntity<PmseAssignment>(pmseTableName, null);
                        }
                    }
                }
                else
                {
                    pmseDevices = DatabaseCache.ServiceCacheHelper.GetServiceCacheObjects(ServiceCacheObjectType.PMSEAssignment, null) as List<PmseAssignment>;
                }

                // (TODO): As Offline staging (i.e From Stage 1  to 6) PMSE assignments has stored date time in the format "dd/MM/yyyy HH:mm"and the simulated tests 
                // PMSE assignments date time are stored in the format "yyyy-MM-dd hh:mm". We suppose to store date time in one common format.
                string dateTimeFormat = (wsdInfo.TestingStage > 0 && wsdInfo.TestingStage <= 6) ? OfcomCalculation.PMSEDateTimeFormat : null;

                var validpmseDevices = pmseDevices.Where(obj => wsdInfo.StartTime >= obj.Start.ToDateTime(dateTimeFormat) && wsdInfo.StartTime < obj.Finish.ToDateTime(dateTimeFormat)).ToList();
                
                var slaveWsdInfo = wsdInfo.Copy();
                Dictionary<int, double> channelValues = new Dictionary<int, double>();
                if (string.IsNullOrEmpty(slaveWsdInfo.EmissionClass))
                {
                    slaveWsdInfo.EmissionClass = "5";
                }

                List<dynamic> recordPMSE = new List<dynamic>();
                HashSet<int> unscheduledCoordinates = new HashSet<int>();
                if (adjustmentData != null)
                {
                    foreach (var adjustment in adjustmentData)
                    {
                        unscheduledCoordinates.Add(string.Concat(adjustment.objDatasetRow.DataValues[0], ",", adjustment.objDatasetRow.DataValues[1]).GetHashCode());
                    }
                }

                foreach (var candidatePixel in coordinatePairsForWSD)
                {
                    int[] unscheduledData = null;
                    if (adjustmentData != null)
                    {
                        if (unscheduledCoordinates.Contains(string.Concat(candidatePixel.Easting, ",", candidatePixel.Northing).GetHashCode()))
                        {
                            foreach (var adjustment in adjustmentData)
                            {
                                var datasetData = adjustment.objDatasetRow;
                                if (datasetData.DataValues[0] == candidatePixel.Easting && datasetData.DataValues[1] == candidatePixel.Northing)
                                {
                                    unscheduledData = adjustment.objunscheduledRow.DataValues;
                                    break;
                                }
                            }
                        }
                    }

                    Dictionary<int, double> pixelPmseValues = new Dictionary<int, double>();
                    foreach (var pmseDevice in validpmseDevices)
                    {
                        ////var distanceFromCandidatePixel = Math.Sqrt(Math.Pow(Math.Abs(candidatePixel.Easting - pmseDevice.OriginalEasting), 2) + Math.Pow(Math.Abs(candidatePixel.Northing - pmseDevice.OriginalNorthing), 2));
                        var mindistance = -1.0;
                        foreach (var wsdcandidateLocation in wsdcandidateLocations)
                        {
                            var distance = Math.Sqrt(Math.Pow(Math.Abs(wsdcandidateLocation.Easting - pmseDevice.OriginalEasting), 2) + Math.Pow(Math.Abs(wsdcandidateLocation.Northing - pmseDevice.OriginalNorthing), 2));

                            if (mindistance == -1.0 || distance < mindistance)
                            {
                                mindistance = distance;
                            }
                        }

                        slaveWsdInfo.MinimumDistance = mindistance;
                        pmseDevice.CoChannel = Conversion.CalculateCoChannelForPMSE(pmseDevice);
                        pmseWithDistance.Add(new Tuple<double, PmseAssignment>(slaveWsdInfo.MinimumDistance, pmseDevice));

                        if (slaveWsdInfo.MinimumDistance == 0.0)
                        {
                            for (int channel = 21; channel <= 60; channel++)
                            {
                                if (!pixelPmseValues.ContainsKey(channel))
                                {
                                    pixelPmseValues.Add(channel, -999);
                                }
                                else
                                {
                                    pixelPmseValues[channel] = -999;
                                }
                            }

                            break;
                        }

                        // NB: This condition is not required if the PMSE Assignment table has pre-computed clutter environment from the terrain data.
                        if (wsdInfo.TestingStage >= 5 || wsdInfo.TestingStage == 0)
                        {
                            pmseDevice.ClutterValue = this.ClutterReader.CalculateClutter(pmseDevice.Easting, pmseDevice.Northing);
                        }

                        for (int channel = 21; channel <= 60; channel++)
                        {
                            var tempChannel = Math.Min(Math.Abs(channel - pmseDevice.Channel), Math.Abs(channel - pmseDevice.CoChannel));
                            slaveWsdInfo.Channel = channel;
                            if (tempChannel > 10)
                            {
                                continue;
                            }

                            // skip if adjacent channel ratio is greater than 10. then no protection needed
                            var tempps1Value = this.GetMaxInBlockSpectralDensity(slaveWsdInfo, pmseDevice, OperationalParameter.Specific);
                            if (pixelPmseValues.ContainsKey(channel))
                            {
                                pixelPmseValues[channel] = Math.Min(pixelPmseValues[channel], tempps1Value);
                            }
                            else
                            {
                                pixelPmseValues.Add(channel, tempps1Value);
                            }                          
                        }
                    }

                    this.AddChannelValues(pixelPmseValues, channelValues, unscheduledData);
                }

                ////this.DumpFile(recordPMSE, "testOutput");
                valuesP0 = channelValues.OrderBy(obj => obj.Key).Select(obj => obj.Value).ToList();
            }

            if (wsdInfo.TestingStage == 1 || (wsdInfo.TestingStage == 5 && (wsdInfo.PMSEAssignmentTable.Contains("11") || wsdInfo.PMSEAssignmentTable.Contains("12"))))
            {
                wsdInfo.TestingIntermediateResults1 = new IntermediateResults1(0, 0);
            }
            else if (wsdInfo.TestingStage >= 2 || !wsdInfo.IsTestingStage)
            {
                wsdInfo.TestingIntermediateResults1 = new IntermediateResults1(0, pmseWithDistance.Count > 0 ? pmseWithDistance.Min(obj => obj.Item1) : 0);
            }

            wsdInfo.TestingIntermediateResults2 = new IntermediateResults2(valuesP1.ToArray(), valuesP0.ToArray());

            return new Tuple<int[], double[]>(valuesP1.ToArray(), valuesP0.ToArray());
        }

        /// <summary>
        /// Calculates the master operational parameters.
        /// </summary>
        /// <param name="wsdInfo">The WSD information.</param>
        /// <returns>returns master operational parameters</returns>
        private Tuple<int, double> CalculateMasterOperationalParameters(Incumbent wsdInfo)
        {
            StringBuilder str = null;

            var wsdOsgLocation = wsdInfo.OSGLocation;

            // for max in block eirp for 100khz
            var valuesP0 = new List<double>();

            // for max in block eirp for 8 Mhz
            var valuesP1 = new List<int>();

            var logFactor = 10 * Math.Log10(80);
            dynamic adjustmentData = null;

            string unscheduledTableName = string.Empty;
            if (wsdInfo.TestingStage == 4)
            {
                unscheduledTableName = "Stage4UnscheduledAdjustments";
            }
            else if (wsdInfo.TestingStage == 0)
            {
                unscheduledTableName = Constants.UnscheduledAdjustmentsTableName;
            }

            if (wsdInfo.TestingStage == 1 || wsdInfo.TestingStage >= 3 || wsdInfo.TestingStage == 0)
            {
                List<OSGLocation> coordinatePairsForWSD = GeoCalculations.GenerateOverlappingCoordinatesOf100Mtrs(wsdOsgLocation.Easting, wsdOsgLocation.Northing, wsdInfo.Location.SemiMajorAxis, wsdInfo.Location.SemiMinorAxis, str);
                var dttSourceTable = this.GetDttTableName(wsdInfo);
                List<DttData> dttValues = this.DalcIncumbent.GetDTTDatasetValues(dttSourceTable, coordinatePairsForWSD);
                List<DttData> unscheduledAdjustments = null;
                if (wsdInfo.TestingStage == 4 || wsdInfo.TestingStage == 0)
                {
                    unscheduledAdjustments = this.DalcCommon.GetUnscheduledAdjustments(unscheduledTableName);
                }

                if (unscheduledAdjustments != null)
                {
                    adjustmentData = from datasetRow in dttValues
                                     join unschedule in unscheduledAdjustments on new { datasetRow.Easting, datasetRow.Northing } equals new { unschedule.Easting, unschedule.Northing }
                                     select new { objunscheduledRow = unschedule, objDatasetRow = datasetRow };

                    foreach (var adjustment in adjustmentData)
                    {
                        for (int i = 0; i < 40; i++)
                        {
                            adjustment.objDatasetRow.DataValues[i + 2] = adjustment.objDatasetRow.DataValues[i + 2] + adjustment.objunscheduledRow.DataValues[i];
                        }
                    }
                }

                for (int i = 0; i < dttValues.Count; i++)
                {
                    this.MapDttDatasetValues(valuesP1, dttValues[i].DataValues);
                }

                if (wsdInfo.DeviceCategory.ToLower() == "master" && wsdInfo.IncumbentType == IncumbentType.TypeB && wsdInfo.Height > 2)
                {
                    for (int i = 0; i < valuesP1.Count; i++)
                    {
                        if (valuesP1[i] != -998)
                        {
                            valuesP1[i] += 7;
                            valuesP1[i] = Math.Min(valuesP1[i], 36);
                        }
                    }
                }

                if (valuesP1.Count == 0)
                {
                    valuesP1 = new List<int>[40].Select(obj => 36).ToList();
                }

                if (wsdInfo.IsTestingStage && wsdInfo.TestingStage == 1)
                {
                    valuesP0 = valuesP1.Select(obj => (double)obj).ToList();
                }
            }

            if (wsdInfo.TestingStage >= 2 || wsdInfo.TestingStage == 0)
            {
                if (wsdInfo.TestingStage == 2)
                {
                    valuesP1 = new List<int>[40].Select(obj => 36).ToList();
                }

                Dictionary<int, double> channelValues = new Dictionary<int, double>();
                List<OSGLocation> wsdcandidateLocations = GeoCalculations.GenerateWsdCandidateLocationOf10Mtrs(wsdOsgLocation, wsdInfo.Location.SemiMajorAxis, wsdInfo.Location.SemiMinorAxis, str);
                string pmseTableName = wsdInfo.PMSEAssignmentTable;
                List<PmseAssignment> pmseDevices = null;
                if (wsdInfo.IsTestingStage)
                {
                    if (RoleEnvironment.IsAvailable)
                    {
                        pmseDevices = this.DalcCommon.FetchEntity<PmseAssignment>(pmseTableName, null);
                    }
                    else
                    {
                        if ((wsdInfo.TestingStage == 2 && pmseTableName.Contains("8B")) || wsdInfo.TestingStage == 3)
                        {
                            pmseDevices = this.ReadPMSEFile(pmseTableName);
                        }
                        else if (wsdInfo.TestingStage == 6 && pmseTableName.Contains("17"))
                        {
                            pmseDevices = this.ReadPMSEFile(pmseTableName);
                        }
                        else
                        {
                            pmseDevices = this.DalcCommon.FetchEntity<PmseAssignment>(pmseTableName, null);
                        }
                    }
                }
                else
                {
                    pmseDevices = DatabaseCache.ServiceCacheHelper.GetServiceCacheObjects(ServiceCacheObjectType.PMSEAssignment, null) as List<PmseAssignment>;
                }

                // (TODO): As Offline staging (i.e From Stage 1  to 6) PMSE assignments has stored date time in the format "dd/MM/yyyy HH:mm" and the simulated tests
                // PMSE assignments date time are stored in the format "yyyy-MM-dd hh:mm". We suppose to store date time in one common format.
                string dateTimeFormat = (wsdInfo.TestingStage > 0 && wsdInfo.TestingStage <= 6) ? OfcomCalculation.PMSEDateTimeFormat : null;

                var sameChannelPMSEDevices = pmseDevices.Where(obj => wsdInfo.StartTime >= obj.Start.ToDateTime(dateTimeFormat) && wsdInfo.StartTime < obj.Finish.ToDateTime(dateTimeFormat)).ToList();

                ////this.Logger.Log(TraceEventType.Critical, LoggingMessageId.RegionCalculationGenericMessage, string.Format("PMSE Devices:{0}", sameChannelPMSEDevices.Count));
                ////this.DumpFile(sameChannelPMSEDevices, "SelectedPMSE");
                var slaveWsdInfo = wsdInfo.Copy();

                foreach (var pmseDevice in sameChannelPMSEDevices)
                {
                    var minDistance = -1.0;
                    foreach (var wsdcandidateLocation in wsdcandidateLocations)
                    {
                        var distance = Math.Sqrt(Math.Pow(Math.Abs(wsdcandidateLocation.Easting - pmseDevice.OriginalEasting), 2) + Math.Pow(Math.Abs(wsdcandidateLocation.Northing - pmseDevice.OriginalNorthing), 2));
                        if (minDistance == -1.0 || distance < minDistance)
                        {
                            minDistance = distance;
                        }
                    }

                    if (minDistance > 5000)
                    {
                        continue;
                    }

                    slaveWsdInfo.MinimumDistance = minDistance;
                    pmseDevice.CoChannel = Conversion.CalculateCoChannelForPMSE(pmseDevice);

                    // make Pwsd-pmse 0 watt for less than 10mtr distance
                    if (slaveWsdInfo.MinimumDistance == 0)
                    {
                        for (int operatingChannel = 21; operatingChannel <= 60; operatingChannel++)
                        {
                            if (!channelValues.ContainsKey(operatingChannel))
                            {
                                channelValues.Add(operatingChannel, -998);
                            }
                            else
                            {
                                channelValues[operatingChannel] = -998;
                            }
                        }

                        break;
                    }

                    // NB: This condition is not required if the PMSE Assignment table has pre-computed clutter environment from the terrain data.
                    if (wsdInfo.TestingStage >= 5 || wsdInfo.TestingStage == 0)
                    {
                        pmseDevice.ClutterValue = this.ClutterReader.CalculateClutter(pmseDevice.Easting, pmseDevice.Northing);
                    }

                    for (int operatingChannel = 21; operatingChannel <= 60; operatingChannel++)
                    {
                        var tempChannel = Math.Min(Math.Abs(operatingChannel - pmseDevice.Channel), Math.Abs(operatingChannel - pmseDevice.CoChannel));
                        slaveWsdInfo.Channel = operatingChannel;
                        if (tempChannel > 10)
                        {
                            continue;
                        }

                        // skip if adjacent channel ratio is greater than 10. then no protection needed
                        var tempps1Value = this.GetMaxInBlockSpectralDensity(slaveWsdInfo, pmseDevice, OperationalParameter.Master);
                        if (!channelValues.ContainsKey(operatingChannel))
                        {
                            channelValues.Add(operatingChannel, tempps1Value);
                        }
                        else
                        {
                            channelValues[operatingChannel] = Math.Min(tempps1Value, channelValues[operatingChannel]);
                        }
                    }
                }

                //// maximum value for Pwsd-Pmse should be 16.9691 Schedule 2 Point 5
                for (int channel = 21; channel <= 60; channel++)
                {
                    var minvalue = 0.0;
                    if (channelValues.ContainsKey(channel))
                    {
                        minvalue = Math.Min(channelValues[channel], 16.9691);
                        channelValues[channel] = minvalue;
                    }
                    else
                    {
                        channelValues[channel] = 16.9691;
                    }
                }

                valuesP0 = channelValues.OrderBy(obj => obj.Key).Select(obj => obj.Value).ToList();
            }

            wsdInfo.TestingIntermediateResults2 = new IntermediateResults2(valuesP1.ToArray(), valuesP0.ToArray(), string.Empty);

            //// intially copy all dataset values to P0
            Dictionary<int, double> baseValues = new Dictionary<int, double>();
            for (int channel = 21; channel <= 60; channel++)
            {
                if (wsdInfo.TestingStage == 1 || (wsdInfo.TestingStage == 5 && (wsdInfo.PMSEAssignmentTable.Contains("11") || wsdInfo.PMSEAssignmentTable.Contains("12"))))
                {
                    if (valuesP1[channel - 21] == -998)
                    {
                        continue;
                    }

                    baseValues.Add(channel, valuesP1[channel - 21] - logFactor);
                }
                else if (wsdInfo.TestingStage == 2)
                {
                    if (valuesP0[channel - 21] == -998)
                    {
                        continue;
                    }

                    baseValues.Add(channel, valuesP0[channel - 21]);
                }
                else
                {
                    if (valuesP1[channel - 21] == -998 || valuesP0[channel - 21] == -998)
                    {
                        continue;
                    }

                    baseValues.Add(channel, Math.Min(valuesP1[channel - 21] - logFactor, valuesP0[channel - 21]));
                }
            }

            List<Tuple<int, double, double>> channelspectralValues = new List<Tuple<int, double, double>>();
            for (int channel = 21; channel <= 60; channel++)
            {
                if (!baseValues.ContainsKey(channel))
                {
                    continue;
                }

                var tempP0F = baseValues[channel];
                if (wsdInfo.MaxMasterEIRP != 0.0)
                {
                    tempP0F = Math.Min(wsdInfo.MaxMasterEIRP, tempP0F);
                }

                var frequency = Conversion.DTTChannelToFrequency(channel) + 4;
                var tempSpectralValue = tempP0F - (20 * Math.Log10(frequency));
                var spectralValue = new Tuple<int, double, double>(channel, tempP0F, tempSpectralValue);
                channelspectralValues.Add(spectralValue);
            }

            var p0FChannel = 21;
            var p0FSpectral = 0.0;
            if (channelspectralValues.Count > 0)
            {
                double maxValue = channelspectralValues.Max(obj => obj.Item3);
                var maxValueRecord = channelspectralValues.FirstOrDefault(obj => obj.Item3 == maxValue);
                p0FChannel = maxValueRecord.Item1;
                p0FSpectral = maxValueRecord.Item2;
            }
            else
            {
                p0FSpectral = double.NegativeInfinity;
            }

            return new Tuple<int, double>(p0FChannel, p0FSpectral);
        }

        /// <summary>
        /// Processes the channels in CSV format.
        /// </summary>
        /// <param name="wsdInfo">The WSD information.</param>
        /// <returns>returns System.String.</returns>
        [ExcludeFromCodeCoverage]
        private Tuple<string, string, string> ProcessChannelsInCSVFormat(Incumbent wsdInfo)
        {
            //// main output
            //// Unique_ID, Request_type, Time_start, Time_end, Channel21_P_PMSE, Channel21_P1 ... Channel60_P_PMSE, Channel60_P1

            //// intermediate results 1
            //// Unique_ID,Request_type,Time_start,Time_end,Easting,Northing,Distance to PMSE victim  (m),Radius of the Master WSD coverage area d0  (m)

            //// intermediate results 2
            //// Unique_ID,Request_type,Time_start,Time_end,Channel21_P_PMSE,Channel21_P1 ... Channel60_P_PMSE, Channel60_P1

            StringBuilder testOutput = new StringBuilder();
            const string FORMAT1 = "{0},";
            const string FORMAT2 = "{0},{1},";

            testOutput.AppendFormat(FORMAT1, wsdInfo.UniqueId);
            testOutput.AppendFormat(FORMAT1, wsdInfo.RequestType);
            testOutput.AppendFormat(FORMAT1, wsdInfo.StartTime.ToString("MM-dd-yyyy HH:mm"));
            testOutput.AppendFormat(FORMAT1, string.Empty);

            for (int i = 21; i <= 60; i++)
            {
                if (wsdInfo.TestingStage == 1)
                {
                    testOutput.AppendFormat(FORMAT2, "NaN", wsdInfo.TestingOutput.P1[i]);
                }
                else if (wsdInfo.TestingStage == 2)
                {
                    testOutput.AppendFormat(FORMAT2, wsdInfo.TestingOutput.PMSE[i], "NaN");
                }
                else if (wsdInfo.TestingStage >= 3 || !wsdInfo.IsTestingStage)
                {
                    testOutput.AppendFormat(FORMAT2, wsdInfo.TestingOutput.PMSE[i], wsdInfo.TestingOutput.P1[i]);
                }
            }

            StringBuilder intermediateResults1 = new StringBuilder();
            StringBuilder intermediateResults2 = new StringBuilder();

            intermediateResults1.AppendFormat(FORMAT1, wsdInfo.UniqueId);
            intermediateResults1.AppendFormat(FORMAT1, wsdInfo.RequestType);
            intermediateResults1.AppendFormat(FORMAT1, wsdInfo.StartTime.ToString("MM-dd-yyyy HH:mm"));
            intermediateResults1.AppendFormat(FORMAT1, string.Empty);
            intermediateResults1.AppendFormat(FORMAT1, wsdInfo.OSGLocation.OriginalEasting);
            intermediateResults1.AppendFormat(FORMAT1, wsdInfo.OSGLocation.OriginalNorthing);
            if (wsdInfo.TestingIntermediateResults1.DistanceToPmseVictim == 0.0)
            {
                intermediateResults1.AppendFormat(FORMAT1, string.Empty);
            }
            else
            {
                intermediateResults1.AppendFormat(FORMAT1, wsdInfo.TestingIntermediateResults1.DistanceToPmseVictim);
            }

            if (wsdInfo.TestingIntermediateResults1.CoverageArea == 0.0)
            {
                intermediateResults1.AppendFormat(FORMAT1, string.Empty);
            }
            else
            {
                intermediateResults1.AppendFormat(FORMAT1, wsdInfo.TestingIntermediateResults1.CoverageArea);
            }

            if (wsdInfo.TestingIntermediateResults1.ClutterType == 0)
            {
                intermediateResults1.AppendFormat(FORMAT1, string.Empty);
            }
            else
            {
                if (wsdInfo.TestingIntermediateResults1.ClutterType <= 21)
                {
                    intermediateResults1.AppendFormat(FORMAT1, "Open");
                }
                else if (wsdInfo.TestingIntermediateResults1.ClutterType == 22)
                {
                    intermediateResults1.AppendFormat(FORMAT1, "Urban");
                }
                else if (wsdInfo.TestingIntermediateResults1.ClutterType == 23)
                {
                    intermediateResults1.AppendFormat(FORMAT1, "SubUrban");
                }
            }

            intermediateResults2.AppendFormat(FORMAT1, wsdInfo.UniqueId);
            intermediateResults2.AppendFormat(FORMAT1, wsdInfo.RequestType);
            intermediateResults2.AppendFormat(FORMAT1, wsdInfo.StartTime.ToString("MM-dd-yyyy HH:mm"));
            intermediateResults2.AppendFormat(FORMAT1, string.Empty);

            for (int i = 21; i <= 60; i++)
            {
                if (wsdInfo.TestingStage == 1)
                {
                    intermediateResults2.AppendFormat(FORMAT2, "NaN", wsdInfo.TestingIntermediateResults2.P1[i]);
                }
                else if (wsdInfo.TestingStage == 2)
                {
                    intermediateResults2.AppendFormat(FORMAT2, wsdInfo.TestingIntermediateResults2.PMSE[i], "NaN");
                }
                else if (wsdInfo.TestingStage == 5 && (wsdInfo.PMSEAssignmentTable.Contains("11") || wsdInfo.PMSEAssignmentTable.Contains("12")))
                {
                    intermediateResults2.AppendFormat(FORMAT2, "NaN", wsdInfo.TestingIntermediateResults2.P1[i]);
                }
                else if (wsdInfo.TestingStage == 6 && wsdInfo.PMSEAssignmentTable.Contains("15"))
                {
                    intermediateResults2.AppendFormat(FORMAT2, "NaN", wsdInfo.TestingIntermediateResults2.P1[i]);
                }
                else
                {
                    intermediateResults2.AppendFormat(FORMAT2, wsdInfo.TestingIntermediateResults2.PMSE[i], wsdInfo.TestingIntermediateResults2.P1[i]);
                }
            }

            return new Tuple<string, string, string>(testOutput.ToString(), intermediateResults1.ToString(), intermediateResults2.ToString());
        }

        /// <summary>
        /// Gets the WSD usage nature.
        /// </summary>
        /// <param name="wsdInfo">The WSD information.</param>
        /// <param name="wsdHeight">The height of the WSD.</param>
        /// <returns>returns PMSEUsageNature.</returns>
        private PMSEUsageNature GetWsdUsageNature(Incumbent wsdInfo, double wsdHeight)
        {
            if (wsdInfo.RequestType.ToLower() == "generic" && wsdInfo.DeviceCategory.ToLower() == "slave")
            {
                return PMSEUsageNature.Outdoor;
            }
            else
            {
                if (wsdInfo.IncumbentType == IncumbentType.TypeA)
                {
                    return PMSEUsageNature.Outdoor;
                }
                else if (wsdInfo.IncumbentType == IncumbentType.TypeB)
                {
                    if (wsdInfo.Height > 2)
                    {
                        return PMSEUsageNature.Indoor;
                    }
                    else
                    {
                        return PMSEUsageNature.Outdoor;
                    }
                }
            }

            return PMSEUsageNature.None;
        }

        /// <summary>
        /// Adds the channel values.
        /// </summary>
        /// <param name="pixelPmseValues">The pixel pmse values.</param>
        /// <param name="channelValues">The channel values.</param>
        /// <param name="unscheduledData">The unscheduled data.</param>
        private void AddChannelValues(Dictionary<int, double> pixelPmseValues, Dictionary<int, double> channelValues, int[] unscheduledData = null)
        {
            for (int channel = 21; channel <= 60; channel++)
            {
                if (unscheduledData != null)
                {
                    if (pixelPmseValues.ContainsKey(channel))
                    {
                        if (unscheduledData[channel - 21] == 0)
                        {
                            pixelPmseValues[channel] = Math.Min(pixelPmseValues[channel], 16.9691);
                        }
                        else
                        {
                            pixelPmseValues[channel] = Math.Min(pixelPmseValues[channel], 16.9691) + unscheduledData[channel - 21];
                        }
                    }
                    else
                    {
                        if (unscheduledData[channel - 21] == 0)
                        {
                            pixelPmseValues.Add(channel, 16.9691);
                        }
                        else
                        {
                            pixelPmseValues.Add(channel, 16.9691 + unscheduledData[channel - 21]);
                        }
                    }
                }
                else
                {
                    if (pixelPmseValues.ContainsKey(channel))
                    {
                        pixelPmseValues[channel] = Math.Min(pixelPmseValues[channel], 16.9691);
                    }
                    else
                    {
                        pixelPmseValues.Add(channel, 16.9691);
                    }
                }
            }

            for (int channel = 21; channel <= 60; channel++)
            {
                if (channelValues.ContainsKey(channel))
                {
                    channelValues[channel] = Math.Min(channelValues[channel], pixelPmseValues[channel]);
                }
                else
                {
                    channelValues[channel] = pixelPmseValues[channel];
                }
            }
        }
    }
}
