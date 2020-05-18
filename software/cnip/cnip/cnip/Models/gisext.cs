using System;

namespace cnip.Models
{
    public static class gisext
    {
        public enum UnitSystem
        {
            SI = 0,
            US = 1
        }
        public enum DistanceFormula
        {
            Haversine = 0,
            SphericalLawOfCosines = 1,
            SphericalEarthProjection = 2
        }

        #region internal: properties (read-only)    
        internal static double EarthRadiusKm { get { return _radiusEarthKM; } }
        internal static double EarthRadiusMiles { get { return _radiusEarthMiles; } }
        internal static double M2Km { get { return _m2Km; } }
        internal static double Deg2rad { get { return _toRad; } }
        internal static double Rad2deg { get { return _toDeg; } }
        #endregion
        #region private: const    
        private const double _radiusEarthMiles = 3959;
        private const double _radiusEarthKM = 6371;
        private const double _m2Km = 1.60934;
        private const double _toRad = Math.PI / 180;
        private const double _toDeg = 180 / Math.PI;
        #endregion
        #region Method 1: Haversine algo    
        /// <summary>    
        /// Distance between two geographic points on surface, km/miles    
        /// Haversine formula to calculate    
        /// great-circle (orthodromic) distance on Earth    
        /// High Accuracy, Medium speed    
        /// re: http://en.wikipedia.org/wiki/Haversine_formula    
        /// </summary>    
        /// <param name="Lat1">double: 1st point Latitude</param>    
        /// <param name="Lon1">double: 1st point Longitude</param>    
        /// <param name="Lat2">double: 2nd point Latitude</param>    
        /// <param name="Lon2">double: 2nd point Longitude</param>    
        /// <returns>double: distance, km/miles</returns>    
        private static double DistanceHaversine(
            Coords loc1,
            Coords loc2,
            UnitSystem unitSystem)
        {
            try
            {
                double _radLat1 = loc1.Y * _toRad;
                double _radLat2 = loc2.Y * _toRad;
                double _dLatHalf = (_radLat2 - _radLat1) / 2;
                double _dLonHalf = Math.PI * (loc2.Y - loc1.X) / 360;

                // intermediate result    
                double _a = Math.Sin(_dLatHalf);
                _a *= _a;

                // intermediate result    
                double _b = Math.Sin(_dLonHalf);
                _b *= _b * Math.Cos(_radLat1) * Math.Cos(_radLat2);

                // central angle, aka arc segment angular distance    
                double _centralAngle =
                    2 * Math.Atan2(
                        Math.Sqrt(_a + _b),
                        Math.Sqrt(1 - _a - _b)
                        );

                // great-circle (orthodromic) distance on Earth between 2 points    
                if (unitSystem == UnitSystem.SI)
                {
                    return _radiusEarthKM * _centralAngle * 1000;
                }
                else
                {
                    return _radiusEarthMiles * _centralAngle;
                }
            }
            catch { throw; }
        }
        #endregion
        #region Method 2: Spherical Law of Cosines    
        /// <summary>    
        /// Distance between two geographic points on surface, km/miles    
        /// Spherical Law of Cosines formula to calculate    
        /// great-circle (orthodromic) distance on Earth;    
        /// High Accuracy, Medium speed    
        /// re: http://en.wikipedia.org/wiki/Spherical_law_of_cosines    
        /// </summary>    
        /// <param name="Lat1">double: 1st point Latitude</param>    
        /// <param name="Lon1">double: 1st point Longitude</param>    
        /// <param name="Lat2">double: 2nd point Latitude</param>    
        /// <param name="Lon2">double: 2nd point Longitude</param>    
        /// <returns>double: distance, km/miles</returns>    
        private static double DistanceSLC(
            Coords loc1,
            Coords loc2,
            UnitSystem unitSystem)
        {
            try
            {
                double _radLat1 = loc1.Y * _toRad;
                double _radLat2 = loc2.Y * _toRad;
                double _radLon1 = loc1.X * _toRad;
                double _radLon2 = loc2.X * _toRad;

                // central angle, aka arc segment angular distance    
                double _centralAngle =
                    Math.Acos(Math.Sin(_radLat1) *
                    Math.Sin(_radLat2) +
                    Math.Cos(_radLat1) *
                    Math.Cos(_radLat2) *
                    Math.Cos(_radLon2 - _radLon1));

                // great-circle (orthodromic) distance on Earth between 2 points    
                if (unitSystem == UnitSystem.SI)
                {
                    return _radiusEarthKM * _centralAngle * 1000;
                }
                else
                {
                    return _radiusEarthMiles * _centralAngle;
                }
            }
            catch { throw; }
        }
        #endregion
        #region Method 3: Spherical Earth projection    
        /// <summary>    
        /// Distance between two geographic points on surface, km/miles    
        /// Spherical Earth projection to a plane formula (using Pythagorean Theorem)    
        /// to calculate great-circle (orthodromic) distance on Earth.    
        /// central angle =    
        /// Sqrt((_radLat2 - _radLat1)^2 + (Cos((_radLat1 + _radLat2)/2) * (Lon2 - Lon1))^2)    
        /// Medium Accuracy, Fast,    
        /// relative error less than 0.1% in search area smaller than 250 miles    
        /// re: http://en.wikipedia.org/wiki/Geographical_distance    
        /// </summary>    
        /// <param name="Lat1">double: 1st point Latitude</param>    
        /// <param name="Lon1">double: 1st point Longitude</param>    
        /// <param name="Lat2">double: 2nd point Latitude</param>    
        /// <param name="Lon2">double: 2nd point Longitude</param>    
        /// <returns>double: distance, km/miles</returns>    
        private static double DistanceSEP(Coords loc1,
                                         Coords loc2,
                                         UnitSystem unitSystem)
        {
            try
            {
                double _radLat1 = loc1.Y * _toRad;
                double _radLat2 = loc2.Y * _toRad;
                double _dLat = (_radLat2 - _radLat1);
                double _dLon = (loc2.X - loc1.X) * _toRad;

                double _a = (_dLon) * Math.Cos((_radLat1 + _radLat2) / 2);

                // central angle, aka arc segment angular distance    
                double _centralAngle = Math.Sqrt(_a * _a + _dLat * _dLat);

                // great-circle (orthodromic) distance on Earth between 2 points    
                if (unitSystem == UnitSystem.SI)
                {
                    return _radiusEarthKM * _centralAngle * 1000;
                }
                else
                {
                    return _radiusEarthMiles * _centralAngle;
                }
            }
            catch { throw; }
        }
        #endregion
        public struct Coords
        {
            public double X { get; set; }
            public double Y { get; set; }
            public Coords(double x, double y)
            {
                X = x; Y = y;
            }
        }
        public struct Bounds
        {
            public double XMin { get; set; }
            public double YMin { get; set; }
            public double XMax { get; set; }
            public double YMax { get; set; }
            public Bounds(
                double xmin,
                double ymin,
                double xmax,
                double ymax)
            {
                XMin = xmin;
                YMin = ymin;
                XMax = xmax;
                YMax = ymax;
            }
        }
        public static Bounds MBR(
            Coords center, double radius)
        {
            Bounds mbr = new Bounds();
            Coords bottomLeft, topRight;
            bottomLeft =
                Destination(center, 225,
                    Math.Sqrt(2 * Math.Pow(radius, 2)));
            topRight =
                Destination(center, 45,
                    Math.Sqrt(2 * Math.Pow(radius, 2)));
            mbr.XMin = bottomLeft.X > topRight.X ?
                topRight.X : bottomLeft.X;
            mbr.XMax = bottomLeft.X > topRight.X ?
                bottomLeft.X : topRight.X;
            mbr.YMin = bottomLeft.Y > topRight.Y ?
                topRight.Y : bottomLeft.Y;
            mbr.YMax = bottomLeft.Y > topRight.Y ?
                bottomLeft.Y : topRight.Y;
            return mbr;
        }
        public static Coords Destination(
            Coords source, double bearing, double distance)
        {
            Coords destination = new Coords();
            source.X = source.X * _toRad;
            source.Y = source.Y * _toRad;
            distance = distance / 1000;
            bearing = bearing * _toRad;
            destination.Y = Math.Asin(
                    (
                        Math.Sin(source.Y) *
                        Math.Cos(distance / _radiusEarthKM)
                    ) +
                    (
                        Math.Cos(source.Y) *
                        Math.Sin(distance / _radiusEarthKM) *
                        Math.Cos(bearing)
                    )
                );
            destination.X = source.X + Math.Atan2(
                Math.Sin(bearing) *
                Math.Sin(distance / _radiusEarthKM) *
                Math.Cos(source.Y),
                Math.Cos(distance / _radiusEarthKM) -
                Math.Sin(source.Y) *
                Math.Sin(destination.Y)
                );
            destination.Y = destination.Y * _toDeg;
            destination.X = destination.X * _toDeg;
            return destination;
        }
        public static double Distance(
            Coords loc1,
            Coords loc2,
            UnitSystem unitSystem = UnitSystem.SI,
            DistanceFormula distanceFormula =
            DistanceFormula.SphericalEarthProjection)
        {
            switch (distanceFormula)
            {
                case DistanceFormula.Haversine:
                    return DistanceHaversine(loc1, loc2, unitSystem);
                case DistanceFormula.SphericalEarthProjection:
                    return DistanceSEP(loc1, loc2, unitSystem);
                case DistanceFormula.SphericalLawOfCosines:
                    return DistanceSLC(loc1, loc2, unitSystem);
                default:
                    return DistanceSEP(loc1, loc2, unitSystem);
            }
        }
    }
}