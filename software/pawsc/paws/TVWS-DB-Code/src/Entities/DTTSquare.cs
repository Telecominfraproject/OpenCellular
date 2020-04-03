// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    ///     Represents Class DTTSquare.
    /// </summary>
    public class DTTSquare
    {
        /// <summary>The number of EASTING</summary>
        public const int NumberOfEastings = 2000;

        /// <summary>The number of NORTHING</summary>
        public const int NumberOfNorthings = 2000;

        /// <summary>The number of channels</summary>
        public const int NumberOfChannels = 40;

        /// <summary>The empty DTT positions</summary>
        private List<string> emptyDTTPositions = new List<string>();

        /// <summary>
        ///     Initializes a new instance of the <see cref="DTTSquare" /> class.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        public DTTSquare(int easting, int northing)
        {
            this.Easting = easting;
            this.Northing = northing;
            this.DttValues = new int[((NumberOfEastings / 100) * (NumberOfNorthings / 100)) * (NumberOfChannels + 2)];

            for (int i = easting; i < (easting + NumberOfEastings); i += 100)
            {
                for (int j = northing; j < (northing + NumberOfNorthings); j += 100)
                {
                    this.emptyDTTPositions.Add(FormatEastingNorthingName(i, j));
                }
            }
        }

        /// <summary>
        ///     Initializes a new instance of the <see cref="DTTSquare" /> class.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <param name="dttValues">The DTT.</param>
        public DTTSquare(int easting, int northing, int[] dttValues)
        {
            this.Easting = easting;
            this.Northing = northing;
            this.DttValues = dttValues;
        }

        /// <summary>
        ///     Gets or sets the easting.
        /// </summary>
        /// <value>The easting.</value>
        public int Easting { get; set; }

        /// <summary>
        ///     Gets or sets the northing.
        /// </summary>
        /// <value>The northing.</value>
        public int Northing { get; set; }

        /// <summary>
        ///     Gets or sets the DTT.
        /// </summary>
        /// <value>The DTT.</value>
        public int[] DttValues { get; set; }

        /// <summary>
        ///     Formats the name of the easting northing.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <returns>returns System.String.</returns>
        public static string FormatEastingNorthingName(int easting, int northing)
        {
            return string.Format("{0}-{1}", easting, northing);
        }

        /// <summary>
        /// Projects to DTT square easting.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <returns>returns EASTING.</returns>
        public static int ProjectToDTTSquareEasting(int easting)
        {
            return (easting / NumberOfEastings) * NumberOfEastings;
        }

        /// <summary>
        /// Projects to DTT square northing.
        /// </summary>
        /// <param name="northing">The northing.</param>
        /// <returns>returns NORTHING.</returns>
        public static int ProjectToDTTSquareNorthing(int northing)
        {
            return (northing / NumberOfNorthings) * NumberOfNorthings;
        }

        /// <summary>
        ///     All the DTT fields set.
        /// </summary>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool AllDTTFieldsSet()
        {
            return this.emptyDTTPositions.Count == 0;
        }

        /// <summary>
        ///     Sets the DTT.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <param name="dtt">The DTT.</param>
        public void SetDTT(int easting, int northing, int[] dtt)
        {
            if (!this.emptyDTTPositions.Remove(FormatEastingNorthingName(easting, northing)))
            {
                throw new Exception(string.Format("{0} cannot be set in {1} {2} DTTSquare -- either it was already set or is an invalid position", FormatEastingNorthingName(easting, northing), this.Easting, this.Northing));
            }

            int pos = this.GetPosition(easting, northing);

            dtt.CopyTo(this.DttValues, pos);
        }

        /// <summary>
        ///     Gets the DTT.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <returns>returns array.</returns>
        public int[] GetDTT(int easting, int northing)
        {
            int[] dtt = new int[NumberOfChannels + 2];
            int pos = this.GetPosition(easting, northing);

            Array.Copy(this.DttValues, pos, dtt, 0, dtt.Length);

            return dtt;
        }

        /// <summary>
        ///     Gets the DTT.
        /// </summary>
        /// <returns>returns DTT data.</returns>
        public List<DttData> GetDttBatch()
        {
            List<DttData> values = new List<DttData>();
            for (int easting = this.Easting; easting < (this.Easting + NumberOfEastings); easting += 100)
            {
                for (int northing = this.Northing; northing < (this.Northing + NumberOfNorthings); northing += 100)
                {
                    DttData dttdata = new DttData();
                    dttdata.DataValues = new int[NumberOfChannels + 2];
                    dttdata.Easting = easting;
                    dttdata.Northing = northing;
                    int pos = this.GetPosition(easting, northing);
                    Array.Copy(this.DttValues, pos, dttdata.DataValues, 0, dttdata.DataValues.Length);
                    values.Add(dttdata);
                }
            }

            return values;
        }

        /// <summary>
        ///     Gets the position.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <returns>returns position.</returns>
        /// <exception cref="System.Exception">
        ///     Easting difference is outside the bounding box
        ///     or
        ///     Northing difference is outside the bounding box
        /// </exception>
        private int GetPosition(int easting, int northing)
        {
            int eastingDiff = (easting / 100) - (this.Easting / 100);
            int northingDiff = (northing / 100) - (this.Northing / 100);

            if (eastingDiff >= NumberOfEastings / 100)
            {
                throw new Exception("Easting difference is outside the bounding box");
            }

            if (northingDiff >= NumberOfNorthings / 100)
            {
                throw new Exception("Northing difference is outside the bounding box");
            }

            return ((eastingDiff % NumberOfEastings) * (NumberOfChannels + 2)) + (northingDiff * ((NumberOfChannels + 2) * (NumberOfNorthings / 100)));
        }
    }
}
