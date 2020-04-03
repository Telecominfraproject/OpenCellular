// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Web.UI.WebControls;
    using ConsoleTesting;
    using Gavaghan.Geodesy;
    using Microsoft.Whitespace.Entities;

    /// <summary>
    /// Represents GeoCalculations.
    /// </summary>
    public static class GeoCalculations
    {
        /// <summary>The VINCENTY calculator</summary>
        private static GeodeticCalculator vincentyCalculator = new GeodeticCalculator();

        /// <summary>The national grid tiles</summary>
        private static string[,] nationalGridTiles = null;

        /// <summary>
        /// Initializes static members of the <see cref="GeoCalculations"/> class.
        /// </summary>
        static GeoCalculations()
        {
            TransformCoordinate.Initialize();
        }

        /// <summary>
        /// Gets the earth radius.
        /// </summary>
        /// <value>The earth radius.</value>
        public static Distance EarthRadius
        {
            get { return new Distance(3963.0, DistanceUnit.Miles); }
        }

        /// <summary>
        /// Gets the location datum.
        /// </summary>
        /// <value>The location datum.</value>
        public static Ellipsoid LocationDatum
        {
            get { return Ellipsoid.WGS84; }
        }

        /// <summary>
        /// Builds the square.
        /// </summary>
        /// <param name="centrePoint">The centre point.</param>
        /// <param name="squareSize">radius of the square.</param>
        /// <returns>returns SquareArea.</returns>
        public static SquareArea BuildSquare(Location centrePoint, Distance squareSize)
        {
            SquareArea square = new SquareArea(centrePoint, squareSize);

            var midPoint = centrePoint.ToVincentyCoordinates();

            double squareHalfDiagonal = (squareSize.InMeter() / 2) * Math.Sqrt(2);

            // Top Right Location from Centre Point
            var topLeft = CalculateCoordinates(midPoint, 315, squareHalfDiagonal).ToLocation();

            // Bottom Right Location from Horizontal Point on X-Axis
            var bottomRight = CalculateCoordinates(midPoint, 135, squareHalfDiagonal).ToLocation();

            square.SetArea(topLeft, bottomRight);

            return square;
        }

        /// <summary>
        /// Builds the circle.
        /// </summary>
        /// <param name="centrePoint">The centre point.</param>
        /// <param name="radius">The radius.</param>
        /// <param name="samplePoints">The sample points.</param>
        /// <param name="arcAngle">The arc angle in degree.</param>
        /// <returns>returns CircleArea.</returns>
        public static CircleArea BuildCircle(Location centrePoint, Distance radius, int samplePoints, int arcAngle)
        {
            double squareHalfSide = radius.InMeter() / 2;
            CircleArea circle = new CircleArea(centrePoint, radius);

            var midPoint = centrePoint.ToVincentyCoordinates();

            List<Location> locPoints = new List<Location>();

            for (int i = arcAngle; i < 360; i = i + arcAngle)
            {
                var location = vincentyCalculator.CalculateEndingGlobalCoordinates(LocationDatum, centrePoint.ToVincentyCoordinates(), i, radius.InMeter()).ToLocation();
                locPoints.Add(location);
            }

            circle.SetPoints(locPoints, arcAngle);
            return circle;
        }

        /// <summary>
        /// Points the on position x.
        /// </summary>
        /// <param name="startPoint">The start point.</param>
        /// <param name="bearing">The bearing.</param>
        /// <param name="distance">The distance.</param>
        /// <returns>returns GlobalCoordinates.</returns>
        public static GlobalCoordinates CalculateCoordinates(GlobalCoordinates startPoint, double bearing, double distance)
        {
            return vincentyCalculator.CalculateEndingGlobalCoordinates(LocationDatum, startPoint, bearing, distance);
        }

        /// <summary>
        /// Gets the location towards bearing.
        /// </summary>
        /// <param name="source">The source.</param>
        /// <param name="distance">The distance.</param>
        /// <param name="bearing">The bearing.</param>
        /// <returns>returns Location.</returns>
        public static Location GetLocationTowardsBearing(Location source, Distance distance, double bearing)
        {
            return CalculateCoordinates(source.ToVincentyCoordinates(), bearing, distance.InMeter()).ToLocation();
        }

        /// <summary>
        /// Gets the location towards bearing.
        /// </summary>
        /// <param name="source">The source.</param>
        /// <param name="distanceInMtr">The distance in MTR.</param>
        /// <param name="bearing">The bearing.</param>
        /// <returns>returns Location.</returns>
        public static Location GetLocationTowardsBearing(Location source, double distanceInMtr, double bearing)
        {
            return CalculateCoordinates(source.ToVincentyCoordinates(), bearing, distanceInMtr).ToLocation();
        }

        /// <summary>
        /// Calculates the bearing.
        /// </summary>
        /// <param name="source">The source.</param>
        /// <param name="destination">The destination.</param>
        /// <returns>returns bearing.</returns>
        public static double CalculateBearing(Location source, Location destination)
        {
            var coordinates = vincentyCalculator.CalculateGeodeticCurve(LocationDatum, source.ToVincentyCoordinates(), destination.ToVincentyCoordinates());

            return coordinates.Azimuth.Degrees;
        }

        /// <summary>
        /// Distances the in miles.
        /// </summary>
        /// <param name="startLocation">The start location.</param>
        /// <param name="endLocation">The end location.</param>
        /// <returns>returns Distance</returns>
        public static Distance GetDistance(Location startLocation, Location endLocation)
        {
            var vincentyCurve = vincentyCalculator.CalculateGeodeticCurve(LocationDatum, startLocation.ToVincentyCoordinates(), endLocation.ToVincentyCoordinates());
            return new Distance(vincentyCurve.EllipsoidalDistance, DistanceUnit.Meter);
        }

        /// <summary>
        /// Latitude in degrees.
        /// </summary>
        /// <param name="degrees">The degrees.</param>
        /// <param name="minutes">The minutes.</param>
        /// <param name="seconds">The seconds.</param>
        /// <param name="direction">The direction.</param>
        /// <returns>returns Latitude in degrees</returns>
        public static double GetLattitudeInDegrees(double degrees, double minutes, double seconds, string direction = null)
        {
            double lattitudeInDegrees = (seconds / 3600.0) + (minutes / 60.0);

            lattitudeInDegrees = (degrees < 0) ? (degrees - lattitudeInDegrees) : (degrees + lattitudeInDegrees);

            if (!string.IsNullOrEmpty(direction) && direction == "S")
            {
                lattitudeInDegrees = -lattitudeInDegrees;
            }

            return lattitudeInDegrees;
        }

        /// <summary>
        /// Longitude in degrees.
        /// </summary>
        /// <param name="degrees">The degrees.</param>
        /// <param name="minutes">The minutes.</param>
        /// <param name="seconds">The seconds.</param>
        /// <param name="direction">The direction.</param>
        /// <returns>returns Longitude in degrees</returns>
        public static double GetLongitudeInDegrees(double degrees, double minutes, double seconds, string direction = null)
        {
            double longitudeInDegrees = (seconds / 3600.0) + (minutes / 60.0);

            longitudeInDegrees = (degrees < 0) ? (degrees - longitudeInDegrees) : (degrees + longitudeInDegrees);

            if (!string.IsNullOrEmpty(direction) && direction == "W")
            {
                longitudeInDegrees = -longitudeInDegrees;
            }

            return longitudeInDegrees;
        }

        /// <summary>
        /// To the RADIAN.
        /// </summary>
        /// <param name="input">The input.</param>
        /// <returns>returns System.Double.</returns>
        public static double ToRad(double input)
        {
            return input * (Math.PI / 180.0);
        }

        /// <summary>
        /// Gets the easting northing.
        /// </summary>
        /// <param name="location">The location.</param>
        /// <returns>returns OSGLocation.</returns>
        public static OSGLocation GetEastingNorthing(Location location)
        {
            var eastingNorthing = TransformCoordinate.ToEastingNorthing(location.Latitude, location.Longitude);
            return new OSGLocation((int)eastingNorthing.Item1, (int)eastingNorthing.Item2);
        }

        /// <summary>
        /// Gets the easting northing with double precision.
        /// </summary>
        /// <param name="location">The location.</param>
        /// <returns>returns OSGLocation.</returns>
        public static OSGLocation GetEastingNorthingWithDoublePrecision(Location location)
        {
            var eastingNorthing = TransformCoordinate.ToEastingNorthing(location.Latitude, location.Longitude);
            return new OSGLocation(eastingNorthing.Item1, eastingNorthing.Item2);
        }

        /// <summary>
        /// Gets the easting northing.
        /// </summary>
        /// <param name="latitude">The latitude.</param>
        /// <param name="longitude">The longitude.</param>
        /// <returns>returns OSGLocation.</returns>
        public static OSGLocation GetEastingNorthing(double latitude, double longitude)
        {
            var eastingNorthing = TransformCoordinate.ToEastingNorthing(latitude, longitude);
            return new OSGLocation((int)eastingNorthing.Item1, (int)eastingNorthing.Item2);
        }

        /// <summary>
        /// Gets the latitude longitude.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <returns>returns Location.</returns>
        public static Location GetLatitudeLongitude(double easting, double northing)
        {
            return TransformCoordinate.ToLatitudeLongitude((int)easting, (int)northing);
        }

        /// <summary>
        /// Degree to RAD.
        /// </summary>
        /// <param name="x">The x.</param>
        /// <returns>returns System.Double.</returns>
        public static double Deg2Rad(double x)
        {
            return x * (Math.PI / 180);
        }

        /// <summary>
        /// Radian to degree.
        /// </summary>
        /// <param name="x">The x.</param>
        /// <returns>returns System.Double.</returns>
        public static double Rad2Deg(double x)
        {
            return x * (180.0 / Math.PI);
        }

        /// <summary>
        /// Gets the location.
        /// </summary>
        /// <param name="latitude">The latitude.</param>
        /// <param name="longitude">The longitude.</param>
        /// <returns>returns Location.</returns>
        public static Location GetLocation(string latitude, string longitude)
        {
            var seperators = new[]
                             {
                                 " ", "-", ",", ":"
                             };

            var latitudePos = latitude.Split(seperators, StringSplitOptions.RemoveEmptyEntries);
            var longitudePos = longitude.Split(seperators, StringSplitOptions.RemoveEmptyEntries);

            double lat_deg = 0, lat_min = 0, lat_sec = 0;
            double lon_deg = 0, lon_min = 0, lon_sec = 0;

            string lat_dir = string.Empty, lon_dir = string.Empty;

            if (latitudePos.Length == 0 || longitudePos.Length == 0)
            {
                return null;
            }

            lat_deg = latitudePos[0].ToDouble();
            lon_deg = longitudePos[0].ToDouble();

            if (latitudePos.Length > 1)
            {
                if (char.IsLetter(latitudePos[1], 0))
                {
                    lat_dir = latitudePos[1];
                }
                else
                {
                    lat_min = latitudePos[1].ToDouble();
                }

                if (latitudePos.Length > 2)
                {
                    if (char.IsLetter(latitudePos[2], 0))
                    {
                        lat_dir = latitudePos[2];
                    }
                    else
                    {
                        lat_sec = latitudePos[2].ToDouble();
                    }

                    if (latitudePos.Length > 3)
                    {
                        if (char.IsLetter(latitudePos[3], 0))
                        {
                            lat_dir = latitudePos[3];
                        }
                    }
                }
            }

            if (longitudePos.Length > 1)
            {
                if (char.IsLetter(longitudePos[1], 0))
                {
                    lon_dir = longitudePos[1];
                }
                else
                {
                    lon_min = longitudePos[1].ToDouble();
                }

                if (longitudePos.Length > 2)
                {
                    if (char.IsLetter(longitudePos[2], 0))
                    {
                        lon_dir = longitudePos[2];
                    }
                    else
                    {
                        lon_sec = longitudePos[2].ToDouble();
                    }

                    if (longitudePos.Length > 3)
                    {
                        if (char.IsLetter(longitudePos[3], 0))
                        {
                            lon_dir = longitudePos[3];
                        }
                    }
                }
            }

            return new Location(GetLattitudeInDegrees(lat_deg, lat_min, lat_sec, lat_dir), GetLongitudeInDegrees(lon_deg, lon_min, lon_sec, lon_dir));
        }

        /// <summary>
        /// Determines whether [is point in polygon] [the specified locations].
        /// </summary>
        /// <param name="points">The locations.</param>
        /// <param name="latitude">The latitude.</param>
        /// <param name="longitude">The longitude.</param>
        /// <returns><c>true</c> if [is point in polygon] [the specified locations]; otherwise, <c>false</c>.</returns>
        public static bool IsPointInPolygon(List<Location> points, double latitude, double longitude)
        {
            var location = new Location(latitude, longitude);
            return IsPointInPolygon(points, location);
        }

        /// <summary>
        /// Determines whether [is point in polygon] [the specified locations].
        /// </summary>
        /// <param name="points">The locations.</param>
        /// <param name="point">The point.</param>
        /// <returns><c>true</c> if [is point in polygon] [the specified locations]; otherwise, <c>false</c>.</returns>
        public static bool IsPointInPolygon(List<Location> points, Location point)
        {
            var i = 0;
            var j = points.Count - 1;
            var inPoly = false;

            for (i = 0; i < points.Count; i++)
            {
                if ((points[i].Longitude < point.Longitude && points[j].Longitude >= point.Longitude)
                  || (points[j].Longitude < point.Longitude && points[i].Longitude >= point.Longitude))
                {
                    if (points[i].Latitude + (((point.Longitude - points[i].Longitude) /
                      (points[j].Longitude - points[i].Longitude)) * (points[j].Latitude - points[i].Latitude)) < point.Latitude)
                    {
                        inPoly = !inPoly;
                    }
                }

                j = i;
            }

            return inPoly;
        }

        /// <summary>
        /// Determines whether [is point in polygon] [the specified points].
        /// </summary>
        /// <param name="points">The points.</param>
        /// <param name="point">The point.</param>
        /// <returns><c>true</c> if [is point in polygon] [the specified points]; otherwise, <c>false</c>.</returns>
        public static bool IsPointInPolygon(Location[] points, Location point)
        {
            var i = 0;
            var j = points.Length - 1;
            var inPoly = false;

            for (i = 0; i < points.Length; i++)
            {
                if ((points[i].Longitude < point.Longitude && points[j].Longitude >= point.Longitude)
                  || (points[j].Longitude < point.Longitude && points[i].Longitude >= point.Longitude))
                {
                    if (points[i].Latitude + (((point.Longitude - points[i].Longitude) /
                      (points[j].Longitude - points[i].Longitude)) * (points[j].Latitude - points[i].Latitude)) < point.Latitude)
                    {
                        inPoly = !inPoly;
                    }
                }

                j = i;
            }

            return inPoly;
        }

        /// <summary>
        /// Determines whether [is point in polygon] [the specified points].
        /// </summary>
        /// <param name="point">The point.</param>
        /// <param name="points">The points.</param>
        /// <returns><c>true</c> if [is point in polygon] [the specified points]; otherwise, <c>false</c>.</returns>
        public static bool IsPointInPolygon(Location point, params Location[] points)
        {
            var i = 0;
            var j = points.Length - 1;
            var inPoly = false;

            for (i = 0; i < points.Length; i++)
            {
                if ((points[i].Longitude < point.Longitude && points[j].Longitude >= point.Longitude)
                  || (points[j].Longitude < point.Longitude && points[i].Longitude >= point.Longitude))
                {
                    if (points[i].Latitude + (((point.Longitude - points[i].Longitude) /
                      (points[j].Longitude - points[i].Longitude)) * (points[j].Latitude - points[i].Latitude)) < point.Latitude)
                    {
                        inPoly = !inPoly;
                    }
                }

                j = i;
            }

            return inPoly;
        }

        /// <summary>
        /// Determines a given polygon lies within the given RegionPolygons collection.
        /// </summary>
        /// <param name="subregions">The list of subregion</param>
        /// <param name="latitude">The latitude.</param>
        /// <param name="longitude">The longitude.</param>
        /// <returns>Boolean value indicating if the point lies inside region polygons collection.</returns>
        public static bool IsPointInRegionPolygons(List<RegionPolygonsCache> subregions, double latitude, double longitude)
        {
            bool isValidLocation = false;

            foreach (RegionPolygonsCache subregion in subregions)
            {
                var containingRegion = subregion.Contains(latitude, longitude);
                if (containingRegion != null)
                {
                    isValidLocation = true;
                    break;
                }
            }

            return isValidLocation;
        }

        /// <summary>
        /// Determines whether [is LPAUX in coverage] [the specified search point].
        /// </summary>
        /// <param name="searchPoint">The search point.</param>
        /// <param name="searchRadiusInMtr">The search radius in MTR.</param>
        /// <param name="lpauxDevice">The LPAUX device.</param>
        /// <returns><c>true</c> if [is LPAUX in coverage] [the specified search point]; otherwise, <c>false</c>.</returns>
        public static bool IsLpauxInCoverage(Location searchPoint, double searchRadiusInMtr, LPAuxRegistration lpauxDevice)
        {
            if (lpauxDevice.PointsArea != null && lpauxDevice.PointsArea.Any())
            {
                for (int i = 0; i < lpauxDevice.PointsArea.Length; i++)
                {
                    var distanceToPoint = GeoCalculations.GetDistance(searchPoint, lpauxDevice.PointsArea[i].ToLocation()).InMeter();
                    //// if distance between two lpaux centre points is greater than searchRadius + 1KM then they will newver overlap
                    if (distanceToPoint <= (searchRadiusInMtr + 1000))
                    {
                        return true;
                    }
                }
            }
            else if (lpauxDevice.QuadrilateralArea != null && lpauxDevice.QuadrilateralArea.Any())
            {
                for (int quadPointIndex = 0; quadPointIndex < lpauxDevice.QuadrilateralArea.Length; quadPointIndex++)
                {
                    var points = new[] { lpauxDevice.QuadrilateralArea[quadPointIndex].NEPoint, lpauxDevice.QuadrilateralArea[quadPointIndex].NWPoint, lpauxDevice.QuadrilateralArea[quadPointIndex].SEPoint, lpauxDevice.QuadrilateralArea[quadPointIndex].SWPoint };
                    for (int pointIndex = 0; pointIndex < points.Length; pointIndex++)
                    {
                        var distanceToPoint = GeoCalculations.GetDistance(searchPoint, points[pointIndex].ToLocation()).InMeter();
                        //// if distance between two lpaux centre points is greater than searchRadius + 1KM then they will newver overlap
                        if (distanceToPoint <= (searchRadiusInMtr + 1000))
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Calculates the radial contour.
        /// </summary>
        /// <param name="startLocation">The start location.</param>
        /// <param name="distanceInMtr">The distance in MTR.</param>
        /// <returns>returns List{Location}.</returns>
        public static List<Location> CalculateRadialContour(Location startLocation, double distanceInMtr)
        {
            // rotating azimuth at 1 degree intervals
            List<Location> deltaLocations = new List<Location>();

            for (int azimuth = 0; azimuth <= 360; azimuth++)
            {
                var deltaCoord = GeoCalculations.GetLocationTowardsBearing(startLocation, distanceInMtr, azimuth);
                deltaLocations.Add(deltaCoord);
            }

            return deltaLocations;
        }

        /// <summary>
        /// Determines whether [is point in polygon] [the specified locations].
        /// </summary>
        /// <param name="points">The locations.</param>
        /// <param name="point">The point.</param>
        /// <returns><c>true</c> if [is point in polygon] [the specified locations]; otherwise, <c>false</c>.</returns>
        public static bool RegionManagementIsPointInPolygon(List<Location> points, Location point)
        {
            var i = 0;
            var j = points.Count - 1;
            var inPoly = false;

            if (points.Count == 4)
            {
                for (i = 0; i < points.Count; i++)
                {
                    if ((points[i].Longitude < point.Longitude && points[j].Longitude >= point.Longitude)
                      || (points[j].Longitude < point.Longitude && points[i].Longitude >= point.Longitude))
                    {
                        if (points[i].Latitude + (((point.Longitude - points[i].Longitude) /
                          (points[j].Longitude - points[i].Longitude)) * (points[j].Latitude - points[i].Latitude)) < point.Latitude)
                        {
                            inPoly = true;
                        }
                    }

                    j = i;
                }
            }

            return inPoly;
        }

        /// <summary>
        /// Determines whether given point is in square
        /// </summary>
        /// <param name="searchArea">The search area.</param>
        /// <param name="point">The point.</param>
        /// <returns><c>true</c> if [is point in square] [the specified search area]; otherwise, <c>false</c>.</returns>
        public static bool IsPointInSquare(SquareArea searchArea, Location point)
        {
            if (point.Latitude <= searchArea.TopLeftPoint.Latitude && point.Latitude >= searchArea.BottomRightPoint.Latitude
               && point.Longitude >= searchArea.TopLeftPoint.Longitude && point.Longitude <= searchArea.BottomRightPoint.Longitude)
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Determines whether given point is in square
        /// </summary>
        /// <param name="contourPoints">The contour points.</param>
        /// <param name="transmitLocation">The transmitter location.</param>
        /// <param name="receiverLocation">The receiver location.</param>
        /// <param name="errors">The errors.</param>
        /// <returns><c>true</c> if [is point in square] [the specified search area]; otherwise, <c>false</c>.</returns>
        public static bool IsValidMVPDKeyHoleFromContour(List<Location> contourPoints, Location transmitLocation, Location receiverLocation, out List<string> errors)
        {
            errors = new List<string>();

            if (IsPointInPolygon(contourPoints, receiverLocation))
            {
                errors.Add(Constants.ErrorMessageMVPDInsideContour);
                return false;
            }

            //// bearing from trasnmitter to mvpd receiver
            var stationToParentTxBearing = GeoCalculations.CalculateBearing(transmitLocation, receiverLocation);
            if (double.IsNaN(stationToParentTxBearing))
            {
                errors.Add(Constants.ErrorMessageInvalidPoints);
                return false;
            }

            if (stationToParentTxBearing >= 360)
            {
                stationToParentTxBearing = stationToParentTxBearing - 360;
            }

            var stationToParentNearestPointOnContour = contourPoints[(int)stationToParentTxBearing];

            var distanceFromContourPointToReceiver = GeoCalculations.GetDistance(stationToParentNearestPointOnContour, receiverLocation);

            if (distanceFromContourPointToReceiver.InKm() > 80.0)
            {
                errors.Add(Constants.ErrorMessageMVPDOutsideValidDistance);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Determines whether translator is at valid distance
        /// </summary>
        /// <param name="contourPoints">The contour points.</param>
        /// <param name="transmiterLocation">The transmitter location.</param>
        /// <param name="receiverLocation">The receiver location.</param>
        /// <param name="logger">The logger.</param>
        /// <param name="callSign">The call sign.</param>
        /// <returns><c>true</c> if [is point in square] [the specified search area]; otherwise, <c>false</c>.</returns>
        public static bool IsValidTranslatorDistance(List<Location> contourPoints, Location transmiterLocation, Location receiverLocation, ILogger logger = null, string callSign = null)
        {
            if (IsPointInPolygon(contourPoints, receiverLocation))
            {
                if (logger != null)
                {
                    logger.Log(System.Diagnostics.TraceEventType.Information, LoggingMessageId.GenericMessage, string.Format("IsValidTranslatorDistance returns false, translator {0} is within contour points", callSign));
                }

                return false;
            }

            //// bearing from trasnmitter to mvpd receiver
            var stationToParentTxBearing = CalculateBearing(transmiterLocation, receiverLocation);

            if (stationToParentTxBearing >= 360)
            {
                stationToParentTxBearing = stationToParentTxBearing - 360;
            }

            var stationToParentNearestPointOnContour = contourPoints[(int)stationToParentTxBearing];
            var distanceFromContourPointToReceiver = GetDistance(stationToParentNearestPointOnContour, receiverLocation);

            if (distanceFromContourPointToReceiver.InKm() > 80.0)
            {
                var txtorxbearing = GeoCalculations.CalculateBearing(transmiterLocation, receiverLocation);
                var rxtocontourDistances = new List<double>();

                double distance = 0.0;
                bool isLessThen80KM = false;
                int index;
                for (int i = (int)txtorxbearing - 90; i < (int)txtorxbearing + 90; i++)
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

                    distance = GeoCalculations.GetDistance(receiverLocation, contourPoints[index]).Value / 1000.0;
                    if (distance <= 80.0)
                    {
                        isLessThen80KM = true;
                        break;
                    }

                    rxtocontourDistances.Add(distance);
                }

                if (!isLessThen80KM)
                {
                    var minimiumdistance = rxtocontourDistances.Min();
                    if (minimiumdistance > 80)
                    {
                        if (logger != null)
                        {
                            logger.Log(System.Diagnostics.TraceEventType.Information, LoggingMessageId.GenericMessage, string.Format("IsValidTranslatorDistance returns false, translator {0} is outside 80 km from contour points {1}", callSign, minimiumdistance));
                        }

                        return false;
                    }
                }
            }

            return true;
        }

        /// <summary>
        /// Gets the name of the national grid tile.
        /// </summary>
        public static void InitNationalGridTiles()
        {
            if (nationalGridTiles == null)
            {
                nationalGridTiles = new string[,]
                                    {
                                        {
                                            "SV", string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, "NL", "NF", "NA", string.Empty, string.Empty, string.Empty
                                        },
                                        {
                                            "SW", "SR", "SM", string.Empty, string.Empty, "NW", "NR", "NM", "NG", "NB", "HW", string.Empty, string.Empty
                                        },
                                        {
                                            "SX", "SS", "SN", "SH", "SC", "NX", "NS", "NN", "NH", "NC", "HX", string.Empty, string.Empty
                                        },
                                        {
                                            "SY", "ST", "SO", "SJ", "SD", "NY", "NT", "NO", "NJ", "ND", "HY", "HT", string.Empty
                                        },
                                        {
                                            "SZ", "SU", "SP", "SK", "SE", "NZ", "NU", string.Empty, "NK", string.Empty, "HZ", "HU", "HP"
                                        },
                                        {
                                            "TV", "TQ", "TL", "TF", "TA", string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, string.Empty
                                        },
                                        {
                                            string.Empty, "TR", "TM", "TG", string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, string.Empty
                                        },
                                    };
            }
        }

        /// <summary>
        /// Gets the name of the national grid tile.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <returns>returns System.String.</returns>
        public static string GetNationalGridTileName(int easting, int northing)
        {
            InitNationalGridTiles();

            int mappedEasting = easting / 100000;
            int mappedNorthing = northing / 100000;

            return nationalGridTiles[mappedEasting, mappedNorthing];
        }

        /// <summary>
        /// Generates the overlapping coordinates.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <param name="deltaX">The delta x.</param>
        /// <param name="deltaY">The delta y.</param>
        /// <param name="str">The string.</param>
        /// <returns>returns coordinates.</returns>
        public static List<OSGLocation> GenerateOverlappingCoordinatesOf100Mtrs(double easting, double northing, double deltaX, double deltaY, StringBuilder str)
        {
            var pairs = new List<OSGLocation>();
            var dx = deltaX;
            var dy = deltaY;

            double topLeftEasting = easting - dx;
            double topLeftNorthing = northing + dy;

            double bottomRightEasting = easting + dx;
            double bottomRightNorthing = northing - dy;

            if (str != null)
            {
                str.Append("\n topLeftEasting" + topLeftEasting);
                str.Append("\n topLeftNorthing" + topLeftNorthing);
                str.Append("\n bottomRightEasting" + bottomRightEasting);
                str.Append("\n bottomRightNorthing" + bottomRightNorthing);
            }

            double x = 0, y = 0;

            var topLeftEastingNorthingRound100 = Conversion.RoundTo100(topLeftEasting, topLeftNorthing);
            var bottomRightEastingNorthingRound100 = Conversion.RoundTo100(bottomRightEasting, bottomRightNorthing);

            if (str != null)
            {
                str.Append("\n ----------- After RoundingOff-----------");
                str.Append("\n topLeftEasting" + topLeftEastingNorthingRound100.Easting);
                str.Append("\n topLeftNorthing" + topLeftEastingNorthingRound100.Northing);
                str.Append("\n bottomRightEasting" + bottomRightEastingNorthingRound100.Easting);
                str.Append("\n bottomRightNorthing" + bottomRightEastingNorthingRound100.Northing);
                str.Append("\n ----------- Coordinate pairs-----------");
            }

            for (y = bottomRightEastingNorthingRound100.Northing; y <= topLeftEastingNorthingRound100.Northing; y += 100)
            {
                for (x = topLeftEastingNorthingRound100.Easting; x <= bottomRightEastingNorthingRound100.Easting; x += 100)
                {
                    var convertTo100mtrs = Conversion.RoundTo100(x, y);
                    var curPair = new OSGLocation(convertTo100mtrs.Easting, convertTo100mtrs.Northing);
                    if (str != null)
                    {
                        str.Append("\n Coordinates " + curPair.Easting.ToString() + "," + curPair.Northing.ToString());
                    }

                    pairs.Add(curPair);
                }
            }

            return pairs;
        }

        /// <summary>
        /// Generates the overlapping coordinates of 10 MTRS.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <param name="deltaX">The delta x.</param>
        /// <param name="deltaY">The delta y.</param>
        /// <param name="str">The string.</param>
        /// <returns>returns locations.</returns>
        public static List<OSGLocation> GenerateOverlappingCoordinatesOf10Mtrs(double easting, double northing, double deltaX, double deltaY, StringBuilder str)
        {
            List<OSGLocation> osgLocation = new List<OSGLocation>();

            var dx = deltaX + 100;
            var dy = deltaY + 100;

            double topLeftEasting = easting - dx;
            double topLeftNorthing = northing + dy;

            double bottomRightEasting = easting + dx;
            double bottomRightNorthing = northing - dy;

            if (str != null)
            {
                str.Append("\n topLeftEasting" + topLeftEasting);
                str.Append("\n topLeftNorthing" + topLeftNorthing);
                str.Append("\n bottomRightEasting" + bottomRightEasting);
                str.Append("\n bottomRightNorthing" + bottomRightNorthing);
            }

            var topLeftEastingNorthingRound10 = Conversion.RoundTo10(topLeftEasting, topLeftNorthing);
            var bottomRightEastingNorthingRound10 = Conversion.RoundTo10(bottomRightEasting, bottomRightNorthing);

            if (str != null)
            {
                str.Append("\n ----------- After RoundingOff-----------");
                str.Append("\n topLeftEasting" + topLeftEastingNorthingRound10.Easting);
                str.Append("\n topLeftNorthing" + topLeftEastingNorthingRound10.Northing);
                str.Append("\n bottomRightEasting" + bottomRightEastingNorthingRound10.Easting);
                str.Append("\n bottomRightNorthing" + bottomRightEastingNorthingRound10.Northing);
                str.Append("\n ----------- Coordinate pairs----------- \n");
            }

            for (double y = topLeftEastingNorthingRound10.Northing; y >= bottomRightEastingNorthingRound10.Northing; y -= 10)
            {
                for (double x = topLeftEastingNorthingRound10.Easting; x <= bottomRightEastingNorthingRound10.Easting; x += 10)
                {
                    var curPair = new OSGLocation((int)x, (int)y);
                    if (str != null)
                    {
                        str.Append("\n Coordinates " + curPair.Easting.ToString() + "," + curPair.Northing.ToString());
                    }

                    osgLocation.Add(curPair);
                }
            }

            return osgLocation.ToList<OSGLocation>();
        }

        /// <summary>
        /// Generates the WSD candidate location of10 MTRS.
        /// </summary>
        /// <param name="wsdlocation">The WSD location.</param>
        /// <param name="deltaX">The delta x.</param>
        /// <param name="deltaY">The delta y.</param>
        /// <param name="str">The string.</param>
        /// <returns>returns locations.</returns>
        public static List<OSGLocation> GenerateWsdCandidateLocationOf10Mtrs(OSGLocation wsdlocation, double deltaX, double deltaY, StringBuilder str)
        {
            List<OSGLocation> osgLocation = new List<OSGLocation>();
            var roundedWsdLocation10 = Conversion.RoundTo10(wsdlocation);

            // round off to 10 mtr only if deltax or deltay less than 100
            if (deltaX < 100)
            {
                deltaX = (int)deltaX / 10 == 0 ? 0 : (((int)deltaX / 10) * 10) + 10;
            }

            if (deltaY < 100)
            {
                deltaY = (int)deltaY / 10 == 0 ? 0 : (((int)deltaY / 10) * 10) + 10;
            }

            double bottomLeftEasting = deltaX == 0.0 ? roundedWsdLocation10.Easting : roundedWsdLocation10.Easting - deltaX;
            double bottomLeftNorthing = deltaY == 0.0 ? roundedWsdLocation10.Northing : roundedWsdLocation10.Northing - deltaY;

            double topLeftEasting = bottomLeftEasting;
            double topLeftNorthing = deltaY == 0.0 ? bottomLeftNorthing + 10 : bottomLeftNorthing + (2 * deltaY);

            double topRightEasting = deltaX == 0.0 ? topLeftEasting + 10 : topLeftEasting + (2 * deltaX);
            double topRightNorthing = topLeftNorthing;

            double bottomRightEasting = topRightEasting;
            double bottomRightNorthing = bottomLeftNorthing;

            // Expecting accuracy of 3 decimal digits for easting i.e upto Milli meters, to decide whether WSD easting is a overlapping with one of the candidate location. 
            if (deltaX >= 100 && wsdlocation.OriginalEasting % 10 >= 0.001)
            {
                // Original Easting is not on a multiple of 10 boundary, so need to increase the Easting by 10 to include partial overlaps.
                topRightEasting += 10;
                bottomRightEasting += 10;
            }

            // Expecting accuracy of 3 decimal digits for northing i.e upto Milli meters, to decide whether WSD northing is a overlapping with one of the candidate location. 
            if (deltaY >= 100 && wsdlocation.OriginalNorthing % 10 >= 0.001)
            {
                // Original Northing is not on a multiple of 10 boundary, so need to increase the Northing by 10 to include partial overlaps.
                topLeftNorthing += 10;
                topRightNorthing += 10;
            }

            for (double y = topLeftNorthing; y >= bottomRightNorthing; y -= 10)
            {
                for (double x = topLeftEasting; x <= bottomRightEasting; x += 10)
                {
                    var curPair = new OSGLocation((int)x, (int)y);
                    osgLocation.Add(curPair);
                }
            }

            return osgLocation;
        }

        /// <summary>
        /// Generates the WSD candidate location of10 MTRS for coverage area.
        /// </summary>
        /// <param name="wsdlatLon">The WSD latitude and longitude</param>
        /// <param name="wsdlocation">The WSD location.</param>
        /// <param name="radius">The radius.</param>
        /// <returns>returns locations.</returns>
        public static List<OSGLocation> GenerateWsdCandidateLocationOf10MtrsForCoverageArea(Location wsdlatLon, OSGLocation wsdlocation, double radius)
        {
            List<OSGLocation> osgLocation = new List<OSGLocation>();

            double topLeftEasting = wsdlocation.Easting - 10;
            double topLeftNorthing = wsdlocation.Northing + 110;

            double bottomRightEasting = wsdlocation.Easting + 110;
            double bottomRightNorthing = wsdlocation.Northing - 10;

            for (double y = topLeftNorthing; y >= bottomRightNorthing; y -= 10)
            {
                for (double x = topLeftEasting; x <= bottomRightEasting; x += 10)
                {
                    // get all 4 corners of this tile
                    var subCordinates = new OSGLocation[4];
                    subCordinates[0] = new OSGLocation(x, y);
                    subCordinates[1] = new OSGLocation(x + 9, y + 9);
                    subCordinates[2] = new OSGLocation(x + 9, y);
                    subCordinates[3] = new OSGLocation(x, y + 9);

                    if (x == 532280 && y == 181530)
                    {
                    }

                    for (int i = 0; i < subCordinates.Length; i++)
                    {
                        var tile = GeoCalculations.GetLatitudeLongitude(subCordinates[i].Easting, subCordinates[i].Northing);
                        var distance = (int)GeoCalculations.GetDistance(wsdlatLon, tile).InMeter() / 10 * 10;
                        ////var distance = Math.Sqrt(Math.Pow(Math.Abs(wsdlocation.Easting - subCordinates[i].Easting), 2) + Math.Pow(Math.Abs(wsdlocation.Northing - subCordinates[i].Northing), 2));
                        if (distance <= ((int)radius / 10 * 10) || distance <= 10)
                        {
                            osgLocation.Add(subCordinates[0]);
                            break;
                        }
                    }
                }
            }

            return osgLocation;
        }

        /// <summary>
        /// Generates the overlapping coordinates of10 MTRS for coverage area using square.
        /// </summary>
        /// <param name="wsdLocation">The WSD location.</param>
        /// <param name="wsdOSGlocation">The WSD OSG location.</param>
        /// <param name="deltaX">The delta x.</param>
        /// <param name="deltaY">The delta y.</param>
        /// <param name="coverageAreaInMtr">The coverage area in MTR.</param>
        /// <returns>returns locations.</returns>
        public static List<OSGLocation> GenerateOverlappingCoordinatesOf10MtrsForCoverageAreaUsingSquare(Location wsdLocation, OSGLocation wsdOSGlocation, double deltaX, double deltaY, double coverageAreaInMtr)
        {
            List<OSGLocation> osgLocation = new List<OSGLocation>();

            var radius = Math.Sqrt(Math.Pow(deltaX, 2) + Math.Pow(deltaY, 2)) + coverageAreaInMtr;

            var squareside = radius * Math.Sqrt(2);

            int innerSquareTopLeftEasting = 0, innerSquareTopLeftNorthing = 0, innerSquareBottomRightEasting = 0, innerSquareBottomRightNorthing = 0;
            if (coverageAreaInMtr > 8000)
            {
                innerSquareTopLeftEasting = Conversion.RoundTo100(wsdOSGlocation.Easting - (squareside / 2.0));
                innerSquareTopLeftNorthing = Conversion.RoundTo100(wsdOSGlocation.Northing + (squareside / 2.0));

                innerSquareBottomRightEasting = Conversion.RoundTo100(wsdOSGlocation.Easting + (squareside / 2.0));
                innerSquareBottomRightNorthing = Conversion.RoundTo100(wsdOSGlocation.Northing - (squareside / 2.0));
            }

            var eastingNorthing = GeoCalculations.GetEastingNorthing(wsdLocation);

            double topLeftEasting = eastingNorthing.Easting - radius;
            double topLeftNorthing = eastingNorthing.Northing + radius;

            double bottomRightEasting = eastingNorthing.Easting + radius;
            double bottomRightNorthing = eastingNorthing.Northing - radius;

            var topLeftEastingNorthingRound100 = Conversion.RoundTo10(topLeftEasting, topLeftNorthing);
            var bottomRightEastingNorthingRound100 = Conversion.RoundTo10(bottomRightEasting, bottomRightNorthing);

            for (int y = topLeftEastingNorthingRound100.Northing; y >= bottomRightEastingNorthingRound100.Northing; y -= 10)
            {
                for (int x = topLeftEastingNorthingRound100.Easting; x <= bottomRightEastingNorthingRound100.Easting; x += 10)
                {
                    if (coverageAreaInMtr > 8000)
                    {
                        if (x >= innerSquareTopLeftEasting && x <= innerSquareBottomRightEasting && y <= innerSquareTopLeftNorthing && y >= innerSquareBottomRightNorthing)
                        {
                            osgLocation.Add(new OSGLocation(x, y));
                            continue;
                        }
                    }

                    // get all 4 corners of this tile
                    var subCordinates = new OSGLocation[4];
                    subCordinates[0] = new OSGLocation(x, y);
                    subCordinates[1] = new OSGLocation(x + 9, y + 9);
                    subCordinates[2] = new OSGLocation(x + 9, y);
                    subCordinates[3] = new OSGLocation(x, y + 9);

                    for (int i = 0; i < subCordinates.Length; i++)
                    {
                        var tile = GeoCalculations.GetLatitudeLongitude(subCordinates[i].Easting, subCordinates[i].Northing);
                        var distance = (int)GeoCalculations.GetDistance(wsdLocation, tile).InMeter();
                        if (distance <= (int)radius || distance <= 10)
                        {
                            ////for (int pairNorthing = subCordinates[0].Northing; pairNorthing + 100 >= subCordinates[0].Northing; pairNorthing -= 10)
                            ////{
                            ////    for (int pairEasting = subCordinates[0].Easting; pairEasting <= subCordinates[0].Easting + 100; pairEasting += 10)
                            ////    {
                            ////        osgLocation.Add(new OSGLocation(pairEasting, pairNorthing));
                            ////    }
                            ////}
                            osgLocation.Add(subCordinates[0]);
                            break;
                        }
                    }
                }
            }

            return osgLocation;
        }

        /// <summary>
        /// Generates the overlapping coordinates of10 MTRS for coverage area using square_ test.
        /// </summary>
        /// <param name="wsdLocation">The WSD location.</param>
        /// <param name="wsdOSGlocation">The WSD OSG location.</param>
        /// <param name="deltaX">The delta x.</param>
        /// <param name="deltaY">The delta y.</param>
        /// <param name="coverageAreaInMtr">The coverage area in MTR.</param>
        /// <returns>returns locations;.</returns>
        public static List<OSGLocation> GenerateOverlappingCoordinatesOf10MtrsForCoverageAreaUsingSquare_Test(Location wsdLocation, OSGLocation wsdOSGlocation, double deltaX, double deltaY, double coverageAreaInMtr)
        {
            List<OSGLocation> osgLocation = new List<OSGLocation>();
            List<Tuple<OSGLocation, OSGLocation>> endpoints = new List<Tuple<OSGLocation, OSGLocation>>();

            var radius = Math.Sqrt(Math.Pow(deltaX, 2) + Math.Pow(deltaY, 2)) + coverageAreaInMtr;
            var eastingNorthing = GeoCalculations.GetEastingNorthing(wsdLocation);

            double topLeftEasting = eastingNorthing.Easting - radius;
            double topLeftNorthing = eastingNorthing.Northing + radius;

            double bottomRightEasting = eastingNorthing.Easting + radius;
            double bottomRightNorthing = eastingNorthing.Northing - radius;

            var topLeftEastingNorthingRound100 = Conversion.RoundTo10(topLeftEasting, topLeftNorthing);
            var bottomRightEastingNorthingRound100 = Conversion.RoundTo10(bottomRightEasting, bottomRightNorthing);

            for (int y = topLeftEastingNorthingRound100.Northing; y >= bottomRightEastingNorthingRound100.Northing; y -= 10)
            {
                for (int x = topLeftEastingNorthingRound100.Easting; x <= bottomRightEastingNorthingRound100.Easting; x += 10)
                {
                    ////double startEasting
                    // get all 4 corners of this tile
                    var subCordinates = new OSGLocation[4];
                    subCordinates[0] = new OSGLocation(x, y);
                    subCordinates[1] = new OSGLocation(x + 9, y + 9);
                    subCordinates[2] = new OSGLocation(x + 9, y);
                    subCordinates[3] = new OSGLocation(x, y + 9);

                    for (int i = 0; i < subCordinates.Length; i++)
                    {
                        var tile = GeoCalculations.GetLatitudeLongitude(subCordinates[i].Easting, subCordinates[i].Northing);
                        var distance = (int)GeoCalculations.GetDistance(wsdLocation, tile).InMeter();
                        if (distance <= (int)radius || distance <= 10)
                        {
                            ////for (int pairNorthing = subCordinates[0].Northing; pairNorthing + 100 >= subCordinates[0].Northing; pairNorthing -= 10)
                            ////{
                            ////    for (int pairEasting = subCordinates[0].Easting; pairEasting <= subCordinates[0].Easting + 100; pairEasting += 10)
                            ////    {
                            ////        osgLocation.Add(new OSGLocation(pairEasting, pairNorthing));
                            ////    }
                            ////}
                            osgLocation.Add(subCordinates[0]);
                            break;
                        }
                    }
                }

                for (int x = topLeftEastingNorthingRound100.Easting; x <= bottomRightEastingNorthingRound100.Easting; x += 10)
                {
                    // get all 4 corners of this tile
                    var subCordinates = new OSGLocation[4];
                    subCordinates[0] = new OSGLocation(x, y);
                    subCordinates[1] = new OSGLocation(x + 9, y + 9);
                    subCordinates[2] = new OSGLocation(x + 9, y);
                    subCordinates[3] = new OSGLocation(x, y + 9);

                    for (int i = 0; i < subCordinates.Length; i++)
                    {
                        var tile = GeoCalculations.GetLatitudeLongitude(subCordinates[i].Easting, subCordinates[i].Northing);
                        var distance = (int)GeoCalculations.GetDistance(wsdLocation, tile).InMeter();
                        if (distance <= (int)radius || distance <= 10)
                        {
                            ////for (int pairNorthing = subCordinates[0].Northing; pairNorthing + 100 >= subCordinates[0].Northing; pairNorthing -= 10)
                            ////{
                            ////    for (int pairEasting = subCordinates[0].Easting; pairEasting <= subCordinates[0].Easting + 100; pairEasting += 10)
                            ////    {
                            ////        osgLocation.Add(new OSGLocation(pairEasting, pairNorthing));
                            ////    }
                            ////}
                            osgLocation.Add(subCordinates[0]);
                            break;
                        }
                    }
                }
            }

            return osgLocation;
        }

        /// <summary>
        /// Generates the overlapping coordinates of100 MTRS for coverage area.
        /// </summary>
        /// <param name="wsdLocation">The WSD location.</param>
        /// <param name="deltaX">The delta x.</param>
        /// <param name="deltaY">The delta y.</param>
        /// <param name="str">The string.</param>
        /// <param name="coverageAreaInMtr">The coverage area in MTR.</param>
        /// <returns>returns list of locations.</returns>
        public static List<OSGLocation> GenerateOverlappingCoordinatesOf100MtrsForCoverageAreaUsingSquare(Location wsdLocation, double deltaX, double deltaY, StringBuilder str, double coverageAreaInMtr)
        {
            var pairs = new List<OSGLocation>();
            var radius = Math.Sqrt(Math.Pow(deltaX, 2) + Math.Pow(deltaY, 2)) + coverageAreaInMtr;
            var eastingNorthing = GeoCalculations.GetEastingNorthingWithDoublePrecision(wsdLocation);

            double topLeftEasting = eastingNorthing.OriginalEasting - radius;
            double topLeftNorthing = eastingNorthing.OriginalNorthing + radius;

            double bottomRightEasting = eastingNorthing.OriginalEasting + radius;
            double bottomRightNorthing = eastingNorthing.OriginalNorthing - radius;

            var topLeftEastingNorthingRound100 = Conversion.RoundTo100(topLeftEasting, topLeftNorthing);
            var bottomRightEastingNorthingRound100 = Conversion.RoundTo100(bottomRightEasting, bottomRightNorthing);

            List<OSGLocation> edgeOverlappigs = new List<OSGLocation>();

            edgeOverlappigs.Add(new OSGLocation(topLeftEasting, eastingNorthing.OriginalNorthing));
            edgeOverlappigs.Add(new OSGLocation(bottomRightEasting, eastingNorthing.OriginalNorthing));
            edgeOverlappigs.Add(new OSGLocation(eastingNorthing.OriginalEasting, topLeftNorthing));
            edgeOverlappigs.Add(new OSGLocation(eastingNorthing.OriginalEasting, bottomRightNorthing));

            for (int y = bottomRightEastingNorthingRound100.Northing; y <= topLeftEastingNorthingRound100.Northing; y += 100)
            {
                for (int x = topLeftEastingNorthingRound100.Easting; x <= bottomRightEastingNorthingRound100.Easting; x += 100)
                {
                    // get all 4 corners of this tile
                    var subCordinates = new OSGLocation[4];
                    subCordinates[0] = new OSGLocation(x, y);
                    subCordinates[1] = new OSGLocation(x + 100, y + 100);
                    subCordinates[2] = new OSGLocation(x + 100, y);
                    subCordinates[3] = new OSGLocation(x, y + 100);

                    bool isOverlapped = false;

                    for (int i = 0; i < subCordinates.Length; i++)
                    {
                        // Pythagorean theorem to calculate the distance b/w the co-ordinates.
                        double distance = Math.Sqrt(Math.Pow(Math.Abs(subCordinates[i].Northing - eastingNorthing.OriginalNorthing), 2) + Math.Pow(Math.Abs(subCordinates[i].Easting - eastingNorthing.OriginalEasting), 2));

                        if (distance <= radius)
                        {
                            isOverlapped = true;
                            break;
                        }
                    }

                    // Case to handle candidate pixels edge overlapping.
                    if (!isOverlapped && (x == topLeftEastingNorthingRound100.Easting || x == bottomRightEastingNorthingRound100.Easting || y == bottomRightEastingNorthingRound100.Northing || y == topLeftEastingNorthingRound100.Northing))
                    {
                        isOverlapped = edgeOverlappigs.Any((edge) =>
                        {
                            double minEasting = subCordinates.Min(osgLocation => osgLocation.Easting);
                            double maxEasting = subCordinates.Max(osgLocation => osgLocation.Easting);
                            double minNorthing = subCordinates.Min(osgLocation => osgLocation.Northing);
                            double maxNorthing = subCordinates.Max(osgLocation => osgLocation.Northing);

                            return edge.OriginalEasting > minEasting && edge.OriginalEasting < maxEasting && edge.OriginalNorthing > minNorthing && edge.OriginalNorthing < maxNorthing;
                        });
                    }

                    if (isOverlapped)
                    {
                        pairs.Add(subCordinates[0]);
                    }
                }
            }

            return pairs.Distinct(new DistinctOSGLocations()).ToList();
        }

        /// <summary>
        /// Generate candidate locations overlaps with the boundary line of WSD coverage radius and filter them into 4 sectors.
        /// </summary>
        /// <param name="wsdLocation">The WSD location.</param>
        /// <param name="deltaX">Horizontal uncertainty in meters</param>
        /// <param name="deltaY">Vertical uncertainty in meters</param>
        /// <param name="coverageAreaInMtr">The WSD coverage range in meters</param>
        /// <returns>Candidate locations belong to 4 different sectors</returns>
        public static List<OSGLocation>[] GenerateWsdCandidateLocationOf10MtrsForCoverageAreaBoundary(Location wsdLocation, double deltaX, double deltaY, double coverageAreaInMtr)
        {
            List<OSGLocation>[] candidateLocationSectors = new List<OSGLocation>[4];

            for (int index = 0; index < candidateLocationSectors.Length; index++)
            {
                candidateLocationSectors[index] = new List<OSGLocation>();
            }

            double radius = Math.Sqrt(Math.Pow(deltaX, 2) + Math.Pow(deltaY, 2)) + coverageAreaInMtr;

            OSGLocation wsdOSGlocation = GeoCalculations.GetEastingNorthingWithDoublePrecision(wsdLocation);
            OSGLocation wsdOSGLocationRoundTo10 = Conversion.RoundTo10(wsdOSGlocation);

            // Top Left corner of a square having length = breadth = radius
            double topLeftEasting = wsdOSGlocation.OriginalEasting - radius;
            double topLeftNorthing = wsdOSGlocation.OriginalNorthing + radius;

            // Bottom Right corner of a square having length = breadth = radius
            double bottomRightEasting = wsdOSGlocation.OriginalEasting + radius;
            double bottomRightNorthing = wsdOSGlocation.OriginalNorthing - radius;

            var topLeftEastingNorthingRound10 = Conversion.RoundTo10(topLeftEasting, topLeftNorthing);
            var bottomRightEastingNorthingRound10 = Conversion.RoundTo10(bottomRightEasting, bottomRightNorthing);

            List<OSGLocation> edgeOverlappings = new List<OSGLocation>();

            edgeOverlappings.Add(new OSGLocation(topLeftEasting, wsdOSGlocation.OriginalNorthing));
            edgeOverlappings.Add(new OSGLocation(bottomRightEasting, wsdOSGlocation.OriginalNorthing));
            edgeOverlappings.Add(new OSGLocation(wsdOSGlocation.OriginalEasting, topLeftNorthing));
            edgeOverlappings.Add(new OSGLocation(wsdOSGlocation.OriginalEasting, bottomRightNorthing));

            for (double northing = bottomRightEastingNorthingRound10.Northing; northing <= topLeftEastingNorthingRound10.Northing; northing += 10)
            {
                // Logic to obtain left half candidate locations.
                for (double easting = topLeftEastingNorthingRound10.Easting; easting < wsdOSGLocationRoundTo10.Easting; easting += 10)
                {
                    bool isOverlapped;

                    ClassifyCandidateLocationsToSectors(new OSGLocation(easting, northing), edgeOverlappings, wsdOSGlocation, topLeftEastingNorthingRound10, bottomRightEastingNorthingRound10, radius, candidateLocationSectors, out isOverlapped);

                    // Break out moment when a first left overlapping candidate location found for northing values other than bottom left/right northing or top left/right northing.
                    if (northing != bottomRightEastingNorthingRound10.Northing && northing != topLeftEastingNorthingRound10.Northing && isOverlapped)
                    {
                        break;
                    }
                }

                // Logic to obtain right half candidate locations.
                for (double easting = bottomRightEastingNorthingRound10.Easting; easting >= wsdOSGLocationRoundTo10.Easting; easting -= 10)
                {
                    bool isOverlapped;

                    ClassifyCandidateLocationsToSectors(new OSGLocation(easting, northing), edgeOverlappings, wsdOSGlocation, topLeftEastingNorthingRound10, bottomRightEastingNorthingRound10, radius, candidateLocationSectors, out isOverlapped);

                    // Break out moment when a first right overlapping candidate location found for northing values other than bottom left/right northing or top left/right northing.
                    if (northing != bottomRightEastingNorthingRound10.Northing && northing != topLeftEastingNorthingRound10.Northing && isOverlapped)
                    {
                        break;
                    }
                }
            }

            // Remove duplicates in each sectors.
            for (int index = 0; index < candidateLocationSectors.Length; index++)
            {
                candidateLocationSectors[index] = candidateLocationSectors[index].Distinct(new DistinctOSGLocations()).ToList();
            }

            return candidateLocationSectors;
        }

        /// <summary>
        /// Calculates index of coverage radius circle sector (0,1,2,3) for which a candidate location belongs to.
        /// </summary>
        /// <param name="candidateLocation">OSGLocation of a candidate location.</param>
        /// <param name="wsdOsgLocation">OSGLocation of a WSD device.</param>
        /// <returns>Index of Coverage are circle sector.</returns>
        public static int FindCandidateLocationSectorIndex(OSGLocation candidateLocation, OSGLocation wsdOsgLocation)
        {
            int sectorIndex = 0;

            // Condition to identify Candidate location belongs to upper half or lower half of coverage area.
            if (candidateLocation.OriginalNorthing >= wsdOsgLocation.Northing)
            {
                // Condition to identify Candidate location belongs to upper left or right quarter of coverage area.
                if (candidateLocation.OriginalEasting < wsdOsgLocation.Easting)
                {
                    sectorIndex = 0;
                }
                else
                {
                    sectorIndex = 1;
                }
            }
            else
            {
                // Condition to identify Candidate location belongs to lower left or right quarter of coverage area.
                if (candidateLocation.OriginalEasting >= wsdOsgLocation.Easting)
                {
                    sectorIndex = 2;
                }
                else
                {
                    sectorIndex = 3;
                }
            }

            return sectorIndex;
        }

        /// <summary>
        /// check if the overlapping coordinates.
        /// </summary>
        /// <param name="targetPoints">The target points inside which to check.</param>
        /// <param name="sourcePoints">The source points.</param>
        /// <param name="sourceQuadPoints">The source quad points.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public static bool IsOverlappingCoordinates(Location[] targetPoints, Position[] sourcePoints = null, QuadrilateralArea[] sourceQuadPoints = null)
        {
            if (sourcePoints != null)
            {
                for (int i = 0; i < sourcePoints.Length; i++)
                {
                    if (IsPointInPolygon(targetPoints, sourcePoints[i].ToLocation()))
                    {
                        return true;
                    }
                }
            }
            else if (sourceQuadPoints != null)
            {
                for (int i = 0; i < sourceQuadPoints.Length; i++)
                {
                    var quadPoint = sourceQuadPoints[i];
                    if (IsPointInPolygon(targetPoints, quadPoint.NEPoint.ToLocation()))
                    {
                        return true;
                    }

                    if (IsPointInPolygon(targetPoints, quadPoint.SEPoint.ToLocation()))
                    {
                        return true;
                    }

                    if (IsPointInPolygon(targetPoints, quadPoint.NWPoint.ToLocation()))
                    {
                        return true;
                    }

                    if (IsPointInPolygon(targetPoints, quadPoint.SWPoint.ToLocation()))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Determines whether [is overlapping coordinates] [the specified source].
        /// </summary>
        /// <param name="searchArea">The source.</param>
        /// <param name="sourcePoint">The target.</param>
        /// <returns><c>true</c> if [is overlapping coordinates] [the specified source]; otherwise, <c>false</c>.</returns>
        public static bool IsOverlappingCoordinates(LPAuxRegistration searchArea, LPAuxRegistration sourcePoint)
        {
            if (searchArea.PointsArea != null && sourcePoint.PointsArea != null)
            {
                if (searchArea.PointsArea.Length > 1)
                {
                    if (IsPointInPolygon(searchArea.PointsArea.Select(obj => obj.ToLocation()).ToArray(), sourcePoint.PointsArea[0].ToLocation()))
                    {
                        return true;
                    }
                }
            }
            else if (searchArea.QuadrilateralArea != null && sourcePoint.QuadrilateralArea != null)
            {
                for (int i = 0; i < searchArea.QuadrilateralArea.Length; i++)
                {
                    SquareArea squareArea = new SquareArea(searchArea.QuadrilateralArea[i].NWPoint.ToLocation(), searchArea.QuadrilateralArea[i].SEPoint.ToLocation());
                    for (int j = 0; j < sourcePoint.QuadrilateralArea.Length; j++)
                    {
                        if (IsPointInSquare(squareArea, sourcePoint.QuadrilateralArea[j].NEPoint.ToLocation()))
                        {
                            return true;
                        }

                        if (IsPointInSquare(squareArea, sourcePoint.QuadrilateralArea[j].NWPoint.ToLocation()))
                        {
                            return true;
                        }

                        if (IsPointInSquare(squareArea, sourcePoint.QuadrilateralArea[j].SEPoint.ToLocation()))
                        {
                            return true;
                        }

                        if (IsPointInSquare(squareArea, sourcePoint.QuadrilateralArea[j].SWPoint.ToLocation()))
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Method to convert latitude/longitude from decimal degrees to degrees minutes seconds format
        /// </summary>
        /// <param name="decimalValue">The decimal value</param>
        /// <returns>returns degree, minute, second.</returns>
        public static Tuple<int, int, double> GetDegreeMinuteSecond(double decimalValue)
        {
            double usedDegree = Math.Abs(decimalValue);
            int degree = (int)usedDegree;
            double temp = (usedDegree - degree) * 60.0;
            int minute = (int)temp;
            double second = (temp - minute) * 60.0;

            if (minute == 60)
            {
                minute = 0;
                degree = degree + 1;
            }

            if (second < 1.5e-5)
            {
                second = 0.0;
            }

            if (second > 60.0 - 1.5e-5)
            {
                second = 0.0;
                minute = minute + 1;
            }

            if (decimalValue < 0)
            {
                degree = -degree;
            }

            return new Tuple<int, int, double>(degree, minute, second.ToString("F2").ToDouble());
        }

        /// <summary>
        /// Checks the LPAUX overlap.
        /// </summary>
        /// <param name="contourPoints">The contour points.</param>
        /// <param name="stationLocation">The station location.</param>
        /// <param name="pointsArea">The points area.</param>
        /// <param name="quadArea">The quad area.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public static bool CheckIfLpAuxOverlap(List<Location> contourPoints, Location stationLocation, Point[] pointsArea = null, QuadrilateralArea[] quadArea = null)
        {
            if (pointsArea != null)
            {
                for (int i = 0; i < pointsArea.Length; i++)
                {
                    var deviceLocation = pointsArea[i].ToLocation();
                    if (IsPointInPolygon(contourPoints, deviceLocation))
                    {
                        return true;
                    }

                    var deviceToStationBearing = GeoCalculations.CalculateBearing(stationLocation, deviceLocation);
                    var deviceToContourPointDistance = GeoCalculations.GetDistance(deviceLocation, contourPoints[(int)deviceToStationBearing]);

                    // check if distance between station contour point and lpaux device point is less than 1km
                    if (deviceToContourPointDistance.InKm() <= 1)
                    {
                        return true;
                    }
                }
            }
            else if (quadArea != null)
            {
                for (int i = 0; i < quadArea.Length; i++)
                {
                    QuadrilateralArea deviceArea = quadArea[i];
                    List<Location> locations = new List<Location>();

                    var nwpoint = deviceArea.NWPoint.ToLocation();
                    var nepoint = deviceArea.NEPoint.ToLocation();
                    var sepoint = deviceArea.SEPoint.ToLocation();
                    var swpoint = deviceArea.SWPoint.ToLocation();

                    // mid points of each side
                    var nwnemidpoint = new Location(nepoint.Latitude, (nepoint.Longitude + nwpoint.Longitude) / 2.0);
                    var nesemidpoint = new Location((nepoint.Latitude + sepoint.Latitude) / 2.0, nepoint.Longitude);
                    var seswmidpoint = new Location(sepoint.Latitude, (sepoint.Longitude + swpoint.Longitude) / 2.0);
                    var swnwmidpoint = new Location((swpoint.Latitude + nwpoint.Latitude) / 2.0, swpoint.Longitude);

                    locations.AddRange(new[] { nwpoint, nepoint, sepoint, swpoint, nwnemidpoint, nesemidpoint, seswmidpoint, swnwmidpoint });

                    for (int locationCount = 0; locationCount < locations.Count; locationCount++)
                    {
                        var deviceLocation = locations[i];
                        if (IsPointInPolygon(contourPoints, deviceLocation))
                        {
                            return true;
                        }

                        var deviceToStationBearing = GeoCalculations.CalculateBearing(stationLocation, deviceLocation);
                        var deviceToContourPointDistance = GeoCalculations.GetDistance(deviceLocation, contourPoints[(int)deviceToStationBearing]);

                        // check if distance between station contour point and lpaux device point is less than 1km
                        if (deviceToContourPointDistance.InKm() <= 1)
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Gets the file code for DTT.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <returns>returns System.String.</returns>
        public static string GetFileCodeForDTT(int easting, int northing)
        {
            string fileCode = string.Empty;

            //// format easting >= SWCorner && easting < (SWCorner + (EastingDimensionInKM * 1000)) && northing >= SWCorner && northing < (SWCorner + (NorthingDimensionInKm * 1000)
            if (easting >= 200000 && easting < (200000 + (100 * 1000)) && northing >= 900000 && northing < (900000 + (100 * 1000)))
            {
                fileCode = "UK_01";
            }
            else if (easting >= 0 && easting < (0 + (500 * 1000)) && northing >= 600000 && northing < (600000 + (100 * 1000)))
            {
                fileCode = "UK_02";
            }
            else if (easting >= 0 && easting < (0 + (500 * 1000)) && northing >= 500000 && northing < (500000 + (100 * 1000)))
            {
                fileCode = "UK_03";
            }
            else if (easting >= 400000 && easting < (400000 + (300 * 1000)) && northing >= 300000 && northing < (300000 + (100 * 1000)))
            {
                fileCode = "UK_04";
            }
            else if (easting >= 400000 && easting < (400000 + (200 * 1000)) && northing >= 400000 && northing < (400000 + (100 * 1000)))
            {
                fileCode = "UK_05";
            }
            else if (easting >= 0 && easting < (0 + (400 * 1000)) && northing >= 400000 && northing < (400000 + (100 * 1000)))
            {
                fileCode = "UK_06";
            }
            else if (easting >= 200000 && easting < (200000 + (200 * 1000)) && northing >= 300000 && northing < (300000 + (100 * 1000)))
            {
                fileCode = "UK_07";
            }
            else if (easting >= 0 && easting < (0 + (100 * 1000)) && northing >= 0 && northing < (0 + (100 * 1000)))
            {
                fileCode = "UK_08";
            }
            else if (easting >= 100000 && easting < (100000 + (200 * 1000)) && northing >= 0 && northing < (0 + (300 * 1000)))
            {
                fileCode = "UK_09";
            }
            else if (easting >= 300000 && easting < (300000 + (200 * 1000)) && northing >= 100000 && northing < (100000 + (100 * 1000)))
            {
                fileCode = "UK_10";
            }
            else if (easting >= 300000 && easting < (300000 + (200 * 1000)) && northing >= 200000 && northing < (200000 + (100 * 1000)))
            {
                fileCode = "UK_11";
            }
            else if (easting >= 300000 && easting < (300000 + (200 * 1000)) && northing >= 0 && northing < (0 + (100 * 1000)))
            {
                fileCode = "UK_12";
            }
            else if (easting >= 500000 && easting < (500000 + (200 * 1000)) && northing >= 200000 && northing < (200000 + (100 * 1000)))
            {
                fileCode = "UK_13";
            }
            else if (easting >= 500000 && easting < (500000 + (200 * 1000)) && northing >= 100000 && northing < (100000 + (100 * 1000)))
            {
                fileCode = "UK_14";
            }
            else if (easting >= 500000 && easting < (500000 + (200 * 1000)) && northing >= 0 && northing < (0 + (100 * 1000)))
            {
                fileCode = "UK_15";
            }
            else if (easting >= 0 && easting < (0 + (200 * 1000)) && northing >= 900000 && northing < (900000 + (100 * 1000)))
            {
                fileCode = "UK_16";
            }
            else if (easting >= 0 && easting < (0 + (300 * 1000)) && northing >= 700000 && northing < (700000 + (200 * 1000)))
            {
                fileCode = "UK_17";
            }
            else if (easting >= 300000 && easting < (300000 + (200 * 1000)) && northing >= 700000 && northing < (700000 + (200 * 1000)))
            {
                fileCode = "UK_18";
            }
            else if (easting >= 400000 && easting < (400000 + (100 * 1000)) && northing >= 1000000 && northing < (1000000 + (300 * 1000)))
            {
                fileCode = "UK_19";
            }
            else if (easting >= 300000 && easting < (300000 + (100 * 1000)) && northing >= 900000 && northing < (900000 + (300 * 1000)))
            {
                fileCode = "UK_20";
            }

            return fileCode;
        }

        /// <summary>
        /// Gets Collection of polygons co-ordinates for a given region.
        /// </summary>
        /// <param name="polygonString">Polygons co-ordinates stored in the text format.</param>
        /// <returns>Collection of polygons co-ordinates.</returns>
        public static IEnumerable<List<Location>> ParseRegionPolygons(string polygonString)
        {
            if (string.IsNullOrWhiteSpace(polygonString))
            {
                throw new ArgumentException("polygonString");
            }

            string[] polygons = polygonString.Split(',');

            foreach (string polygon in polygons)
            {
                List<Location> locations = new List<Location>();
                string[] cordinates = polygon.Split(' ');

                for (int i = 0; i < cordinates.Length; i += 2)
                {
                    double latitude = double.Parse(cordinates[i]);
                    double longitude = double.Parse(cordinates[i + 1]);

                    Location location = new Location(latitude, longitude);
                    locations.Add(location);
                }

                yield return locations;
            }
        }

        /// <summary>
        /// Parse encoded location rectangle string data of a Region.
        /// </summary>
        /// <param name="locationRectangles">Encoded location rectangles string.</param>
        /// <returns>Collection of location rectangles.</returns>
        public static IEnumerable<LocationRect> ParseLocationRectangles(string locationRectangles)
        {
            if (string.IsNullOrWhiteSpace(locationRectangles))
            {
                throw new ArgumentException("locationRectangles");
            }

            string[] locRectList = locationRectangles.Split(',');
            List<LocationRect> locRectCollection = new List<LocationRect>();

            for (int i = 0; i < locRectList.Length; i++)
            {
                string[] locRect = locRectList[i].Split(' ');

                // read Min latitude.
                double south = double.Parse(locRect[0]);

                // read Min Longitude.
                double west = double.Parse(locRect[1]);

                // read Max Latitude.
                double north = double.Parse(locRect[2]);

                // read Max Longitude.
                double east = double.Parse(locRect[3]);

                LocationRect boundingBox = new LocationRect(north, west, south, east);
                locRectCollection.Add(boundingBox);
            }

            return locRectCollection;
        }

        /// <summary>
        /// Identifies and places the given candidate location to an appropriate sector collection.
        /// </summary>
        /// <param name="currentOSGLocation">Candidate location to be pushed to the sectors.</param>
        /// <param name="edgeOverlappings">Collection of edge overlapping to be considered.</param>
        /// <param name="wsdOSGLocation">WSD location.</param>
        /// <param name="topLeftEastingNorthing">Top left easting and northing rounded to 10 meters.</param>
        /// <param name="bottomRightEastingNorthing">Bottom right easting and northing rounded to 10 meters.</param>
        /// <param name="wsdCoverageRadius">WSD coverage radius.</param>
        /// <param name="candidateLocationSectors">Array of four different sectors.</param>
        /// <param name="isOverlapped">Boolean to identify overlapping exists or not.</param>
        private static void ClassifyCandidateLocationsToSectors(OSGLocation currentOSGLocation, List<OSGLocation> edgeOverlappings, OSGLocation wsdOSGLocation, OSGLocation topLeftEastingNorthing, OSGLocation bottomRightEastingNorthing, double wsdCoverageRadius, List<OSGLocation>[] candidateLocationSectors, out bool isOverlapped)
        {
            OSGLocation[] subCordinates = GetCandidateLocationSquare(currentOSGLocation.Easting, currentOSGLocation.Northing);
            OSGLocation wsdOSGLocationRoundTo10 = Conversion.RoundTo10(wsdOSGLocation);

            isOverlapped = IsOverlappingCandidateLocations(subCordinates, edgeOverlappings, wsdOSGLocation, topLeftEastingNorthing, bottomRightEastingNorthing, wsdCoverageRadius);

            if (isOverlapped)
            {
                foreach (OSGLocation candidateLocation in subCordinates)
                {
                    int sectorIndex = FindCandidateLocationSectorIndex(candidateLocation, wsdOSGLocationRoundTo10);
                    candidateLocationSectors[sectorIndex].Add(candidateLocation);
                }
            }
        }

        /// <summary>
        /// Generates grid points of a square for a given easting and northing.
        /// </summary>
        /// <param name="easting">Easting in meters.</param>
        /// <param name="northing">Northing in meters.</param>
        /// <returns>Array of grid points which forms a square.</returns>
        private static OSGLocation[] GetCandidateLocationSquare(double easting, double northing)
        {
            OSGLocation[] tile = new OSGLocation[4];

            tile[0] = new OSGLocation(easting, northing);
            tile[1] = new OSGLocation(easting + 10, northing);
            tile[2] = new OSGLocation(easting, northing + 10);
            tile[3] = new OSGLocation(easting + 10, northing + 10);

            return tile;
        }

        /// <summary>
        /// Determines candidate locations for the given WSD coverage area.
        /// </summary>
        /// <param name="candidateLocationTile">Collection of grid points which forms a square of resolution 10 meters.</param>
        /// <param name="edgeOverlappings">Collection of edge overlapping to be considered.</param>
        /// <param name="wsdOSGLocation">WSD location.</param>
        /// <param name="topLeftEastingNorthing">Top left easting and northing rounded to 10 meters.</param>
        /// <param name="bottomRightEastingNorthing">Bottom right easting and northing rounded to 10 meters.</param>
        /// <param name="wsdCoverageRadius">WSD coverage radius in meters.</param>
        /// <returns>Boolean to indicate there exist a overlapping or not.</returns>
        private static bool IsOverlappingCandidateLocations(OSGLocation[] candidateLocationTile, List<OSGLocation> edgeOverlappings, OSGLocation wsdOSGLocation, OSGLocation topLeftEastingNorthing, OSGLocation bottomRightEastingNorthing, double wsdCoverageRadius)
        {
            bool isOverlapped = false;
            double currentEasting = candidateLocationTile[0].Easting;
            double currentNorthing = candidateLocationTile[0].Northing;

            for (int i = 0; i < candidateLocationTile.Length; i++)
            {
                // Pythagorean theorem to calculate the distance b/w the co-ordinates.
                double distance = Math.Sqrt(Math.Pow(Math.Abs(candidateLocationTile[i].Northing - wsdOSGLocation.OriginalNorthing), 2) + Math.Pow(Math.Abs(candidateLocationTile[i].Easting - wsdOSGLocation.OriginalEasting), 2));

                if (distance < wsdCoverageRadius)
                {
                    isOverlapped = true;
                    break;
                }
            }

            // Case to handle candidate pixels edge overlapping.
            if (!isOverlapped && (currentEasting == topLeftEastingNorthing.Easting || currentEasting == bottomRightEastingNorthing.Easting || currentNorthing == bottomRightEastingNorthing.Northing || currentNorthing == topLeftEastingNorthing.Northing))
            {
                isOverlapped = edgeOverlappings.Any((edge) =>
                {
                    double minEasting = candidateLocationTile.Min(osgLoc => osgLoc.Easting);
                    double maxEasting = candidateLocationTile.Max(osgLoc => osgLoc.Easting);
                    double minNorthing = candidateLocationTile.Min(osgLoc => osgLoc.Northing);
                    double maxNorthing = candidateLocationTile.Max(osgLoc => osgLoc.Northing);

                    return edge.OriginalEasting > minEasting && edge.OriginalEasting < maxEasting && edge.OriginalNorthing > minNorthing && edge.OriginalNorthing < maxNorthing;
                });
            }

            return isOverlapped;
        }

        /// <summary>
        /// Get intersection points a line having in a polygon
        /// </summary>
        /// <param name="contour">list of locations constructed as polygon</param>
        /// <param name="lineStart">start point of line</param>
        /// <param name="lineEnd">end point of line</param>
        /// <returns></returns>
        public static List<Location> GetIntersections(List<Location> contour, Location lineStart, Location lineEnd)
        {
            int i = 0; //// iterator along the polygon
            Location firstCoords; ////First point (needed for closing polygon path)
            Location lastCoords; ////Previously visited point
            List<Location> intersections = new List<Location>(); ////List to hold found intersections

            firstCoords = contour[0]; ////Getting the first coordinate pair
            lastCoords = firstCoords; ////Priming the previous coordinate pair
            i++;

            while (i < contour.Count)
            {
                Location currentCoords = contour[i];

                if (i != contour.Count - 1)
                {
                    Location intersection = GetIntersection(lastCoords.Latitude, lastCoords.Longitude, currentCoords.Latitude, currentCoords.Longitude, lineStart.Latitude, lineStart.Longitude, lineEnd.Latitude, lineEnd.Longitude);
                    if (intersection != null)
                    {
                        intersections.Add(intersection);
                    }

                    lastCoords = contour[i];
                    break;
                }
                else
                {
                    Location intersection = GetIntersection(currentCoords.Latitude, currentCoords.Longitude, firstCoords.Latitude, firstCoords.Longitude, lineStart.Latitude, lineStart.Longitude, lineEnd.Latitude, lineEnd.Longitude);
                    if (intersection != null)
                    {
                        intersections.Add(intersection);
                    }

                    break;
                }
            }

            return intersections;
        }

        /// <summary>
        /// Get intersection point between two lines
        /// </summary>
        /// <param name="Ax">start point latitude of one end of first line </param>
        /// <param name="Ay">start point longitude of one end of first line </param>
        /// <param name="Bx">end point latitude of one end of first line </param>
        /// <param name="By">end point longitude of one end of first line </param>
        /// <param name="Cx">start point latitude of one end of second line</param>
        /// <param name="Cy">start point longitude of one end of second line</param>
        /// <param name="Dx">end point latitude of one end of second line </param>
        /// <param name="Dy">end point longitude of one end of second line </param>
        /// <returns>intersection point</returns>
        public static Location GetIntersection(
            double Ax, double Ay,
            double Bx, double By,
            double Cx, double Cy,
            double Dx, double Dy)
        {
            double distAB, theCos, theSin, newX, ABpos;

            //  Fail if either line is undefined.
            if ((Ax == Bx && Ay == By) || (Cx == Dx && Cy == Dy))
            {
                return null;
            }

            //  (1) Translate the system so that point A is on the origin.
            Bx -= Ax; 
            By -= Ay;
            Cx -= Ax; 
            Cy -= Ay;
            Dx -= Ax; 
            Dy -= Ay;

            //  Discover the length of segment A-B.
            distAB = Math.Sqrt((Bx * Bx) + (By * By));

            //  (2) Rotate the system so that point B is on the positive X axis.
            theCos = Bx / distAB;
            theSin = By / distAB;
            newX = (Cx * theCos) + (Cy * theSin);
            Cy = (Cy * theCos) - (Cx * theSin);
            Cx = newX;
            newX = (Dx * theCos) + (Dy * theSin);
            Dy = (Dy * theCos) - (Dx * theSin);
            Dx = newX;

            //  Fail if the lines are parallel.
            if (Cy == Dy)
            {
                return null;
            }

            //  (3) Discover the position of the intersection point along line A-B.
            ABpos = Dx + (Cx - Dx) * Dy / (Dy - Cy);

            //  (4) Apply the discovered position to line A-B in the original coordinate system.
            double X = Ax + (ABpos * theCos);
            double Y = Ay + (ABpos * theSin);

            //  Success.
            return new Location(X, Y);
        }
    }
}
