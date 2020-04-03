// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System.Linq;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents CDBSTVENGDATA
    /// </summary>
    public class CDBSTvEngData : CDBSTableEntity
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="CDBSTvEngData"/> class.
        /// </summary>
        public CDBSTvEngData()
        {
        }

        /////// <summary>
        /////// Gets or sets the ANT_INPUT_PWR
        /////// </summary>
        /////// <value>The ANT_INPUT_PWR.</value>
        ////public double AntInputPwr { get; set; }

        /////// <summary>
        /////// Gets or sets the ANT_MAX_PWR_GAIN
        /////// </summary>
        /////// <value>The ANT_MAX_PWR_GAIN.</value>
        ////public double AntMaxPwrGain { get; set; }

        /// <summary>
        /// Gets or sets the ANT_POLARIZATION
        /// </summary>
        /// <value>The ANT_POLARIZATION.</value>
        public string AntPolarization { get; set; }

        /// <summary>
        /// Gets or sets the ANTENNA_TYPE
        /// </summary>
        /// <value>The ANTENNA_TYPE.</value>
        public string AntennaType { get; set; }

        /// <summary>
        /// Gets or sets the ASRN_NA_IND
        /// </summary>
        /// <value>The ASRN_NA_IND.</value>
        public string AsrnNaInd { get; set; }

        /// <summary>
        /// Gets or sets the ASRN
        /// </summary>
        /// <value>The ASRN.</value>
        public int Asrn { get; set; }

        /////// <summary>
        /////// Gets or sets the AURAL_FREQ
        /////// </summary>
        /////// <value>The AURAL_FREQ.</value>
        ////public double AuralFreq { get; set; }

        /// <summary>
        /// Gets or sets the AVG_HORIZ_PWR_GAIN
        /// </summary>
        /// <value>The AVG_HORIZ_PWR_GAIN.</value>
        public double AvgHorizPwrGain { get; set; }

        /// <summary>
        /// Gets or sets the BIASED_LAT
        /// </summary>
        /// <value>The BIASED_LAT.</value>
        public double BiasedLat { get; set; }

        /// <summary>
        /// Gets or sets the BIASED_LONG
        /// </summary>
        /// <value>The BIASED_LONG.</value>
        public double BiasedLong { get; set; }

        /// <summary>
        /// Gets or sets the BORDER_CODE
        /// </summary>
        /// <value>The BORDER_CODE.</value>
        public string BorderCode { get; set; }

        /// <summary>
        /// Gets or sets the CARRIER_FREQ
        /// </summary>
        /// <value>The CARRIER_FREQ.</value>
        public double CarrierFreq { get; set; }

        /////// <summary>
        /////// Gets or sets the DOCKET_NUM
        /////// </summary>
        /////// <value>The DOCKET_NUM.</value>
        ////public string DocketNum { get; set; }

        /// <summary>
        /// Gets or sets the EFFECTIVE_ERP
        /// </summary>
        /// <value>The EFFECTIVE_ERP.</value>
        public double EffectiveErp { get; set; }

        /// <summary>
        /// Gets or sets the ELECTRICAL_DEG
        /// </summary>
        /// <value>The ELECTRICAL_DEG.</value>
        public double ElectricalDeg { get; set; }

        /// <summary>
        /// Gets or sets the ELEV_AMSL
        /// </summary>
        /// <value>The ELEV_AMSL.</value>
        public double ElevAmsl { get; set; }

        /// <summary>
        /// Gets or sets the ELEV_BLDG_AG
        /// </summary>
        /// <value>The ELEV_BLDG_AG.</value>
        public double ElevBldgAg { get; set; }

        /// <summary>
        /// Gets or sets the ENG_RECORD_TYPE
        /// </summary>
        /// <value>The ENG_RECORD_TYPE.</value>
        public string EngRecordType { get; set; }

        /// <summary>
        /// Gets or sets the FAC_ZONE
        /// </summary>
        /// <value>The FAC_ZONE.</value>
        public string FacZone { get; set; }

        /// <summary>
        /// Gets or sets the FREQ_OFFSET
        /// </summary>
        /// <value>The FREQ_OFFSET.</value>
        public string FreqOffset { get; set; }

        /// <summary>
        /// Gets or sets the GAIN_AREA
        /// </summary>
        /// <value>The GAIN_AREA.</value>
        public double GainArea { get; set; }

        /// <summary>
        /// Gets or sets the HAAT_RC_MTR
        /// </summary>
        /// <value>The HAAT_RC_MTR.</value>
        public double HaatRcMtr { get; set; }

        /// <summary>
        /// Gets or sets the HAG_OVERALL_MTR
        /// </summary>
        /// <value>The HAG_OVERALL_MTR.</value>
        public double HagOverallMtr { get; set; }

        /// <summary>
        /// Gets or sets the HAG_RC_MTR
        /// </summary>
        /// <value>The HAG_RC_MTR.</value>
        public double HagRcMtr { get; set; }

        /// <summary>
        /// Gets or sets the HORIZ_BT_ERP
        /// </summary>
        /// <value>The HORIZ_BT_ERP.</value>
        public double HorizBtErp { get; set; }

        /// <summary>
        /// Gets or sets the LAT_DEG
        /// </summary>
        /// <value>The LAT_DEG.</value>
        public int LatDeg { get; set; }

        /// <summary>
        /// Gets or sets the LAT_DIR
        /// </summary>
        /// <value>The LAT_DIR.</value>
        public string LatDir { get; set; }

        /// <summary>
        /// Gets or sets the LAT_MIN
        /// </summary>
        /// <value>The LAT_MIN.</value>
        public int LatMin { get; set; }

        /// <summary>
        /// Gets or sets the LAT_SEC
        /// </summary>
        /// <value>The LAT_SEC.</value>
        public double LatSec { get; set; }

        /// <summary>
        /// Gets or sets the LON_DEG
        /// </summary>
        /// <value>The LON_DEG.</value>
        public int LonDeg { get; set; }

        /// <summary>
        /// Gets or sets the LON_DIR
        /// </summary>
        /// <value>The LON_DIR.</value>
        public string LonDir { get; set; }

        /// <summary>
        /// Gets or sets the LON_MIN
        /// </summary>
        /// <value>The LON_MIN.</value>
        public int LonMin { get; set; }

        /// <summary>
        /// Gets or sets the LON_SEC
        /// </summary>
        /// <value>The LON_SEC.</value>
        public double LonSec { get; set; }

        /// <summary>
        /// Gets or sets the LOSS_AREA
        /// </summary>
        /// <value>The LOSS_AREA.</value>
        public double LossArea { get; set; }

        /// <summary>
        /// Gets or sets the MAX_ANT_PWR_GAIN
        /// </summary>
        /// <value>The MAX_ANT_PWR_GAIN.</value>
        public double MaxAntPwrGain { get; set; }

        /// <summary>
        /// Gets or sets the MAX_ERP_DBK
        /// </summary>
        /// <value>The MAX_ERP_DBK.</value>
        public double MaxErpDbk { get; set; }

        /// <summary>
        /// Gets or sets the MAX_ERP_KW
        /// </summary>
        /// <value>The MAX_ERP_KW.</value>
        public double MaxErpKw { get; set; }

        /// <summary>
        /// Gets or sets the MAX_HAAT
        /// </summary>
        /// <value>The MAX_HAAT.</value>
        public double MaxHaat { get; set; }

        /// <summary>
        /// Gets or sets the MECHANICAL_DEG
        /// </summary>
        /// <value>The MECHANICAL_DEG.</value>
        public double MechanicalDeg { get; set; }

        /// <summary>
        /// Gets or sets the MULTIPLEXOR_LOSS
        /// </summary>
        /// <value>The MULTIPLEXOR_LOSS.</value>
        public double MultiplexorLoss { get; set; }

        /////// <summary>
        /////// Gets or sets the POWER_OUTPUT_VIS_DBK
        /////// </summary>
        /////// <value>The POWER_OUTPUT_VIS_DBK.</value>
        ////public double PowerOutputVisDbk { get; set; }

        /// <summary>
        /// Gets or sets the POWER_OUTPUT_VIS_KW
        /// </summary>
        /// <value>The POWER_OUTPUT_VIS_KW.</value>
        public double PowerOutputVisKw { get; set; }

        /////// <summary>
        /////// Gets or sets the PREDICT_COVERAGE_AREA
        /////// </summary>
        /////// <value>The PREDICT_COVERAGE_AREA.</value>
        ////public double PredictCoverageArea { get; set; }

        /////// <summary>
        /////// Gets or sets the PREDICT_POP
        /////// </summary>
        /////// <value>The PREDICT_POP.</value>
        ////public int PredictPop { get; set; }

        /////// <summary>
        /////// Gets or sets the TERRAIN_DATA_SRC_OTHER
        /////// </summary>
        /////// <value>The TERRAIN_DATA_SRC_OTHER.</value>
        ////public string TerrainDataSrcOther { get; set; }

        /////// <summary>
        /////// Gets or sets the TERRAIN_DATA_SRC
        /////// </summary>
        /////// <value>The TERRAIN_DATA_SRC.</value>
        ////public string TerrainDataSrc { get; set; }

        /// <summary>
        /// Gets or sets the TILT_TOWARDS_AZIMUTH
        /// </summary>
        /// <value>The TILT_TOWARDS_AZIMUTH.</value>
        public double TiltTowardsAzimuth { get; set; }

        /// <summary>
        /// Gets or sets the TRUE_DEG
        /// </summary>
        /// <value>The TRUE_DEG.</value>
        public double TrueDeg { get; set; }

        /// <summary>
        /// Gets or sets the TV_DOM_STATUS
        /// </summary>
        /// <value>The TV_DOM_STATUS.</value>
        public string TvDomStatus { get; set; }

        /// <summary>
        /// Gets or sets the UPPERBAND_FREQ
        /// </summary>
        /// <value>The UPPERBAND_FREQ.</value>
        public double UpperbandFreq { get; set; }

        /// <summary>
        /// Gets or sets the VERT_BT_ERP
        /// </summary>
        /// <value>The VERT_BT_ERP.</value>
        public double VertBtErp { get; set; }

        /// <summary>
        /// Gets or sets the VISUAL_FREQ
        /// </summary>
        /// <value>The VISUAL_FREQ.</value>
        public double VisualFreq { get; set; }

        /// <summary>
        /// Gets or sets the VSD_SERVICE
        /// </summary>
        /// <value>The VSD_SERVICE.</value>
        public string VsdService { get; set; }

        /// <summary>
        /// Gets or sets the RCAMSL_HORIZ_MTR
        /// </summary>
        /// <value>The RCAMSL_HORIZ_MTR.</value>
        public double RcamslHorizMtr { get; set; }

        /// <summary>
        /// Gets or sets the ANT_ROTATION
        /// </summary>
        /// <value>The ANT_ROTATION.</value>
        public double AntRotation { get; set; }

        /////// <summary>
        /////// Gets or sets the INPUT_TRANS_LINE
        /////// </summary>
        /////// <value>The INPUT_TRANS_LINE.</value>
        ////public double InputTransLine { get; set; }

        /// <summary>
        /// Gets or sets the MAX_ERP_TO_HOR
        /// </summary>
        /// <value>The MAX_ERP_TO_HOR.</value>
        public double MaxErpToHor { get; set; }

        /// <summary>
        /// Gets or sets the TRANS_LINE_LOSS
        /// </summary>
        /// <value>The TRANS_LINE_LOSS.</value>
        public double TransLineLoss { get; set; }

        /////// <summary>
        /////// Gets or sets the LOTTERY_GROUP
        /////// </summary>
        /////// <value>The LOTTERY_GROUP.</value>
        ////public int LotteryGroup { get; set; }

        /// <summary>
        /// Gets or sets the ANALOG_CHANNEL
        /// </summary>
        /// <value>The ANALOG_CHANNEL.</value>
        public int AnalogChannel { get; set; }

        /////// <summary>
        /////// Gets or sets the LAT_WHOLE_SECS
        /////// </summary>
        /////// <value>The LAT_WHOLE_SECS.</value>
        ////public int LatWholeSecs { get; set; }

        /////// <summary>
        /////// Gets or sets the LON_WHOLE_SECS
        /////// </summary>
        /////// <value>The LON_WHOLE_SECS.</value>
        ////public int LonWholeSecs { get; set; }

        /// <summary>
        /// Gets or sets the MAX_ERP_ANY_ANGLE
        /// </summary>
        /// <value>The MAX_ERP_ANY_ANGLE.</value>
        public double MaxErpAnyAngle { get; set; }

        /// <summary>
        /// Gets or sets the STATION_CHANNEL
        /// </summary>
        /// <value>The STATION_CHANNEL.</value>
        public int StationChannel { get; set; }

        /////// <summary>
        /////// Gets or sets the LIC_ANT_MAKE
        /////// </summary>
        /////// <value>The LIC_ANT_MAKE.</value>
        ////public string LicAntMake { get; set; }

        /////// <summary>
        /////// Gets or sets the LIC_ANT_MODEL_NUM
        /////// </summary>
        /////// <value>The LIC_ANT_MODEL_NUM.</value>
        ////public string LicAntModelNum { get; set; }

        /////// <summary>
        /////// Gets or sets the DT_EMISSION_MASK
        /////// </summary>
        /////// <value>The DT_EMISSION_MASK.</value>
        ////public string DtEmissionMask { get; set; }

        /// <summary>
        /// Gets or sets the SITE_NUMBER
        /// </summary>
        /// <value>The SITE_NUMBER.</value>
        public int SiteNumber { get; set; }

        /////// <summary>
        /////// Gets or sets the ELEVATION_ANTENNA_ID
        /////// </summary>
        /////// <value>The ELEVATION_ANTENNA_ID.</value>
        ////public int ElevationAntennaId { get; set; }

        /////// <summary>
        /////// Gets or sets the LAST_CHANGE_DATE
        /////// </summary>
        /////// <value>The LAST_CHANGE_DATE.</value>
        ////public string LastChangeDate { get; set; }

        /// <summary>
        /// Gets or sets the channel.
        /// </summary>
        /// <value>The channel.</value>
        public int Channel { get; set; }

        /// <summary>
        /// Gets or sets the latitude.
        /// </summary>
        /// <value>The latitude.</value>
        public double Latitude { get; set; }

        /// <summary>
        /// Gets or sets the longitude.
        /// </summary>
        /// <value>The longitude.</value>
        public double Longitude { get; set; }

        /// <summary>
        /// Gets or sets the height.
        /// </summary>
        /// <value>The height.</value>
        public double Height { get; set; }

        /// <summary>
        /// Gets or sets the Transmit Power.
        /// </summary>
        /// <value>The Transmit Power.</value>
        public double TxPower { get; set; }

        /// <summary>
        /// Gets or sets the contour.
        /// </summary>
        /// <value>The contour.</value>
        public string Contour { get; set; }

        /// <summary>
        /// Gets or sets the KEY TV.
        /// </summary>
        /// <value>The KEY TV.</value>
        public string KeyTv { get; set; }

        /// <summary>
        /// Gets or sets the application status.
        /// </summary>
        /// <value>The application status.</value>
        public string AppStatus { get; set; }

        /// <summary>
        /// Gets or sets the application service.
        /// </summary>
        /// <value>The application service.</value>
        public string AppService { get; set; }

        /// <summary>
        /// Gets or sets the type of the application.
        /// </summary>
        /// <value>The type of the application.</value>
        public string AppType { get; set; }

        /// <summary>Gets or sets the DTV_TYPE</summary>
        /// <value>The DTV_TYPE.</value>
        public string DtvType { get; set; }

        /// <summary>Gets or sets the CUTOFF_DATE</summary>
        /// <value>The CUTOFF_DATE.</value>
        public string CpExpDate { get; set; }

        /// <summary>
        /// Gets or sets the border.
        /// </summary>
        /// <value>The border.</value>
        public int Border { get; set; }

        /// <summary>
        /// Gets or sets the antenna pattern data.
        /// </summary>
        /// <value>The antenna pattern data.</value>
        public string AntennaPatternData { get; set; }

        /// <summary>
        /// Gets or sets the call banner key.
        /// </summary>
        /// <value>The call banner key.</value>
        public string CallBannerKey { get; set; }

        /// <summary>
        /// Gets or sets the state.
        /// </summary>
        /// <value>The state.</value>
        public string State { get; set; }

        /// <summary>
        /// Gets or sets the city.
        /// </summary>
        /// <value>The city.</value>
        public string City { get; set; }

        /// <summary>
        /// Gets or sets the type of the entity.
        /// </summary>
        /// <value>The type of the entity.</value>
        public string EntityType { get; set; }

        /// <summary>
        /// Gets or sets the facility country.
        /// </summary>
        /// <value>The facility country.</value>
        public string FacCountry { get; set; }

        /// <summary>
        /// Gets or sets the parent call sign.
        /// </summary>
        /// <value>The parent call sign.</value>
        public string ParentCallSign { get; set; }

        /// <summary>
        /// Gets or sets the parent facility identifier.
        /// </summary>
        /// <value>The parent facility identifier.</value>
        public int ParentFacilityId { get; set; }

        /// <summary>
        /// Gets or sets the parent latitude.
        /// </summary>
        /// <value>The parent latitude.</value>
        public double ParentLatitude { get; set; }

        /// <summary>
        /// Gets or sets the parent longitude.
        /// </summary>
        /// <value>The parent longitude.</value>
        public double ParentLongitude { get; set; }

        /// <summary>
        /// Gets or sets the key hole radius_ MTR.
        /// </summary>
        /// <value>The key hole radius_ MTR.</value>
        public double KeyHoleRadiusMtrs { get; set; }

        /// <summary>
        /// Gets or sets the azimuth to parent.
        /// </summary>
        /// <value>The azimuth to parent.</value>
        public double AzimuthToParent { get; set; }

        /// <summary>
        /// Gets or sets the parent key TV.
        /// </summary>
        /// <value>The parent key TV.</value>
        public string ParentKeyTv { get; set; }

        /// <summary>
        /// Gets or sets the facility status.
        /// </summary>
        /// <value>The facility status.</value>
        public string FacStatus { get; set; }

        /// <summary>
        /// Gets or sets the NAD27 latitude.
        /// </summary>
        /// <value>The NAD27 latitude.</value>
        public double Nad27Latitude { get; set; }

        /// <summary>
        /// Gets or sets the NAD27 longitude.
        /// </summary>
        /// <value>The NAD27 longitude.</value>
        public double Nad27Longitude { get; set; }

        /// <summary>
        /// Gets or sets the license expiration date.
        /// </summary>
        /// <value>The license expiration date.</value>
        public string LicExpirationDate { get; set; }

        /// <summary>
        /// Merges the facility data.
        /// </summary>
        /// <param name="objData">The object data.</param>
        public void MergeFacilityData(CDBSFacility objData)
        {
            if (objData != null)
            {
                this.CallSign = objData.FacCallsign;
                this.FacCountry = objData.FacCountry;
                this.FacStatus = objData.FacStatus;
                this.LicExpirationDate = objData.LicExpirationDate;
                this.ParentFacilityId = objData.AssocFacilityId;
            }
        }

        /// <summary>
        /// Merges the translator data.
        /// </summary>
        /// <param name="objData">The object data.</param>
        /// <param name="objFacRecord">The object facility record.</param>
        public void MergeTranslatorData(CDBSTvEngData objData, CDBSFacility objFacRecord)
        {
            this.ParentCallSign = objFacRecord.FacCallsign;
            this.ParentFacilityId = objData.FacilityId;
            this.ParentLatitude = objData.Latitude;
            this.ParentLongitude = objData.Longitude;
            this.ParentKeyTv = objData.KeyTv;
            this.Channel = objData.Channel;
        }

        /// <summary>
        /// Merges the application tracking data.
        /// </summary>
        /// <param name="objData">The object data.</param>
        public void MergeAppTrackingData(CDBSAppTracking objData)
        {
            if (objData != null)
            {
                this.AppStatus = objData.AppStatus;
                this.CpExpDate = objData.CpExpDate;
            }
        }

        /// <summary>
        /// Merges the application data.
        /// </summary>
        /// <param name="objData">The object data.</param>
        public void MergeApplicationData(CDBSApplication objData)
        {
            if (objData != null)
            {
                this.AppService = objData.AppService;
                this.AppType = objData.AppType;
                this.DtvType = objData.DtvType;
                this.State = objData.CommState;
                this.City = objData.CommCity;
            }
        }

        /// <summary>
        /// Merges the antenna pattern data.
        /// </summary>
        /// <param name="objData">The object data.</param>
        public void MergeAntennaPatternsData(CDBSAntennaPattern objData)
        {
            if (objData != null)
            {
                this.AntennaPatternData = objData.Patterns;
            }
        }
    }
}
