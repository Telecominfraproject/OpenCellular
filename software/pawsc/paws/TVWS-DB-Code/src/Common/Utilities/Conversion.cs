// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.IO.Compression;
    using System.Linq;
    using System.Security.AccessControl;
    using System.Text;
    using System.Web;
    using Gavaghan.Geodesy;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.ServiceRuntime;
    using WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents Conversion.
    /// </summary>
    public static class Conversion
    {
        #region Distance Conversions

        /// <summary>
        /// Feet to meter.
        /// </summary>
        /// <param name="feet">The feet.</param>
        /// <returns>returns System.Double.</returns>
        public static double FeetToMeters(double feet)
        {
            return feet * 0.30479999798832;
        }

        /// <summary>
        /// Convert Feet to Miles
        /// </summary>
        /// <param name="feet">The feet.</param>
        /// <returns>returns System.Double.</returns>
        public static double FeetToMiles(double feet)
        {
            return feet / 5280;
        }

        /// <summary>
        /// Meter to feet.
        /// </summary>
        /// <param name="meters">The meters.</param>
        /// <returns>returns System.Double.</returns>
        public static double MetersToFeet(double meters)
        {
            return meters * 3.2808399166666664;
        }

        /// <summary>
        /// Miles to km.
        /// </summary>
        /// <param name="miles">The miles.</param>
        /// <returns>returns System.Double.</returns>
        public static double MilesToKm(double miles)
        {
            return miles * 1.609344;
        }

        /// <summary>
        /// Miles to feet.
        /// </summary>
        /// <param name="miles">The miles.</param>
        /// <returns>returns System.Double.</returns>
        public static double MilesToFeet(double miles)
        {
            return miles * 5280;
        }

        /// <summary>
        /// Miles to meters.
        /// </summary>
        /// <param name="miles">The miles.</param>
        /// <returns>returns System.Double.</returns>
        public static double MilesToMeters(double miles)
        {
            return MilesToKm(miles) * 1000;
        }

        /// <summary>
        /// Km to miles.
        /// </summary>
        /// <param name="km">The km.</param>
        /// <returns>returns System.Double.</returns>
        public static double KmToMiles(double km)
        {
            return km * 0.621371192;
        }

        /// <summary>
        /// Km to meters.
        /// </summary>
        /// <param name="km">The km.</param>
        /// <returns>returns System.Double.</returns>
        public static double KmToMeters(double km)
        {
            return km * 1000;
        }

        /// <summary>
        /// Meter to km.
        /// </summary>
        /// <param name="meters">The meters.</param>
        /// <returns>returns System.Double.</returns>
        public static double MetersToKm(double meters)
        {
            return meters / 1000.0;
        }

        #endregion

        #region Power/Gain Conversions

        /// <summary>
        /// Millis the watt to decibel.
        /// </summary>
        /// <param name="mW">The m w.</param>
        /// <returns>returns System.Double.</returns>
        public static double MilliWattToDecibel(double mW)
        {
            return 10 * Math.Log10(mW / 1);
        }

        /// <summary>
        /// Watts to decibel.
        /// </summary>
        /// <param name="w">The w.</param>
        /// <returns>returns System.Double.</returns>
        public static double WattToDecibel(double w)
        {
            return MilliWattToDecibel(w * 1000);
        }

        /// <summary>
        /// Converts Kw to decibel.
        /// </summary>
        /// <param name="kw">The kw.</param>
        /// <returns>returns System.Double.</returns>
        public static double KiloWattToDecibel(double kw)
        {
            return WattToDecibel(kw * 1000);
        }

        /// <summary>
        /// converts watt to decibel.
        /// </summary>
        /// <param name="mw">The mw.</param>
        /// <returns>returns decibel.</returns>
        public static double MegaWattToDecibel(double mw)
        {
            return KiloWattToDecibel(mw * 1000);
        }

        /// <summary>
        /// Decibels to MILLIWATT.
        /// </summary>
        /// <param name="db">The database.</param>
        /// <returns>returns mw.</returns>
        public static double DecibelToMilliWatt(double db)
        {
            return Math.Pow(10, db / 10);
        }

        /// <summary>
        /// Decibels to watt.
        /// </summary>
        /// <param name="db">The database.</param>
        /// <returns>returns watt.</returns>
        public static double DecibelToWatt(double db)
        {
            return Math.Pow(10, db / 10) / 1000;
        }

        /// <summary>
        /// Decibels to kilo watt.
        /// </summary>
        /// <param name="db">The database.</param>
        /// <returns>returns kw.</returns>
        public static double DecibelToKiloWatt(double db)
        {
            return Math.Pow(10, db / 10) / (1000 * 1000);
        }

        /// <summary>
        /// Decibels to mega watt.
        /// </summary>
        /// <param name="db">The database.</param>
        /// <returns>returns mw.</returns>
        public static double DecibelToMegaWatt(double db)
        {
            return Math.Pow(10, db / 10) / (1000 * 1000 * 1000);
        }

        /// <summary>
        /// Kilowatts to watt.
        /// </summary>
        /// <param name="kw">The kw.</param>
        /// <returns>returns kw.</returns>
        public static double KilowattToWatt(double kw)
        {
            return kw / 1000;
        }

        /// <summary>
        /// Watts to kilo watt.
        /// </summary>
        /// <param name="watt">The watt.</param>
        /// <returns>returns watt.</returns>
        public static double WattToKiloWatt(double watt)
        {
            return watt * 1000;
        }

        /// <summary>
        /// Watts to mega watt.
        /// </summary>
        /// <param name="watt">The watt.</param>
        /// <returns>returns megawatt.</returns>
        public static double WattToMegaWatt(double watt)
        {
            return watt * 1000 * 1000;
        }

        /// <summary>
        /// MW to watt.
        /// </summary>
        /// <param name="watt">The watt.</param>
        /// <returns>returns System.Double.</returns>
        public static double MegaWattToWatt(double watt)
        {
            return watt / 1000 / 1000;
        }
        #endregion

        /// <summary>
        /// converts DTV channel to frequency.
        /// </summary>
        /// <param name="channel">The channel.</param>
        /// <returns>returns System.Double.</returns>
        public static double DTVChannelToFrequency(int channel)
        {
            if (channel <= 6 && channel >= 2)
            {
                return ((channel - 2) * 6) + 54;
            }
            else if (channel >= 7 && channel <= 13)
            {
                return ((channel - 7) * 6) + 174;
            }
            else if (channel >= 14 && channel <= 65)
            {
                return ((channel - 14) * 6) + 470;
            }
            else
            {
                return 548.0;
            }
        }

        /// <summary>
        /// DTT channel to frequency.
        /// </summary>
        /// <param name="channel">The OFCOM channel.</param>
        /// <returns>returns frequency.</returns>
        public static double DTTChannelToFrequency(int channel)
        {
            double startingFreq = 470;
            int startingChannel = 21;

            return startingFreq + ((channel - startingChannel) * 8);
        }

        /// <summary>
        /// DTT frequency to channel.
        /// </summary>
        /// <param name="frequency">The frequency for OFCOM channel.</param>
        /// <returns>returns System.Double.</returns>
        public static int DTTFrequencyToChannel(double frequency)
        {
            double midFrequency = frequency - 470;
            const int StartingChannel = 21;
            var calculatedChannel = (midFrequency / 8) + StartingChannel;
            if (calculatedChannel == (int)calculatedChannel)
            {
                calculatedChannel = calculatedChannel - 1;
            }

            return (int)calculatedChannel;
        }

        /// <summary>
        /// Frequency to channel.
        /// </summary>
        /// <param name="frequency">The frequency.</param>
        /// <returns>returns channel.</returns>
        public static int FrequencyToChannel(double frequency)
        {
            int startingFrequency = 54;
            int startChannel = 2;
            const int Bandwidth = 6;

            if (frequency >= 54 && frequency <= 74)
            {
                return (((int)frequency - startingFrequency) / Bandwidth) + startChannel;
            }

            if (frequency >= 174 && frequency <= 216)
            {
                startingFrequency = 174;
                startChannel = 7;
                return (((int)frequency - startingFrequency) / Bandwidth) + startChannel;
            }

            if (frequency >= 470 && frequency <= 806)
            {
                startingFrequency = 470;
                startChannel = 14;
                return (((int)frequency - startingFrequency) / Bandwidth) + startChannel;
            }

            return 0;
        }

        /// <summary>
        /// Channels to contour database.
        /// </summary>
        /// <param name="channel">The channel.</param>
        /// <param name="vsdService">The VSD service.</param>
        /// <returns>returns contour DB.</returns>
        public static double ChannelToContourDb(int channel, string vsdService)
        {
            int band = ChannelToBand(channel);

            if (band == 1)
            {
                if (IsDigitalService(vsdService))
                {
                    return 28;
                }
                else
                {
                    return 47;
                }
            }
            else if (band == 2)
            {
                if (IsDigitalService(vsdService))
                {
                    return 36;
                }
                else
                {
                    return 56;
                }
            }
            else
            {
                if (IsDigitalService(vsdService))
                {
                    return 41;
                }
                else
                {
                    return 64;
                }
            }
        }

        /// <summary>
        /// Determines whether [is digital service] [the specified VSD service].
        /// </summary>
        /// <param name="vsdService">The VSD service.</param>
        /// <returns><c>true</c> if [is digital service] [the specified VSD service]; otherwise, <c>false</c>.</returns>
        public static bool IsDigitalService(string vsdService)
        {
            bool isDigital = vsdService == "DT" || vsdService == "DC" || vsdService == "DD" || vsdService == "LD";
            return isDigital;
        }

        /// <summary>
        /// To the type of the incumbent.
        /// </summary>
        /// <param name="incumbentType">Type of the incumbent.</param>
        /// <returns>returns IncumbentType.</returns>
        public static IncumbentType ToIncumbentType(string incumbentType)
        {
            IncumbentType incType = IncumbentType.None;
            Enum.TryParse<IncumbentType>(incumbentType, true, out incType);
            if (incType == IncumbentType.None && !string.IsNullOrWhiteSpace(incumbentType))
            {
                if (incumbentType.ToLower() == "a")
                {
                    incType = IncumbentType.TypeA;
                }
                else if (incumbentType.ToLower() == "b")
                {
                    incType = IncumbentType.TypeB;
                }
            }

            return incType;
        }

        /// <summary>
        /// Channel to band.
        /// </summary>
        /// <param name="channel">The channel.</param>
        /// <returns>returns band.</returns>
        public static int ChannelToBand(int channel)
        {
            int band = 1;

            if (channel > 6 && channel <= 13)
            {
                band = 2;
            }

            if (channel >= 14)
            {
                band = 3;
            }

            return band;
        }

        /// <summary>
        /// Gets the minimum and maximum frequency for channel.
        /// </summary>
        /// <param name="channel">The channel.</param>
        /// <returns>returns minimum and maximum Frequency.</returns>
        public static Tuple<double, double> GetMinMaxFreqForChannel(int channel)
        {
            double startFreq = 0;

            if (channel <= 6 && channel >= 2)
            {
                startFreq = ((channel - 2) * 6) + 54;
            }
            else if (channel >= 7 && channel <= 13)
            {
                startFreq = ((channel - 7) * 6) + 174;
            }
            else if (channel >= 14 && channel <= 65)
            {
                startFreq = ((channel - 14) * 6) + 470;
            }

            Tuple<double, double> tuple = new Tuple<double, double>(startFreq, startFreq + 6);

            return tuple;
        }

        /// <summary>
        /// Gets the minimum and maximum frequency for DTT OFCOM channel.
        /// </summary>
        /// <param name="channel">The channel.</param>
        /// <param name="bandwidth">The bandwidth in MHZ.</param>
        /// <returns>returns minimum and maximum Frequency.</returns>
        public static Tuple<double, double> GetMinMaxFreqForDTTChannel(int channel, double bandwidth)
        {
            double startFrequency = DTTChannelToFrequency(channel);

            return new Tuple<double, double>(startFrequency * Math.Pow(10, 6), (startFrequency + bandwidth) * Math.Pow(10, 6));
        }

        /// <summary>
        /// Patches Azimuth patterns for Antenna Patterns via linear interpolation.
        /// </summary>
        /// <param name="antennaPatterns">returns An array of field values for ever degree in a radial</param>
        public static void PatchAntennaPatterns(double[] antennaPatterns)
        {
            int firstNonZeroIndex = -1;
            int lastNonZeroIndex = -1;

            double delta = 0;

            // The 360th deg is the same as the 0th degree
            antennaPatterns[360] = antennaPatterns[0];

            for (int i = 0; i < 361; i++)
            {
                if (antennaPatterns[i] != 0)
                {
                    if (lastNonZeroIndex == -1)
                    {
                        lastNonZeroIndex = i;
                        firstNonZeroIndex = i;
                    }
                    else
                    {
                        delta = (antennaPatterns[i] - antennaPatterns[lastNonZeroIndex]) / (i - lastNonZeroIndex);

                        for (int j = lastNonZeroIndex + 1; j < i; j++)
                        {
                            /*
                            AntennaPatterns[j] = AntennaPatterns[j-1] +  
                                                 Math.Abs(AntennaPatterns[i] - AntennaPatterns[LastNonZeroIndex]) / 
                                                         (i - LastNonZeroIndex);
                             * */
                            antennaPatterns[j] = antennaPatterns[j - 1] + delta;
                        }

                        lastNonZeroIndex = i;
                    }
                }
            }

            // Wrap around and fix the last set of entries
            delta = (antennaPatterns[firstNonZeroIndex] - antennaPatterns[lastNonZeroIndex]) / ((360 - lastNonZeroIndex) + firstNonZeroIndex + 1);

            for (int i = lastNonZeroIndex + 1; i < 361; i++)
            {
                antennaPatterns[i] = antennaPatterns[i - 1] + delta;
            }

            if (antennaPatterns[0] == 0)
            {
                antennaPatterns[0] = antennaPatterns[360] + delta;

                for (int i = 1; i < firstNonZeroIndex; i++)
                {
                    antennaPatterns[i] = antennaPatterns[i - 1] + delta;
                }
            }
        }

        /// <summary>
        /// Converts the usage spectrum to incumbent.
        /// </summary>
        /// <param name="usedSpectrum">The used spectrum.</param>
        /// <returns>returns Incumbent.</returns>
        public static Incumbent ConvertUsageSpectrumToIncumbent(UsedSpectrum usedSpectrum)
        {
            if (usedSpectrum.Spectra.Profile != null)
            {
                return new Incumbent()
                {
                    Latitude = usedSpectrum.Location.Latitude,
                    Longitude = usedSpectrum.Location.Longitude,
                    Channel = usedSpectrum.Spectra.Profile.ChannelId.ToInt32(),
                    EmissionClass = usedSpectrum.DeviceDescriptor.EtsiEnDeviceEmissionsClass,
                    IncumbentType = ToIncumbentType(usedSpectrum.DeviceDescriptor.EtsiEnDeviceType)
                };
            }
            else
            {
                return new Incumbent()
                {
                    Latitude = usedSpectrum.Location.Latitude,
                    Longitude = usedSpectrum.Location.Longitude,
                    EmissionClass = usedSpectrum.DeviceDescriptor.EtsiEnDeviceEmissionsClass,
                    IncumbentType = ToIncumbentType(usedSpectrum.DeviceDescriptor.EtsiEnDeviceType)
                };
            }
        }

        /// <summary>
        /// Rounds the specified easting or northing to nearest value
        /// </summary>
        /// <param name="eastingOrNorthing">The easting or northing.</param>
        /// <param name="resolution">The resolution 100 or 10.</param>
        /// <returns>returns rounded value.</returns>
        public static int RoundToResolution(int eastingOrNorthing, int resolution)
        {
            return (int)Math.Round((double)eastingOrNorthing / resolution, 0) * resolution;
        }

        /// <summary>
        /// Rounds to resolution.
        /// </summary>
        /// <param name="eastingOrNorthing">The easting or northing.</param>
        /// <returns>returns rounded value.</returns>
        public static int RoundTo100(double eastingOrNorthing)
        {
            return (int)Math.Round(eastingOrNorthing / 100, 0) * 100;
        }

        /// <summary>
        /// Rounds the to100.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <returns>returns Location.</returns>
        public static OSGLocation RoundTo100(double easting, double northing)
        {
            string eastingStr = "0";
            if (easting != 0.0)
            {
                eastingStr = ((int)easting).ToString();
                eastingStr = eastingStr.Substring(0, eastingStr.Length - 2) + "00";
            }

            string northingStr = "0";
            if (northing != 0.0)
            {
                northingStr = ((int)northing).ToString();
                northingStr = northingStr.Substring(0, northingStr.Length - 2) + "00";
            }

            OSGLocation eastingNorthing = new OSGLocation(eastingStr.ToInt32(), northingStr.ToInt32());
            return eastingNorthing;
        }

        /// <summary>
        /// Rounds the to100.
        /// </summary>
        /// <param name="location">The location.</param>
        /// <returns>returns OSGLocation.</returns>
        public static OSGLocation RoundTo100(OSGLocation location)
        {
            return Conversion.RoundTo100(location.Easting, location.Northing);
        }

        /// <summary>
        /// Rounds to resolution of 10 meters.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <returns>returns rounded value.</returns>
        public static OSGLocation RoundTo10(double easting, double northing)
        {
            string eastingStr = "0";
            string northingStr = "0";
            if (easting != 0.0)
            {
                eastingStr = ((int)easting).ToString();
                eastingStr = eastingStr.Substring(0, eastingStr.Length - 1) + "0";
            }

            if (northing != 0.0)
            {
                northingStr = ((int)northing).ToString();
                northingStr = northingStr.Substring(0, northingStr.Length - 1) + "0";
            }

            OSGLocation eastingNorthing = new OSGLocation((eastingStr.Substring(0, eastingStr.Length - 1) + "0").ToInt32(), (northingStr.Substring(0, northingStr.Length - 1) + "0").ToInt32());
            return eastingNorthing;
        }

        /// <summary>
        /// Rounds to 10.
        /// </summary>
        /// <param name="location">The location.</param>
        /// <returns>returns OSGLocation.</returns>
        public static OSGLocation RoundTo10(OSGLocation location)
        {
            return Conversion.RoundTo10(location.Easting, location.Northing);
        }

        /// <summary>
        /// Rounds the specified easting or northing to nearest 100 meters
        /// </summary>
        /// <param name="eastingOrNorthing">The easting or northing.</param>
        /// <returns>returns rounded value.</returns>
        public static int RoundTo100(int eastingOrNorthing)
        {
            return RoundToResolution(eastingOrNorthing, 100);
        }

        /// <summary>
        /// Decompresses the DTT data.
        /// </summary>
        /// <param name="dttblockdata">The DTT block data.</param>
        /// <returns>returns DTTSquare.</returns>
        public static DTTSquare DecompressDTTData(DTTDataAvailability dttblockdata)
        {
            DTTSquare dttdata = null;
            using (MemoryStream ms = new MemoryStream())
            {
                int msgLength = BitConverter.ToInt32(dttblockdata.DataRecord, 0);
                ms.Write(dttblockdata.DataRecord, 0, dttblockdata.DataRecord.Length - 0);
                byte[] buffer = new byte[msgLength];
                ms.Position = 0;
                GZipStream zip = new GZipStream(ms, CompressionMode.Decompress);
                zip.Read(buffer, 0, buffer.Length);

                // Convert byte to integer array
                int[] block = new int[buffer.Length / sizeof(int)];
                Buffer.BlockCopy(buffer, 0, block, 0, block.Length);

                dttdata = new DTTSquare(dttblockdata.Easting, dttblockdata.Northing, block);
            }

            return dttdata;
        }

        /// <summary>
        /// Decompresses the DTT data.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>returns DTTSquare.</returns>
        public static int[] ByteToInt(byte[] data)
        {
            List<int> bytes = new List<int>();
            for (int i = 0; i < data.Length; i += 4)
            {
                bytes.Add(BitConverter.ToInt32(data, i));
            }

            return bytes.ToArray();
        }

        /// <summary>
        /// Compresses the DTT data.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>returns DTTSquare.</returns>
        public static byte[] IntToByte(int[] data)
        {
            List<byte> bytes = new List<byte>();
            for (int i = 0; i < data.Length; i++)
            {
                bytes.AddRange(BitConverter.GetBytes(data[i]));
            }

            return bytes.ToArray();
        }

        /// <summary>
        /// Calculates the co channel for pmse.
        /// </summary>
        /// <param name="pmseDevice">The Program Making and Special Events device.</param>
        /// <returns>returns integer</returns>
        public static int CalculateCoChannelForPMSE(PmseAssignment pmseDevice)
        {
            var pmseChannelCentreFrequency = Conversion.DTTChannelToFrequency(pmseDevice.Channel) + 4;
            var pmseLowerChannelUpperBound = Conversion.DTTChannelToFrequency(pmseDevice.Channel);
            var pmseUpperChannelLowerBound = Conversion.DTTChannelToFrequency(pmseDevice.Channel + 1);

            if (pmseDevice.FrequencyMHz + (pmseDevice.BandwidthMHz / 2.0) > pmseUpperChannelLowerBound)
            {
                return pmseDevice.Channel + 1;
            }

            if (pmseDevice.FrequencyMHz - (pmseDevice.BandwidthMHz / 2.0) < pmseLowerChannelUpperBound)
            {
                return pmseDevice.Channel - 1;
            }

            return 0;
        }
    }
}
