// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    public class RegionPolygonsCache
    {
        public RegionPolygonsCache(string regionName, LocationRect locationRect, IEnumerable<List<Location>> polygons, IEnumerable<LocationRect> locationRectangles)
        {
            if (string.IsNullOrWhiteSpace(regionName))
            {
                throw new ArgumentException("regionName");
            }

            if (locationRect == null)
            {
                throw new ArgumentException("locationRect");
            }

            if (polygons == null)
            {
                throw new ArgumentException("polygons");
            }

            if (locationRectangles == null)
            {
                throw new ArgumentNullException("locationRectDictionary");
            }

            this.RegionName = regionName;
            this.BoundingBox = locationRect;
            this.PolygonCollection = new List<List<Location>>(polygons);
            this.LocationRectangles = new List<LocationRect>(locationRectangles);
        }

        public string RegionName { get; private set; }

        public LocationRect BoundingBox { get; private set; }

        public List<List<Location>> PolygonCollection { get; private set; }

        public List<LocationRect> LocationRectangles { get; private set; }

        public int PolygonsCount
        {
            get
            {
                return this.PolygonCollection == null ? 0 : this.PolygonCollection.Count();
            }
        }

        public DateTime LastModifiedTime { get; set; }

        /// <summary>
        /// Obtain Bounding box for the given PolygonCollection index.
        /// </summary>
        /// <param name="index">Index of the polygon</param>
        /// <returns>Bounding box.</returns>
        public LocationRect GetPolygonBoundingBox(int index)
        {
            return this.LocationRectangles.ElementAt(index);
        }

        /// <summary>
        /// Order PolygonCollection in terms of bounding box area.
        /// </summary>
        /// <param name="asc">Boolean value indicating order by ascending or descending.</param>
        public void OrderPolygonCollectionBy(bool asc = true)
        {
            for (int i = 0; i < this.LocationRectangles.Count - 1; i++)
            {
                int swapCount = 0;

                for (int j = 0; j < (this.LocationRectangles.Count - i - 1); j++)
                {
                    double currentLocRectArea = GetLocationRectArea(this.LocationRectangles[j]);
                    double nextLocRectArea = GetLocationRectArea(this.LocationRectangles[j + 1]);

                    if (asc && (currentLocRectArea > nextLocRectArea))
                    {
                        this.SwapIndexContents(j, j + 1);
                        swapCount++;
                    }
                    else if (!asc && (currentLocRectArea < nextLocRectArea))
                    {
                        this.SwapIndexContents(j, j + 1);
                        swapCount++;
                    }
                }

                if (swapCount == 0)
                {
                    break;
                }
            }
        }
        
        private static double GetLocationRectArea(LocationRect locationRect)
        {
            double width = Math.Abs(locationRect.East - locationRect.West);
            double height = Math.Abs(locationRect.North - locationRect.South);

            return width * height;
        }

        private void SwapIndexContents(int i, int j)
        {
            LocationRect tempLocRect = this.LocationRectangles[i];
            List<Location> tempPolygon = this.PolygonCollection[i];

            this.LocationRectangles[i] = this.LocationRectangles[j];
            this.LocationRectangles[j] = tempLocRect;

            this.PolygonCollection[i] = this.PolygonCollection[j];
            this.PolygonCollection[j] = tempPolygon;
        }
    }
}
