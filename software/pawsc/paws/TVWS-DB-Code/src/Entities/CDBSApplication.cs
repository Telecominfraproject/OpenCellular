// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    /// <summary>
    /// Represents Class CDBSApplication.
    /// </summary>
    public class CDBSApplication : CDBSTableEntity
    {
        /// <summary>Gets or sets the APP_ARN</summary>
        /// <value>The APP_ARN.</value>
        public string AppArn { get; set; }

        /// <summary>Gets or sets the APP_SERVICE</summary>
        /// <value>The APP_SERVICE.</value>
        public string AppService { get; set; }

        /////// <summary>
        /////// Gets or sets the FILE_PREFIX
        /////// </summary>
        /////// <value>The FILE_PREFIX.</value>
        ////public string FilePrefix { get; set; }

        /// <summary>Gets or sets the COMM_CITY</summary>
        /// <value>The COMM_CITY.</value>
        public string CommCity { get; set; }

        /// <summary>Gets or sets the COMM_STATE</summary>
        /// <value>The COMM_STATE.</value>
        public string CommState { get; set; }

        /// <summary>Gets or sets the FAC_FREQUENCY</summary>
        /// <value>The FAC_FREQUENCY.</value>
        public double FacFrequency { get; set; }

        /// <summary>Gets or sets the STATION_CHANNEL</summary>
        /// <value>The STATION_CHANNEL.</value>
        public int StationChannel { get; set; }

        /// <summary>Gets or sets the FAC_CALLSIGN</summary>
        /// <value>The FAC_CALLSIGN.</value>
        public string FacCallsign { get; set; }

        /// <summary>Gets or sets the GENERAL_APP_SERVICE</summary>
        /// <value>The GENERAL_APP_SERVICE.</value>
        public string GeneralAppService { get; set; }

        /// <summary>Gets or sets the APP_TYPE</summary>
        /// <value>The APP_TYPE.</value>
        public string AppType { get; set; }

        /////// <summary>
        /////// Gets or sets the PAPER_FILED_IND
        /////// </summary>
        /////// <value>The PAPER_FILED_IND.</value>
        ////public string PaperFiledInd { get; set; }

        /// <summary>Gets or sets the DTV_TYPE</summary>
        /// <value>The DTV_TYPE.</value>
        public string DtvType { get; set; }

        /// <summary>Gets or sets the FRN</summary>
        /// <value>The FRN.</value>
        public string Frn { get; set; }

        /////// <summary>
        /////// Gets or sets the SHORTFORM_APP_ARN
        /////// </summary>
        /////// <value>The SHORTFORM_APP_ARN.</value>
        ////public string ShortformAppArn { get; set; }

        /////// <summary>
        /////// Gets or sets the SHORTFORM_FILE_PREFIX
        /////// </summary>
        /////// <value>The SHORTFORM_FILE_PREFIX.</value>
        ////public string ShortformFilePrefix { get; set; }

        /////// <summary>
        /////// Gets or sets the CORRESP_IND
        /////// </summary>
        /////// <value>The CORRESP_IND.</value>
        ////public string CorrespInd { get; set; }

        /// <summary>Gets or sets the ASSOC_FACILITY_ID</summary>
        /// <value>The ASSOC_FACILITY_ID.</value>
        public int AssocFacilityId { get; set; }

        /// <summary>Gets or sets the NETWORK_AFFIL</summary>
        /// <value>The NETWORK_AFFIL.</value>
        public string NetworkAffil { get; set; }

        /// <summary>Gets or sets the SAT_TV_IND</summary>
        /// <value>The SAT_TV_IND.</value>
        public string SatTvInd { get; set; }

        /// <summary>Gets or sets the COMM_COUNTY</summary>
        /// <value>The COMM_COUNTY.</value>
        public string CommCounty { get; set; }

        /// <summary>Gets or sets the COMM_ZIP1</summary>
        /// <value>The COMM_ZIP1.</value>
        public string CommZip1 { get; set; }

        /// <summary>Gets or sets the COMM_ZIP2</summary>
        /// <value>The COMM_ZIP2.</value>
        public string CommZip2 { get; set; }

        /////// <summary>
        /////// Gets or sets the LAST_CHANGE_DATE
        /////// </summary>
        /////// <value>The LAST_CHANGE_DATE.</value>
        ////public string LastChangeDate { get; set; }
    }
}
