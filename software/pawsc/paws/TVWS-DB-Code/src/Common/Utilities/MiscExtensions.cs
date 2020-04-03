// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using Gavaghan.Geodesy;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>Represents Extensions</summary>
    public static class MiscExtensions
    {
        #region Location Extensions

        /// <summary>To the geo location.</summary>
        /// <param name="objLocation">The object location.</param>
        /// <returns>returns GeoLocation.</returns>
        public static GeoLocation ToGeoLocation(this Location objLocation)
        {
            return new GeoLocation()
                   {
                       Point = new Ellipse()
                               {
                                   Center = new Point()
                                            {
                                                Latitude = objLocation.Latitude.ToString(),
                                                Longitude = objLocation.Longitude.ToString()
                                            },
                               }
                   };
        }

        /// <summary>
        /// To the EASTING NORTHINGS.
        /// </summary>
        /// <param name="location">The location.</param>
        /// <returns>returns KeyValuePair{System.DoubleSystem.Double}.</returns>
        public static OSGLocation ToEastingNorthing(this Location location)
        {
            return GeoCalculations.GetEastingNorthing(location);
        }

        /// <summary>To the location.</summary>
        /// <param name="objGeoLocation">The object geo location.</param>
        /// <returns>returns Location.</returns>
        public static Location ToLocation(this GeoLocation objGeoLocation)
        {
            if (objGeoLocation != null && objGeoLocation.Point != null)
            {
                var location = new Location();
                if (objGeoLocation.Point.Center != null)
                {
                    if (!string.IsNullOrEmpty(objGeoLocation.Point.Center.Latitude))
                    {
                        location.Latitude = objGeoLocation.Point.Center.Latitude.ToDouble();
                    }

                    if (!string.IsNullOrEmpty(objGeoLocation.Point.Center.Longitude))
                    {
                        location.Longitude = objGeoLocation.Point.Center.Longitude.ToDouble();
                    }
                }

                location.SemiMajorAxis = objGeoLocation.Point.SemiMajorAxis.HasValue ? Convert.ToDouble(objGeoLocation.Point.SemiMajorAxis.Value) : 0.0;
                location.SemiMinorAxis = objGeoLocation.Point.SemiMinorAxis.HasValue ? Convert.ToDouble(objGeoLocation.Point.SemiMinorAxis.Value) : 0.0;

                return location;
            }

            if (objGeoLocation.Region != null && objGeoLocation.Region.Exterior != null && objGeoLocation.Region.Exterior.Length > 0)
            {
                return new Location()
                {
                    Latitude = double.Parse(objGeoLocation.Region.Exterior[0].Latitude),
                    Longitude = double.Parse(objGeoLocation.Region.Exterior[0].Longitude)
                };
            }

            return new Location(0.0, 0.0);
        }

        /// <summary>
        /// To the location.
        /// </summary>
        /// <param name="objGeoLocation">The object location.</param>
        /// <returns>returns Location.</returns>
        public static Location ToLocation(this Point objGeoLocation)
        {
            return new Location()
            {
                Latitude = objGeoLocation.Latitude.ToDouble(),
                Longitude = objGeoLocation.Longitude.ToDouble()
            };
        }

        /// <summary>
        /// To the point.
        /// </summary>
        /// <param name="objLocation">The object location.</param>
        /// <returns>returns Location.</returns>
        public static Point ToPoint(this Location objLocation)
        {
            return new Point()
            {
                Latitude = objLocation.Latitude.ToString("F13"),
                Longitude = objLocation.Longitude.ToString("F13"),
            };
        }

        /// <summary>
        /// To the location.
        /// </summary>
        /// <param name="objGeoLocation">The object location.</param>
        /// <returns>returns Location.</returns>
        public static Location ToLocation(this Position objGeoLocation)
        {
            return new Location()
            {
                Latitude = objGeoLocation.Latitude,
                Longitude = objGeoLocation.Longitude
            };
        }

        /// <summary>
        /// To the position.
        /// </summary>
        /// <param name="objPoint">The object point.</param>
        /// <returns>returns Position.</returns>
        public static Position ToPosition(this Point objPoint)
        {
            return new Position()
            {
                Latitude = objPoint.Latitude.ToDouble(),
                Longitude = objPoint.Longitude.ToDouble()
            };
        }

        /// <summary>
        /// To the locations.
        /// </summary>
        /// <param name="objGeoLocation">The object geo location.</param>
        /// <returns>returns Locations.</returns>
        public static List<Location> ToLocations(this IEnumerable<GeoLocation> objGeoLocation)
        {
            List<Location> locations = new List<Location>();
            locations.AddRange(objGeoLocation.Select(obj => obj.ToLocation()));

            return locations;
        }

        /// <summary>
        /// To the location.
        /// </summary>
        /// <param name="objGlobalCoordinates">The object global coordinates.</param>
        /// <returns>returns Location.</returns>
        public static Location ToLocation(this GlobalCoordinates objGlobalCoordinates)
        {
            return new Location()
                   {
                       Latitude = objGlobalCoordinates.Latitude.Degrees,
                       Longitude = objGlobalCoordinates.Longitude.Degrees,
                   };
        }

        /// <summary>To the square area.</summary>
        /// <param name="objLocation">The object location.</param>
        /// <param name="squareSize">Size of the square.</param>
        /// <returns>returns SquareArea.</returns>
        public static SquareArea ToSquareArea(this Location objLocation, Distance squareSize)
        {
            return GeoCalculations.BuildSquare(objLocation, squareSize);
        }

        /// <summary>To the global coordinates.</summary>
        /// <param name="objLocation">The object location.</param>
        /// <returns>returns GlobalCoordinates.</returns>
        public static GlobalCoordinates ToVincentyCoordinates(this Location objLocation)
        {
            GlobalCoordinates geodeticCoordinates = new GlobalCoordinates(new Angle(objLocation.Latitude), new Angle(objLocation.Longitude));
            return geodeticCoordinates;
        }

        /// <summary>Gets the location towards bearing.</summary>
        /// <param name="source">The source.</param>
        /// <param name="distance">The distance.</param>
        /// <param name="bearing">The bearing.</param>
        /// <returns>returns Location.</returns>
        public static Location GetLocationTowardsBearing(this Location source, Distance distance, double bearing)
        {
            return GeoCalculations.GetLocationTowardsBearing(source, distance, bearing);
        }

        /// <summary>Calculates the bearing.</summary>
        /// <param name="source">The source.</param>
        /// <param name="destination">The destination.</param>
        /// <returns>returns bearing.</returns>
        public static double CalculateBearing(Location source, Location destination)
        {
            return GeoCalculations.CalculateBearing(source, destination);
        }

        #endregion

        #region Distance Extensions

        /// <summary>Returns a Distance object containing the negative of the container value</summary>
        /// <param name="objDistance">The object distance.</param>
        /// <returns>returns Distance.</returns>
        public static Distance Negative(this Distance objDistance)
        {
            Distance newDistance = new Distance(-objDistance.Value, objDistance.Unit);
            return newDistance;
        }

        /// <summary>Converts the unit.</summary>
        /// <param name="objDistance">The object distance.</param>
        /// <param name="targetType">Type of the target.</param>
        /// <returns>returns Distance.</returns>
        public static Distance ConvertUnit(this Distance objDistance, DistanceUnit targetType)
        {
            DistanceUnit sourceType = objDistance.Unit;
            double convertedValue = 0;

            switch (sourceType)
            {
                case DistanceUnit.Feet:
                    switch (targetType)
                    {
                        case DistanceUnit.Feet:
                            convertedValue = objDistance.Value;
                            break;

                        case DistanceUnit.Meter:
                            convertedValue = objDistance.Value.FeetToMeters();
                            break;

                        case DistanceUnit.KM:
                            convertedValue = objDistance.Value.FeetToMeters().MetersToKm();
                            break;

                        case DistanceUnit.Miles:
                            convertedValue = objDistance.Value.FeetToMiles();
                            break;
                    }

                    break;

                case DistanceUnit.Meter:
                    switch (targetType)
                    {
                        case DistanceUnit.Feet:
                            convertedValue = objDistance.Value.MetersToFeet();
                            break;

                        case DistanceUnit.Meter:
                            convertedValue = objDistance.Value;
                            break;

                        case DistanceUnit.KM:
                            convertedValue = objDistance.Value.MetersToKm();
                            break;

                        case DistanceUnit.Miles:
                            convertedValue = objDistance.Value.MetersToKm().KmToMiles();
                            break;
                    }

                    break;

                case DistanceUnit.KM:
                    switch (targetType)
                    {
                        case DistanceUnit.Feet:
                            convertedValue = objDistance.Value.KmToMeters().MetersToFeet();
                            break;

                        case DistanceUnit.Meter:
                            convertedValue = objDistance.Value.KmToMeters();
                            break;

                        case DistanceUnit.KM:
                            convertedValue = objDistance.Value;
                            break;

                        case DistanceUnit.Miles:
                            convertedValue = objDistance.Value.KmToMiles();
                            break;
                    }

                    break;

                case DistanceUnit.Miles:
                    switch (targetType)
                    {
                        case DistanceUnit.Feet:
                            convertedValue = objDistance.Value.MilesToFeet();
                            break;

                        case DistanceUnit.Meter:
                            convertedValue = objDistance.Value.MilesToMeters();
                            break;

                        case DistanceUnit.KM:
                            convertedValue = objDistance.Value.MilesToKm();
                            break;

                        case DistanceUnit.Miles:
                            convertedValue = objDistance.Value;
                            break;
                    }

                    break;
            }

            return new Distance(convertedValue, targetType);
        }

        /// <summary>
        /// Converts the unit.
        /// </summary>
        /// <param name="objPower">The object power.</param>
        /// <param name="targetType">Type of the target.</param>
        /// <returns>returns Power.</returns>
        public static Power ConvertUnit(this Power objPower, PowerUnit targetType)
        {
            PowerUnit sourceType = objPower.Unit;
            double convertedValue = 0;

            switch (sourceType)
            {
                case PowerUnit.KiloWatt:
                    switch (targetType)
                    {
                        case PowerUnit.Watt:
                            convertedValue = objPower.Value.KilowattToWatt();
                            break;

                        case PowerUnit.KiloWatt:
                            convertedValue = objPower.Value;
                            break;

                        case PowerUnit.MegaWatt:
                            convertedValue = objPower.Value.KilowattToWatt().WattToMegaWatt();
                            break;

                        case PowerUnit.dB:
                            convertedValue = objPower.Value.KiloWattToDecibel();
                            break;
                    }

                    break;

                case PowerUnit.MegaWatt:
                    switch (targetType)
                    {
                        case PowerUnit.Watt:
                            convertedValue = objPower.Value.MegaWattToWatt();
                            break;

                        case PowerUnit.KiloWatt:
                            convertedValue = objPower.Value.MegaWattToWatt().WattToKiloWatt();
                            break;

                        case PowerUnit.MegaWatt:
                            convertedValue = objPower.Value;
                            break;

                        case PowerUnit.dB:
                            convertedValue = objPower.Value.MegaWattToDecibel();
                            break;
                    }

                    break;

                case PowerUnit.Watt:
                    switch (targetType)
                    {
                        case PowerUnit.Watt:
                            convertedValue = objPower.Value;
                            break;

                        case PowerUnit.KiloWatt:
                            convertedValue = objPower.Value.WattToKiloWatt();
                            break;

                        case PowerUnit.MegaWatt:
                            convertedValue = objPower.Value.WattToMegaWatt();
                            break;

                        case PowerUnit.dB:
                            convertedValue = objPower.Value.WattToDecibel();
                            break;
                    }

                    break;

                case PowerUnit.dB:
                    switch (targetType)
                    {
                        case PowerUnit.Watt:
                            convertedValue = objPower.Value.DecibelToWatt();
                            break;

                        case PowerUnit.KiloWatt:
                            convertedValue = objPower.Value.DecibelToKiloWatt();
                            break;

                        case PowerUnit.MegaWatt:
                            convertedValue = objPower.Value.DecibelToMegaWatt();
                            break;

                        case PowerUnit.dB:
                            convertedValue = objPower.Value;
                            break;
                    }

                    break;
            }

            return new Power(convertedValue, targetType);
        }

        /// <summary>distance in miles</summary>
        /// <param name="objDistance">The object distance.</param>
        /// <returns>returns Distance.</returns>
        public static double InMiles(this Distance objDistance)
        {
            return objDistance.ConvertUnit(DistanceUnit.Miles).Value;
        }

        /// <summary>distance in km.</summary>
        /// <param name="objDistance">The object distance.</param>
        /// <returns>returns km.</returns>
        public static double InKm(this Distance objDistance)
        {
            return objDistance.ConvertUnit(DistanceUnit.KM).Value;
        }

        /// <summary>distance in feet.</summary>
        /// <param name="objDistance">The object distance.</param>
        /// <returns>returns feet.</returns>
        public static double InFeet(this Distance objDistance)
        {
            return objDistance.ConvertUnit(DistanceUnit.Feet).Value;
        }

        /// <summary>distance in meter</summary>
        /// <param name="objDistance">The object distance.</param>
        /// <returns>returns Distance.</returns>
        public static double InMeter(this Distance objDistance)
        {
            return objDistance.ConvertUnit(DistanceUnit.Meter).Value;
        }

        #endregion

        #region Distance Conversions

        /// <summary>Feet to meters.</summary>
        /// <param name="feet">The feet.</param>
        /// <returns>returns System.Double.</returns>
        public static double FeetToMeters(this double feet)
        {
            return Conversion.FeetToMeters(feet);
        }

        /// <summary>Convert Feet to Miles</summary>
        /// <param name="feet">The feet.</param>
        /// <returns>returns System.Double.</returns>
        public static double FeetToMiles(this double feet)
        {
            return Conversion.FeetToMiles(feet);
        }

        /// <summary>Convert Meters to Feet</summary>
        /// <param name="meters">The meters.</param>
        /// <returns>returns System.Double.</returns>
        public static double MetersToFeet(this double meters)
        {
            return Conversion.MetersToFeet(meters);
        }

        /// <summary>Convert Miles to Kilometers</summary>
        /// <param name="miles">The miles.</param>
        /// <returns>returns System.Double.</returns>
        public static double MilesToKm(this double miles)
        {
            return Conversion.MilesToKm(miles);
        }

        /// <summary>Convert Miles to Feet</summary>
        /// <param name="miles">The miles.</param>
        /// <returns>returns System.Double.</returns>
        public static double MilesToFeet(this double miles)
        {
            return Conversion.MilesToFeet(miles);
        }

        /// <summary>Miles to meters.</summary>
        /// <param name="miles">The miles.</param>
        /// <returns>returns System.Double.</returns>
        public static double MilesToMeters(this double miles)
        {
            return Conversion.MilesToMeters(miles);
        }

        /// <summary>Km to miles.</summary>
        /// <param name="km">The km.</param>
        /// <returns>returns System.Double.</returns>
        public static double KmToMiles(this double km)
        {
            return Conversion.KmToMiles(km);
        }

        /// <summary>Km to meters.</summary>
        /// <param name="km">The km.</param>
        /// <returns>returns System.Double.</returns>
        public static double KmToMeters(this double km)
        {
            return Conversion.KmToMeters(km);
        }

        /// <summary>Meter to km.</summary>
        /// <param name="meters">The meters.</param>
        /// <returns>returns System.Double.</returns>
        public static double MetersToKm(this double meters)
        {
            return Conversion.MetersToKm(meters);
        }

        #endregion

        #region Power/Gain Conversions

        /// <summary>mw to decibel.</summary>
        /// <param name="mW">The m w.</param>
        /// <returns>returns System.Double.</returns>
        public static double MilliWattToDecibel(this double mW)
        {
            return Conversion.MilliWattToDecibel(mW);
        }

        /// <summary>Watt to decibel.</summary>
        /// <param name="w">The w.</param>
        /// <returns>returns System.Double.</returns>
        public static double WattToDecibel(this double w)
        {
            return Conversion.WattToDecibel(w);
        }

        /// <summary>Kw to decibel.</summary>
        /// <param name="kw">The kw.</param>
        /// <returns>returns power in decibel.</returns>
        public static double KiloWattToDecibel(this double kw)
        {
            return Conversion.KiloWattToDecibel(kw);
        }

        /// <summary>MW to decibel.</summary>
        /// <param name="mw">The mw.</param>
        /// <returns>returns megawatt.</returns>
        public static double MegaWattToDecibel(this double mw)
        {
            return Conversion.MegaWattToDecibel(mw);
        }

        /// <summary>Decibel to mw.</summary>
        /// <param name="db">The database.</param>
        /// <returns>returns power in mw.</returns>
        public static double DecibelToMilliWatt(this double db)
        {
            return Conversion.DecibelToMilliWatt(db);
        }

        /// <summary>Decibel to watt.</summary>
        /// <param name="db">The database.</param>
        /// <returns>returns watt.</returns>
        public static double DecibelToWatt(this double db)
        {
            return Conversion.DecibelToWatt(db);
        }

        /// <summary>Decibel to kilo watt.</summary>
        /// <param name="db">The database.</param>
        /// <returns>returns power in kw.</returns>
        public static double DecibelToKiloWatt(this double db)
        {
            return Conversion.DecibelToKiloWatt(db);
        }

        /// <summary>Decibel to mega watt.</summary>
        /// <param name="db">The database.</param>
        /// <returns>returns System.Double.</returns>
        public static double DecibelToMegaWatt(this double db)
        {
            return Conversion.DecibelToMegaWatt(db);
        }

        /// <summary>Kilowatt to watt.</summary>
        /// <param name="kw">The kw.</param>
        /// <returns>returns kw.</returns>
        public static double KilowattToWatt(this double kw)
        {
            return Conversion.KilowattToWatt(kw);
        }

        /// <summary>Watt to kilo watt.</summary>
        /// <param name="watt">The watt.</param>
        /// <returns>returns watt.</returns>
        public static double WattToKiloWatt(this double watt)
        {
            return Conversion.WattToKiloWatt(watt);
        }

        /// <summary>Watt to mega watt.</summary>
        /// <param name="watt">The watt.</param>
        /// <returns>returns megawatt. </returns>
        public static double WattToMegaWatt(this double watt)
        {
            return Conversion.WattToMegaWatt(watt);
        }

        /// <summary>MW to watt.</summary>
        /// <param name="watt">The watt.</param>
        /// <returns>returns power in watts.</returns>
        public static double MegaWattToWatt(this double watt)
        {
            return Conversion.MegaWattToWatt(watt);
        }

        /// <summary>in decibel.</summary>
        /// <param name="objPower">The object power.</param>
        /// <returns>returns Power in decibel.</returns>
        public static double InDb(this Power objPower)
        {
            return objPower.ConvertUnit(PowerUnit.dB).Value;
        }

        /// <summary>in watt.</summary>
        /// <param name="objPower">The object power.</param>
        /// <returns>returns Power.</returns>
        public static Power InWatt(this Power objPower)
        {
            return objPower.ConvertUnit(PowerUnit.Watt);
        }

        /// <summary>in mega watt.</summary>
        /// <param name="objPower">The object power.</param>
        /// <returns>returns Power.</returns>
        public static Power InMegaWatt(this Power objPower)
        {
            return objPower.ConvertUnit(PowerUnit.MegaWatt);
        }

        /// <summary>in kilo watt.</summary>
        /// <param name="objPower">The object power.</param>
        /// <returns>returns Power.</returns>
        public static Power InKiloWatt(this Power objPower)
        {
            return objPower.ConvertUnit(PowerUnit.KiloWatt);
        }

        #endregion

        #region GeographicExtensions

        /// <summary>
        /// To the RADIAN.
        /// </summary>
        /// <param name="objValue">The object value.</param>
        /// <returns>returns System.Double.</returns>
        public static double ToRad(this double objValue)
        {
            return GeoCalculations.ToRad(objValue);
        }

        #endregion

        /// <summary>
        /// To the rule set information entity.
        /// </summary>
        /// <param name="ruleSetInfo">The rule set information.</param>
        /// <returns>returns RuleSetInfoEntity.</returns>
        public static RuleSetInfoEntity ToRuleSetInfoEntity(this RulesetInfo ruleSetInfo)
        {
            RuleSetInfoEntity ruleSetInfoEntity = new RuleSetInfoEntity();
            ruleSetInfoEntity.Authority = ruleSetInfo.Authority;
            ruleSetInfoEntity.MaxEirpHz = ruleSetInfo.MaxEirpHz;
            ruleSetInfoEntity.MaxLocationChange = ruleSetInfo.MaxLocationChange;
            ruleSetInfoEntity.MaxNominalChannelBwMhz = ruleSetInfo.MaxNominalChannelBwMhz;
            ruleSetInfoEntity.MaxPollingSecs = ruleSetInfo.MaxPollingSecs;
            ruleSetInfoEntity.MaxTotalBwMhz = ruleSetInfo.MaxTotalBwMhz;
            ruleSetInfoEntity.RulesetId = ruleSetInfo.RulesetId;
            return ruleSetInfoEntity;
        }

        /// <summary>
        /// To the rule set information.
        /// </summary>
        /// <param name="ruleSetInfoEntity">The rule set information entity.</param>
        /// <returns>returns rule set information.</returns>
        public static RulesetInfo ToRuleSetInfo(this RuleSetInfoEntity ruleSetInfoEntity)
        {
            RulesetInfo ruleSetInfo = new RulesetInfo();
            ruleSetInfo.Authority = ruleSetInfoEntity.Authority;
            ruleSetInfo.MaxEirpHz = ruleSetInfoEntity.MaxEirpHz;
            ruleSetInfo.MaxLocationChange = ruleSetInfoEntity.MaxLocationChange;
            ruleSetInfo.MaxNominalChannelBwMhz = ruleSetInfoEntity.MaxNominalChannelBwMhz;
            ruleSetInfo.MaxPollingSecs = ruleSetInfoEntity.MaxPollingSecs;
            ruleSetInfo.MaxTotalBwMhz = ruleSetInfoEntity.MaxTotalBwMhz;
            ruleSetInfo.RulesetId = ruleSetInfoEntity.RulesetId;
            return ruleSetInfo;
        }

        /// <summary>
        /// Checks the if any of the incumbents contains desired location.
        /// </summary>
        /// <param name="station">The incumbent.</param>
        /// <param name="point">The target incumbent.</param>
        /// <param name="offContourDistance">The desired external contour distance.</param>
        /// <returns>returns Incumbent.</returns>
        public static bool IsInOrAroundContour(this Incumbent station, Location point, Distance offContourDistance)
        {
            if (string.IsNullOrEmpty(station.Contour))
            {
                return false;
            }

            var contour = JsonSerialization.DeserializeString<Contour>(station.Contour);
            List<Location> polygonPoints = contour.ContourPoints;

            if (GeoCalculations.IsPointInPolygon(polygonPoints, point))
            {
                return true;
            }

            // point is not in contour 
            // need to check for safe Distance from contour
            var bearingToTarget = GeoCalculations.CalculateBearing(station.Location, point);            

            double distance = 0.0;
            bool lessThanOffContourDistance = false;
            int index;
            for (int i = (int)bearingToTarget - 90; i < (int)bearingToTarget + 90; i++)
            {
                index = i;
                if (index >= 360)
                {
                    index = i - 360;
                }
                else if (index < 0)
                {
                    index = 360 + i;
                }

                distance = GeoCalculations.GetDistance(point, polygonPoints[index]).Value / 1000.0;
                if (distance <= offContourDistance.InKm())
                {
                    lessThanOffContourDistance = true;
                    break;                
                }                
            }

            return lessThanOffContourDistance;            
        }

        /// <summary>
        /// Patches the specified antenna patterns.
        /// </summary>
        /// <param name="antennaPatterns">The antenna patterns.</param>
        /// <returns>returns antenna Patterns.</returns>
        public static double[] Patch(this double[] antennaPatterns)
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

            return antennaPatterns;
        }

        /// <summary>
        /// To the DOUBLE.
        /// </summary>
        /// <param name="dataValue">The data value.</param>
        /// <returns>returns System.Double.</returns>
        public static double ToDouble(this string dataValue)
        {
            double doubleValue = 0;
            double.TryParse(dataValue, out doubleValue);
            return doubleValue;
        }

        /// <summary>
        /// To the Integer 32bit.
        /// </summary>
        /// <param name="dataValue">The data value.</param>
        /// <returns>returns Integer.</returns>
        public static int ToInt32(this string dataValue)
        {
            int intValue = 0;
            int.TryParse(dataValue, out intValue);
            return intValue;
        }

        /// <summary>
        /// To the BOOL.
        /// </summary>
        /// <param name="dataValue">The data value.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public static bool ToBool(this string dataValue)
        {
            bool boolValue = false;
            bool.TryParse(dataValue, out boolValue);
            return boolValue;
        }

        /// <summary>
        /// To the DateTime.
        /// </summary>
        /// <param name="dataValue">The data value.</param>
        /// <param name="format">The format.</param>
        /// <returns>returns date.</returns>
        public static DateTime ToDateTime(this string dataValue, string format = null)
        {
            DateTime value;
            if (string.IsNullOrEmpty(dataValue))
            {
                return DateTime.Now;
            }

            if (format == null)
            {
                value = DateTime.Parse(dataValue, CultureInfo.InvariantCulture);
            }
            else
            {
                if (format == "mm/dd/yyyy")
                {
                    var data = dataValue.Split(new[] { "//", "/" }, StringSplitOptions.RemoveEmptyEntries);
                    value = new DateTime(data[2].ToInt32(), data[0].ToInt32(), data[1].ToInt32());
                }
                else
                {
                    DateTime.TryParseExact(dataValue, format, CultureInfo.InvariantCulture, DateTimeStyles.None, out value);
                }
            }

            return value;
        }

        /// <summary>
        /// To the DateTime.
        /// </summary>
        /// <param name="dataValue">The data value.</param>
        /// <returns>returns date.</returns>
        public static DateTimeOffset ToDateTimeOffset(this string dataValue)
        {
            DateTimeOffset value = DateTimeOffset.MinValue;
            DateTimeOffset.TryParse(dataValue, CultureInfo.InvariantCulture, DateTimeStyles.AssumeUniversal, out value);

            return value;
        }

        /// <summary>
        /// Copies the specified object record.
        /// </summary>
        /// <param name="objRecord">The object record.</param>
        /// <returns>returns ULSRecord.</returns>
        public static ULSRecord Copy(this ULSRecord objRecord)
        {
            ULSRecord rec = JsonSerialization.DeserializeString<ULSRecord>(JsonSerialization.SerializeObject(objRecord));
            return rec;
        }

        /// <summary>
        /// Copies the specified object record.
        /// </summary>
        /// <param name="objRecord">The object record.</param>
        /// <returns>returns ULSRecord.</returns>
        public static CDBSTvEngData Copy(this CDBSTvEngData objRecord)
        {
            CDBSTvEngData rec = JsonSerialization.DeserializeString<CDBSTvEngData>(JsonSerialization.SerializeObject(objRecord));
            return rec;
        }

        /// <summary>
        /// Partitions the specified source.
        /// </summary>
        /// <typeparam name="T">type of List</typeparam>
        /// <param name="source">The source.</param>
        /// <param name="size">The size.</param>
        /// <returns>returns partition list.</returns>
        public static List<List<T>> Partition<T>(this List<T> source, int size)
        {
            List<List<T>> partitions = new List<List<T>>();
            for (int i = 0; i < Math.Ceiling(source.Count / (double)size); i++)
            {
                partitions.Add(new List<T>(source.Skip(size * i).Take(size)));
            }

            return partitions;
        }

        /// <summary>
        /// Copies the specified object record.
        /// </summary>
        /// <typeparam name="T">type of object</typeparam>
        /// <param name="objRecord">The object record.</param>
        /// <returns>returns ULSRecord.</returns>
        public static T Copy<T>(this T objRecord)
        {
            var rec = JsonSerialization.DeserializeString<T>(JsonSerialization.SerializeObject(objRecord));
            return rec;
        }

        /// <summary>
        /// Determines whether a point is in a RegionPolygons.
        /// </summary>
        /// <param name="subregion">Type of RegionPolygonsCache.</param>
        /// <param name="latitude">The latitude.</param>
        /// <param name="longitude">The longitude.</param>
        /// <returns>The boolean value indicating point lies inside the polygon or not</returns>
        public static List<Location> Contains(this RegionPolygonsCache subregion, double latitude, double longitude)
        {          
            if (subregion == null)
            {
                return null;
            }

            // If the point doesn't exist in region bounding box then return false.
            if (!subregion.BoundingBox.Contains(latitude, longitude))
            {
                return null;
            }

            List<LocationRect> locationRectangles = subregion.LocationRectangles;

            // Start with smaller location rectangles, so that we have less polygon points to deal with when finding a point inside the polygon.
            for (int i = locationRectangles.Count - 1; i >= 0; i--)
            {
                if (locationRectangles[i].Contains(latitude, longitude))
                {
                    List<Location> polygon = subregion.PolygonCollection[i];

                    if (GeoCalculations.IsPointInPolygon(polygon, latitude, longitude))
                    {
                        return polygon;
                    }
                }
            }

            return null;
        }
    }
}
