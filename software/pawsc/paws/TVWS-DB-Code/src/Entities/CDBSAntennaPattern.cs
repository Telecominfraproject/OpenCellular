// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    /// <summary>Represents Class CDBSAntennaPattern.</summary>
    public class CDBSAntennaPattern : CDBSTableEntity
    {
        /// <summary>Gets or sets the AZIMUTH</summary>
        /// <value>The AZIMUTH.</value>
        public double Azimuth { get; set; }

        /// <summary>Gets or sets the FIELD_VALUE</summary>
        /// <value>The FIELD_VALUE.</value>
        public double FieldValue { get; set; }

        /////// <summary>
        /////// Gets or sets the ADDITIONAL_AZ_NUM
        /////// </summary>
        /////// <value>The ADDITIONAL_AZ_NUM.</value>
        ////public int AdditionalAzNum { get; set; }

        /////// <summary>
        /////// Gets or sets the LAST_CHANGE_DATE
        /////// </summary>
        /////// <value>The LAST_CHANGE_DATE.</value>
        ////public string LastChangeDate { get; set; }

        /// <summary>Gets or sets the patterns.</summary>
        /// <value>The patterns.</value>
        public string Patterns { get; set; }
    }
}
