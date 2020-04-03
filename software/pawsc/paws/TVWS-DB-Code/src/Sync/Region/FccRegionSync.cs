// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Sync.Region
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.Configuration;
    using System.Data;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.IO.Compression;
    using System.Linq;
    using System.Net;
    using System.Net.Http;
    using System.Security.AccessControl;
    using System.Security.Cryptography;
    using System.Text;
    using System.Xml.Linq;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;
    using RegionCalculation;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents FCC RegionSync
    /// </summary>
    public class FccRegionSync : IRegionSync
    {
        /// <summary>The uhf band protected locations</summary>
        private List<DynamicTableEntity> uhfBandProtectedLocations;

        /// <summary>Initializes a new instance of the <see cref="FccRegionSync" /> class.</summary>
        public FccRegionSync()
        {
        }

        /// <summary>Gets or sets the DALC region synchronize.</summary>
        /// <value>The DALC region synchronize.</value>
        [Dependency]
        public IDalcRegionSync DalcRegionSync { get; set; }

        /// <summary>Gets or sets the sync logger.</summary>
        /// <value>The sync logger.</value>
        [Dependency]
        public ILogger Logger { get; set; }

        /// <summary>Gets or sets the sync auditor.</summary>
        /// <value>The sync auditor.</value>
        [Dependency]
        public IAuditor Auditor { get; set; }

        /// <summary>
        /// Gets or sets the common DALC.
        /// </summary>
        /// <value>The common DALC.</value>
        [Dependency]
        public IDalcCommon CommonDalc { get; set; }

        /// <summary>Gets or sets the synchronize incumbent.</summary>
        /// <value>The synchronize incumbent.</value>
        [Dependency]
        public IDalcIncumbent DalcIncumbent { get; set; }

        /// <summary>
        /// Gets or sets the terrain elevation.
        /// </summary>
        /// <value>The terrain elevation.</value>
        [Dependency]
        public ITerrainElevation TerrainElevation { get; set; }

        /// <summary>
        /// Gets or sets the region calculation.
        /// </summary>
        /// <value>The region calculation.</value>
        [Dependency]
        public IRegionCalculation RegionCalculation { get; set; }

        /// <summary>
        /// Gets or sets the net file stream.
        /// </summary>
        /// <value>The net file stream.</value>
        [Dependency]
        public INetFileStream NetFileStream { get; set; }

        #region Implementation of IRegionSync

        /// <summary>Performs Region Synchronization.</summary>
        public void SyncDB()
        {
            const string LogMethodName = "FccRegionSync.SyncDB";

            // Begin  Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Enter " + LogMethodName);

            try
            {
                if (Utils.Configuration["RunCDBSProcess"].ToInt32() == 1)
                {
                    this.ProcessCDBSUSMexicoTvEngData();
                    this.ProcessCDBSCanadianData();
                }

                if (Utils.Configuration["RunContourProcess"].ToInt32() == 1)
                {
                    this.CalculateContours();
                }

                if (Utils.Configuration["RunCDBSProcess"].ToInt32() == 1)
                {
                    this.ProcessCDBSTranslatorData();
                }

                if (Utils.Configuration["RunULSProcess"].ToInt32() == 1)
                {
                    this.ProcessULS();
                }

                if (Utils.Configuration["RunFCCDeviceAuthorizationProcess"].ToInt32() == 1)
                {
                    this.ProcessAuthorizedFCCDeviceData();
                }

                if (Utils.Configuration["RunCDBSProcess"].ToInt32() == 1)
                {
                    this.MergeData();
                }
            }
            catch (Exception ex)
            {
                // Log Exception
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerGenericMessage, string.Format("Error in {0} -> {1}", LogMethodName, ex.ToString()));
                this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Failure, 0, LogMethodName + ex.ToString());
            }

            // End Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "End " + LogMethodName);

            // Audit Success transaction
            this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Success, 0, LogMethodName);
        }

        #endregion

        /// <summary>
        /// Creates the facility table data.
        /// </summary>
        /// <param name="fileData">The file data.</param>
        /// <returns>returns SyncRow.</returns>
        public List<CDBSFacility> BuildFacilityData(Stream fileData)
        {
            List<CDBSFacility> rowsList = new List<CDBSFacility>();

            Action<string[]> processData = (dataArray) =>
            {
                CDBSFacility cdbsFacility = new CDBSFacility();

                if (string.IsNullOrEmpty(dataArray[5]))
                {
                    return;
                }

                cdbsFacility.CommCity = dataArray[0].Replace(',', ' ');
                cdbsFacility.CommState = dataArray[1].Replace(',', ' ');
                cdbsFacility.FacAddress1 = dataArray[3].Replace(',', ' ');
                cdbsFacility.FacAddress2 = dataArray[4].Replace(',', ' ');
                cdbsFacility.FacCallsign = dataArray[5];
                cdbsFacility.FacChannel = dataArray[6].ToInt32();
                cdbsFacility.FacCity = dataArray[7].Replace(',', ' ');
                cdbsFacility.FacCountry = dataArray[8].Replace(',', ' ');
                cdbsFacility.FacFrequency = dataArray[9].ToDouble();
                cdbsFacility.FacService = dataArray[10];
                cdbsFacility.FacState = dataArray[11];
                cdbsFacility.FacStatusDate = dataArray[12];
                cdbsFacility.FacType = dataArray[13];
                cdbsFacility.FacilityId = dataArray[14].ToInt32();
                cdbsFacility.LicExpirationDate = dataArray[15];
                cdbsFacility.FacStatus = dataArray[16];
                cdbsFacility.FacZip1 = dataArray[17];
                cdbsFacility.FacZip2 = dataArray[18];
                cdbsFacility.StationType = dataArray[19];
                cdbsFacility.AssocFacilityId = dataArray[20].ToInt32();
                cdbsFacility.CallsignEffDate = dataArray[21];
                cdbsFacility.TsidNtsc = dataArray[22].ToInt32();
                cdbsFacility.TsidDtv = dataArray[23].ToInt32();
                cdbsFacility.DigitalStatus = dataArray[24];
                cdbsFacility.SatTv = dataArray[25];
                cdbsFacility.NetworkAffil = dataArray[26];
                cdbsFacility.RowKey = cdbsFacility.FacilityId.ToString();
                cdbsFacility.PartitionKey = "CDBS";
                cdbsFacility.CallSign = cdbsFacility.FacCallsign;

                rowsList.Add(cdbsFacility);
            };

            string dataLine = string.Empty;
            using (TextReader txtRdr = new StreamReader(fileData))
            {
                while ((dataLine = txtRdr.ReadLine()) != null)
                {
                    processData(this.ParseDataSegment(dataLine));
                }
            }

            return rowsList;
        }

        /// <summary>
        /// Creates the application table data.
        /// </summary>
        /// <param name="fileData">The file data.</param>
        /// <returns>returns SyncRow.</returns>
        public List<CDBSApplication> BuildApplicationData(Stream fileData)
        {
            List<CDBSApplication> rowsList = new List<CDBSApplication>();

            Action<string[]> processData = (dataArray) =>
            {
                CDBSApplication cdbsApp = new CDBSApplication();

                cdbsApp.AppArn = dataArray[0];
                cdbsApp.AppService = dataArray[1];
                cdbsApp.ApplicationId = dataArray[2].ToInt32();
                cdbsApp.FacilityId = dataArray[3].ToInt32();
                cdbsApp.CommCity = dataArray[5];
                cdbsApp.CommState = dataArray[6];
                cdbsApp.FacFrequency = dataArray[7].ToDouble();
                cdbsApp.StationChannel = dataArray[8].ToInt32();
                cdbsApp.FacCallsign = dataArray[9];
                cdbsApp.CallSign = cdbsApp.FacCallsign;
                cdbsApp.GeneralAppService = dataArray[10];
                cdbsApp.AppType = dataArray[11];
                cdbsApp.DtvType = dataArray[13];
                cdbsApp.Frn = dataArray[14];
                cdbsApp.AssocFacilityId = dataArray[18].ToInt32();
                cdbsApp.NetworkAffil = dataArray[19];
                cdbsApp.SatTvInd = dataArray[20];
                cdbsApp.CommCounty = dataArray[21];
                cdbsApp.CommZip1 = dataArray[22];
                cdbsApp.CommZip2 = dataArray[23];
                cdbsApp.RowKey = string.Concat(cdbsApp.ApplicationId, ";", cdbsApp.FacilityId);
                cdbsApp.PartitionKey = "CDBS";

                rowsList.Add(cdbsApp);
            };

            string dataLine = string.Empty;
            using (TextReader txtRdr = new StreamReader(fileData))
            {
                while ((dataLine = txtRdr.ReadLine()) != null)
                {
                    processData(this.ParseDataSegment(dataLine));
                }
            }

            return rowsList;
        }

        /// <summary>
        /// Creates the antenna pattern table data.
        /// </summary>
        /// <param name="fileData">The file data.</param>
        /// <returns>returns SyncRow.</returns>
        public List<CDBSAntennaPattern> BuildAntennaPatternData(Stream fileData)
        {
            List<CDBSAntennaPattern> rowsList = new List<CDBSAntennaPattern>();

            string dataLine = string.Empty;

            Action<string[]> processData = (dataArray) =>
            {
                CDBSAntennaPattern cdbsAntennaPattern = new CDBSAntennaPattern();

                cdbsAntennaPattern.AntennaId = dataArray[0].ToInt32();
                cdbsAntennaPattern.Azimuth = dataArray[1].ToDouble();
                cdbsAntennaPattern.FieldValue = dataArray[2].ToDouble();
                cdbsAntennaPattern.IsNew = true;

                rowsList.Add(cdbsAntennaPattern);
            };

            using (TextReader txtRdr = new StreamReader(fileData))
            {
                while ((dataLine = txtRdr.ReadLine()) != null)
                {
                    processData(this.ParseDataSegment(dataLine));
                }
            }

            // group by antenna Id
            var grpdData = rowsList.GroupBy(obj => obj.AntennaId).ToList();
            rowsList.Clear();

            foreach (var grpRows in grpdData)
            {
                var row = grpRows.FirstOrDefault();

                double[] antennaFieldValues = new double[361];
                foreach (var grpRow in grpRows)
                {
                    antennaFieldValues[(int)grpRow.Azimuth] = grpRow.FieldValue;
                }

                row.Patterns = JsonSerialization.SerializeObject(antennaFieldValues);
                row.RowKey = row.AntennaId.ToString();
                row.PartitionKey = "CDBS";
                rowsList.Add(row);
            }

            return rowsList;
        }

        /// <summary>
        /// Creates the app tracking table data.
        /// </summary>
        /// <param name="fileData">The file data.</param>
        /// <returns>returns SyncRow.</returns>
        public List<CDBSAppTracking> BuildAppTrackingData(Stream fileData)
        {
            List<CDBSAppTracking> rowsList = new List<CDBSAppTracking>();

            Action<string[]> processData = (dataArray) =>
            {
                CDBSAppTracking cdbsAppTracking = new CDBSAppTracking();

                cdbsAppTracking.ApplicationId = dataArray[0].ToInt32();
                cdbsAppTracking.AppStatusDate = dataArray[1];
                cdbsAppTracking.CutoffDate = dataArray[2];
                cdbsAppTracking.CutoffType = dataArray[3];
                cdbsAppTracking.CpExpDate = dataArray[4];
                cdbsAppTracking.AppStatus = dataArray[5];
                cdbsAppTracking.DtvChecklist = dataArray[6];
                cdbsAppTracking.AcceptedDate = dataArray[8];
                cdbsAppTracking.TollingCode = dataArray[9];
                cdbsAppTracking.RowKey = cdbsAppTracking.ApplicationId.ToString();
                cdbsAppTracking.PartitionKey = "CDBS";

                rowsList.Add(cdbsAppTracking);
            };

            string dataLine = string.Empty;
            using (TextReader txtRdr = new StreamReader(fileData))
            {
                while ((dataLine = txtRdr.ReadLine()) != null)
                {
                    processData(this.ParseDataSegment(dataLine));
                }
            }

            return rowsList;
        }

        /// <summary>
        /// Builds if STA data.
        /// </summary>
        /// <param name="fileData">The file data.</param>
        /// <returns>returns STA list.</returns>
        public List<CDBSIfStaData> BuildIfStaData(Stream fileData)
        {
            List<CDBSIfStaData> rowsList = new List<CDBSIfStaData>();

            Action<string[]> processData = (dataArray) =>
            {
                CDBSIfStaData cdbsIfStaData = new CDBSIfStaData();

                if (string.IsNullOrEmpty(dataArray[3]))
                {
                    return;
                }

                cdbsIfStaData.ApplicationId = Convert.ToInt32(dataArray[0]);
                cdbsIfStaData.LicAntSysChange = dataArray[1];
                cdbsIfStaData.AntennaSystem = dataArray[2];
                cdbsIfStaData.RefAppArn = dataArray[3];
                cdbsIfStaData.RefFilePrefix = dataArray[4];
                cdbsIfStaData.CpAntSysFilePrefix = dataArray[5];
                cdbsIfStaData.CpAntSysAppArn = dataArray[6];
                rowsList.Add(cdbsIfStaData);
            };

            string dataLine = string.Empty;
            using (TextReader txtRdr = new StreamReader(fileData))
            {
                while ((dataLine = txtRdr.ReadLine()) != null)
                {
                    processData(this.ParseDataSegment(dataLine));
                }
            }

            return rowsList;
        }

        /// <summary>
        /// Creates the table data for TV_ENG_DATA.
        /// </summary>
        /// <param name="fileData">The file data.</param>
        /// <returns>returns SyncRow.</returns>
        public List<CDBSTvEngData> BuildTvEngData(Stream fileData)
        {
            List<CDBSTvEngData> rowsList = new List<CDBSTvEngData>();

            string dataLine = string.Empty;
            ////double rcamsl_horiz_mtr, haat_rc_mtr, hag_rc_mtr = 0;

            Action<string[]> processData = (dataArray) =>
            {
                CDBSTvEngData cdbsTvEngData = null;

                cdbsTvEngData = new CDBSTvEngData();
                cdbsTvEngData.AntPolarization = dataArray[2];
                cdbsTvEngData.AntennaId = dataArray[3].ToInt32();
                cdbsTvEngData.AntennaType = dataArray[4];
                cdbsTvEngData.ApplicationId = dataArray[5].ToInt32();
                cdbsTvEngData.AsrnNaInd = dataArray[6];
                cdbsTvEngData.Asrn = dataArray[7].ToInt32();
                cdbsTvEngData.AvgHorizPwrGain = dataArray[9].ToDouble();
                cdbsTvEngData.BiasedLat = dataArray[10].ToDouble();
                cdbsTvEngData.BiasedLong = dataArray[11].ToDouble();
                cdbsTvEngData.BorderCode = dataArray[12];
                cdbsTvEngData.CarrierFreq = dataArray[13].ToDouble();
                cdbsTvEngData.EffectiveErp = dataArray[15].ToDouble();
                cdbsTvEngData.ElectricalDeg = dataArray[16].ToDouble();
                cdbsTvEngData.ElevAmsl = dataArray[17].ToDouble();
                cdbsTvEngData.ElevBldgAg = dataArray[18].ToDouble();
                cdbsTvEngData.EngRecordType = dataArray[19];
                cdbsTvEngData.FacZone = dataArray[20];
                cdbsTvEngData.FacilityId = dataArray[21].ToInt32();
                cdbsTvEngData.FreqOffset = dataArray[22];
                cdbsTvEngData.GainArea = dataArray[23].ToDouble();
                cdbsTvEngData.HaatRcMtr = dataArray[24].ToDouble();
                cdbsTvEngData.HagOverallMtr = dataArray[25].ToDouble();
                cdbsTvEngData.HagRcMtr = dataArray[26].ToDouble();
                cdbsTvEngData.HorizBtErp = dataArray[27].ToDouble();
                cdbsTvEngData.LatDeg = dataArray[28].ToInt32();
                cdbsTvEngData.LatDir = dataArray[29];
                cdbsTvEngData.LatMin = dataArray[30].ToInt32();
                cdbsTvEngData.LatSec = dataArray[31].ToDouble();
                cdbsTvEngData.LonDeg = dataArray[32].ToInt32();
                cdbsTvEngData.LonDir = dataArray[33];
                cdbsTvEngData.LonMin = dataArray[34].ToInt32();
                cdbsTvEngData.LonSec = dataArray[35].ToDouble();
                cdbsTvEngData.LossArea = dataArray[36].ToDouble();
                cdbsTvEngData.MaxAntPwrGain = dataArray[37].ToDouble();
                cdbsTvEngData.MaxErpDbk = dataArray[38].ToDouble();
                cdbsTvEngData.MaxErpKw = dataArray[39].ToDouble();
                cdbsTvEngData.MaxHaat = dataArray[40].ToDouble();
                cdbsTvEngData.MechanicalDeg = dataArray[41].ToDouble();
                cdbsTvEngData.MultiplexorLoss = dataArray[42].ToDouble();
                cdbsTvEngData.PowerOutputVisKw = dataArray[44].ToDouble();
                cdbsTvEngData.TiltTowardsAzimuth = dataArray[49].ToDouble();
                cdbsTvEngData.TrueDeg = dataArray[50].ToDouble();
                cdbsTvEngData.TvDomStatus = dataArray[51];
                cdbsTvEngData.UpperbandFreq = dataArray[52].ToDouble();
                cdbsTvEngData.VertBtErp = dataArray[53].ToDouble();
                cdbsTvEngData.VisualFreq = dataArray[54].ToDouble();
                cdbsTvEngData.VsdService = dataArray[55];
                cdbsTvEngData.RcamslHorizMtr = dataArray[56].ToDouble();
                cdbsTvEngData.AntRotation = dataArray[57].ToDouble();
                cdbsTvEngData.MaxErpToHor = dataArray[59].ToDouble();
                cdbsTvEngData.TransLineLoss = dataArray[60].ToDouble();
                cdbsTvEngData.AnalogChannel = dataArray[62].ToInt32();
                cdbsTvEngData.MaxErpAnyAngle = dataArray[65].ToDouble();
                cdbsTvEngData.StationChannel = dataArray[66].ToInt32();
                cdbsTvEngData.SiteNumber = dataArray[70].ToInt32();
                cdbsTvEngData.Nad27Longitude = GeoCalculations.GetLongitudeInDegrees(cdbsTvEngData.LonDeg, cdbsTvEngData.LonMin, cdbsTvEngData.LonSec, cdbsTvEngData.LonDir);
                cdbsTvEngData.Nad27Latitude = GeoCalculations.GetLattitudeInDegrees(cdbsTvEngData.LatDeg, cdbsTvEngData.LatMin, cdbsTvEngData.LatSec, cdbsTvEngData.LatDir);

                double longitude = cdbsTvEngData.Nad27Longitude.ToString("F8").ToDouble();
                double latitude = cdbsTvEngData.Nad27Latitude.ToString("F8").ToDouble();

                var nad83Location = TransformCoordinate.ToNAD83(new Location(latitude, longitude));
                if (nad83Location != null)
                {
                    cdbsTvEngData.Latitude = nad83Location.Latitude.ToString("F13").ToDouble();
                    cdbsTvEngData.Longitude = nad83Location.Longitude.ToString("F13").ToDouble();
                }
                else
                {
                    cdbsTvEngData.Latitude = cdbsTvEngData.Nad27Latitude.ToString("F13").ToDouble();
                    cdbsTvEngData.Longitude = cdbsTvEngData.Nad27Longitude.ToString("F13").ToDouble();
                }

                cdbsTvEngData.TxPower = cdbsTvEngData.EffectiveErp;
                cdbsTvEngData.Channel = cdbsTvEngData.StationChannel;
                cdbsTvEngData.Height = cdbsTvEngData.RcamslHorizMtr;
                cdbsTvEngData.IsNew = true;
                cdbsTvEngData.PartitionKey = "CDBS";

                cdbsTvEngData.KeyTv = string.Concat(cdbsTvEngData.FacilityId, "-", cdbsTvEngData.VsdService);
                if (cdbsTvEngData.VsdService == "DD")
                {
                    cdbsTvEngData.KeyTv = string.Concat(cdbsTvEngData.FacilityId, "-", cdbsTvEngData.VsdService, "-", cdbsTvEngData.SiteNumber);
                }
                else if (cdbsTvEngData.VsdService == "LD")
                {
                    cdbsTvEngData.KeyTv = string.Concat(cdbsTvEngData.FacilityId, "-", cdbsTvEngData.VsdService, "-", cdbsTvEngData.SiteNumber, "-", cdbsTvEngData.StationChannel);
                }

                cdbsTvEngData.RowKey = cdbsTvEngData.KeyTv;

                rowsList.Add(cdbsTvEngData);
            };

            using (TextReader txtRdr = new StreamReader(fileData))
            {
                while ((dataLine = txtRdr.ReadLine()) != null)
                {
                    processData(this.ParseDataSegment(dataLine));
                }
            }

            return rowsList;
        }

        /// <summary>
        /// reads all data from ws_translator_input_channels.zip file and creates list CDBSTvEngData
        /// </summary>
        /// <param name="fileData">file stream</param>
        /// <returns>list CDBSTvEngData</returns>
        public List<TranslatorInput> BuildUpdatedTranslatorsData(Stream fileData)
        {
            List<TranslatorInput> rowsList = new List<TranslatorInput>();

            Action<string> processData = (record) =>
            {
                try
                {
                    string[] dataArray = this.ParseDataSegment(record);
                    TranslatorInput translatorInput = new TranslatorInput();
                    translatorInput.PrimaryCallSign = dataArray[4];
                    translatorInput.Whitespace_Id = dataArray[2];
                    translatorInput.PrimaryChannel = string.IsNullOrEmpty(dataArray[7]) ? 0 : Convert.ToInt32(dataArray[7]);
                    translatorInput.PrimaryFacilityId = dataArray[8].ToInt32();
                    translatorInput.ProgramOriginalCallsign = dataArray[11];
                    translatorInput.ProgramOriginalChannel = string.IsNullOrEmpty(dataArray[12]) ? 0 : Convert.ToInt32(dataArray[12]);
                    translatorInput.ProgramOriginalFacilityId = dataArray[13].ToInt32();
                    translatorInput.DelivaryMethod = dataArray[9];
                    rowsList.Add(translatorInput);
                }
                catch (Exception ex)
                {
                    this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerCdbsFileReadingGenericMessage, "Error while reading ws_translator_input_channel record: " + record + " , error : " + ex.ToString());

                    return;
                }
            };

            try
            {
                string dataLine = string.Empty;
                using (TextReader txtRdr = new StreamReader(fileData))
                {
                    while ((dataLine = txtRdr.ReadLine()) != null)
                    {
                        processData(dataLine);
                    }
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerCdbsFileReadingGenericMessage, "Error while reading ws_translator_input_channel file " + ex.ToString());
            }

            return rowsList;
        }

        /// <summary>
        /// reads all data from ws_translator_data.zip file and creates list of Translator data
        /// </summary>
        /// <param name="fileData">file stream</param>
        /// <returns>list of translator data</returns>
        public List<TranslatorData> BuildWSTranslatorData(Stream fileData)
        {
            List<TranslatorData> rowsList = new List<TranslatorData>();
            string dataLine = string.Empty;

            try
            {
                using (TextReader txtRdr = new StreamReader(fileData))
                {
                    while ((dataLine = txtRdr.ReadLine()) != null)
                    {
                        try
                        {
                            var dataArray = this.ParseDataSegment(dataLine);
                            TranslatorData translator = new TranslatorData();
                            translator.TranslatorCallsign = dataArray[4];
                            translator.Whitespaces_Id = dataArray[1];
                            translator.Facility_Id = dataArray[3].ToInt32();

                            rowsList.Add(translator);
                        }
                        catch (Exception ex)
                        {
                            this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerCdbsFileReadingGenericMessage, "Error while reading ws_translator_data record: " + dataLine + " , error : " + ex.ToString());
                            continue;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerCdbsFileReadingGenericMessage, "Error while reading ws_translator_data file " + ex.ToString());
            }

            return rowsList;
        }

        /// <summary>
        /// Calculates the contours.
        /// </summary>
        public void CalculateContours()
        {
            string logMethodName = "FccRegionSync.CalculateContours";

            Stopwatch stopwatch = new Stopwatch();
            stopwatch.Start();

            try
            {
                // Begin  Log Transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Contour Generation Start Time " + DateTime.UtcNow.ToString());

                var columnNames = new[]
                                  {
                                      "TxPower", "Latitude", "Longitude", "Height", "Channel", "AntennaId", "VsdService", "AntRotation", "AntennaType", "CallSign", "AntennaPatternData"
                                  };

                string tableName = Constants.CDBSUSMexicoTVEngDataTableName;

                var regionCalculation = Utils.Configuration.CurrentContainer.Resolve<IRegionCalculation>();

                var incumbents = this.DalcIncumbent.GetIncumbents(Utils.GetRegionalTableName(tableName), columnNames, new { IsNew = true });

                for (int i = 0; i < incumbents.Length; i++)
                {
                    try
                    {
                        // resresolving for a new Transaction Id in Logger
                        regionCalculation = Utils.Configuration.CurrentContainer.Resolve<IRegionCalculation>();
                        incumbents[i].BuildContourItems = true;
                        Contour contour = regionCalculation.CalculateContour(incumbents[i]);
                        contour.ContourPointItems = null;
                        SyncRow updateRow = new SyncRow();
                        updateRow.PartitionKey = "CDBS";
                        updateRow.RowKey = incumbents[i].RowKey;
                        updateRow.ETag = incumbents[i].ETag;
                        updateRow.Add("Contour", EntityProperty.GeneratePropertyForString(JsonSerialization.SerializeObject(contour)));
                        updateRow.Add("IsNew", EntityProperty.GeneratePropertyForBool(false));

                        this.DalcIncumbent.UpdateIncumbent(Utils.GetRegionalTableName(tableName), updateRow);

                    }
                    catch (Exception ex)
                    {
                        this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerGenericMessage, string.Format("Contour generation Error {0} - {1}", incumbents[i].CallSign, ex.ToString()));
                    }


                }

                //update portal contours and portal summary
                List<CDBSTvEngData> cdbsUSStations = this.CommonDalc.FetchEntity<CDBSTvEngData>(Utils.GetRegionalTableName(Constants.CDBSUSMexicoTVEngDataTableName), null);

                var portalContours = PortalContourHelper.GetContoursFromTVStations(cdbsUSStations, IncumbentType.TV_US);
                this.DalcRegionSync.UpdatePortalContoursAndSummary(portalContours, (int)IncumbentType.TV_US);

                stopwatch.Stop();
                // end log Transaction                

                this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, string.Format("Contour generation End Time {0}h ,{1}min ,{2}sec", stopwatch.Elapsed.Hours, stopwatch.Elapsed.Minutes, stopwatch.Elapsed.Seconds));
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "End Contour");

                this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Success, stopwatch.Elapsed.Milliseconds, logMethodName);
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Error -> " + ex.ToString());
                this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Failure, stopwatch.Elapsed.Milliseconds, logMethodName);
            }
        }

        /// <summary>
        /// Dumps the file.
        /// </summary>
        /// <typeparam name="T">type of records</typeparam>
        /// <param name="table">The table.</param>
        /// <param name="fileName">Name of the file.</param>
        /// <param name="separator">The separator.</param>
        public void DumpFile<T>(List<T> table, string fileName, string separator = ",")
        {
            StringBuilder headerBuilder = new StringBuilder();

            if (table.Count == 0)
            {
                return;
            }

            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(table[0]);

            foreach (PropertyDescriptor entityProperty in properties)
            {
                headerBuilder.AppendFormat("{0}{1}", entityProperty.Name, separator);
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
                        if (entityProperty.PropertyType == typeof(int[]))
                        {
                            itemValue = entityProperty.GetValue(item);
                            if (itemValue != null)
                            {
                                var itemArray = itemValue as int[];
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
                    writer.Write(separator);
                }

                rows.Add(writer.ToString());
            }

            File.WriteAllLines(Path.Combine(Utils.GetLocalStorePath(), fileName + ".csv"), rows);
        }

        /// <summary>
        /// Processes the authorized FCC device data.
        /// </summary>
        public void ProcessAuthorizedFCCDeviceData()
        {
            const string LogMethodName = "FccRegionSync.ProcessAuthorizedFCCDeviceData";

            // Begin  Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Enter " + LogMethodName);
            var status = 1;

            try
            {
                var fccUrl = Utils.Configuration["AuthorizeDeviceURL"];

                fccUrl = string.Concat(fccUrl, DateTime.Now.ToString("MM-dd-yyyy", CultureInfo.InvariantCulture));

                if (string.IsNullOrEmpty(fccUrl))
                {
                    return;
                }

                HttpClient easClient = new HttpClient();
                var response = easClient.GetStringAsync(new Uri(fccUrl)).Result;
                XDocument easXml = XDocument.Parse(response);

                List<AuthorizedDeviceRecord> authorizedDeviceRecords = new List<AuthorizedDeviceRecord>();
                foreach (var currentElement in easXml.Root.Elements())
                {
                    AuthorizedDeviceRecord deviceRecord = new AuthorizedDeviceRecord();
                    var innerNodes = currentElement.Descendants().ToList();
                    deviceRecord.ApplicationPurpose = innerNodes[0].Value;
                    deviceRecord.EquipmentClass = innerNodes[1].Value;
                    deviceRecord.FCCId = innerNodes[2].Value;
                    deviceRecord.Status = innerNodes[3].Value;
                    deviceRecord.StatusDate = innerNodes[4].Value.Replace("/", "-");

                    deviceRecord.RowKey = string.Concat(deviceRecord.FCCId, ";", deviceRecord.ApplicationPurpose, ";", deviceRecord.StatusDate, deviceRecord.EquipmentClass);
                    deviceRecord.PartitionKey = "FCC";
                    authorizedDeviceRecords.Add(deviceRecord);
                }

                status = this.DalcRegionSync.UpdateAuthorizedDeviceRecords(Constants.AuthorizedDeviceModelsTableName, authorizedDeviceRecords);
                this.DalcRegionSync.SetDatabaseSyncStatus("CDBS", Constants.AuthorizedDeviceModelsTableName, status == 1);
            }
            catch (Exception ex)
            {
                // Log Exception
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerGenericMessage, string.Format("Error in {0} -> {1}", LogMethodName, ex.ToString()));
                this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Failure, 0, LogMethodName + ex.ToString());
                this.DalcRegionSync.SetDatabaseSyncStatus("CDBS", Constants.AuthorizedDeviceModelsTableName, false);
            }

            // End Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "End " + LogMethodName);

            // Audit Success transaction
            this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Success, 0, LogMethodName);
        }

        /// <summary>Processes the file.</summary>
        public void ProcessCDBSUSMexicoTvEngData()
        {
            const string LogMethodName = "FccRegionSync.ProcessCDBSUSMexicoTvEngData";

            const string TableName = Constants.CDBSUSMexicoTVEngDataTableName;

            Stopwatch stopwatch = new Stopwatch();
            stopwatch.Start();

            // Begin  Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Enter " + LogMethodName);

            int status = 1;

            try
            {
                List<CDBSTvEngData> cdbsTvEngData_Tmp = null;
                List<CDBSFacility> facilityData = null;
                List<CDBSApplication> applicationData = null;
                List<CDBSAppTracking> appTrackingData = null;
                List<CDBSAntennaPattern> antennaPatternData = null;
                List<CDBSIfStaData> cdbsIfStaData = null;

                using (var fileStream = this.CreateCDBSRequest("tv_eng_data.zip"))
                {
                    ZipArchive zipFile = new ZipArchive(fileStream, ZipArchiveMode.Read);
                    cdbsTvEngData_Tmp = this.BuildTvEngData(zipFile.Entries[0].Open());
                }

                using (var fileStream = this.CreateCDBSRequest("facility.zip"))
                {
                    ZipArchive zipFile = new ZipArchive(fileStream, ZipArchiveMode.Read);
                    facilityData = this.BuildFacilityData(zipFile.Entries[0].Open());
                }

                using (var fileStream = this.CreateCDBSRequest("application.zip"))
                {
                    ZipArchive zipFile = new ZipArchive(fileStream, ZipArchiveMode.Read);
                    applicationData = this.BuildApplicationData(zipFile.Entries[0].Open());
                }

                using (var fileStream = this.CreateCDBSRequest("app_tracking.zip"))
                {
                    ZipArchive zipFile = new ZipArchive(fileStream, ZipArchiveMode.Read);
                    appTrackingData = this.BuildAppTrackingData(zipFile.Entries[0].Open());
                }

                using (var fileStream = this.CreateCDBSRequest("ant_pattern.zip"))
                {
                    ZipArchive zipFile = new ZipArchive(fileStream, ZipArchiveMode.Read);
                    antennaPatternData = this.BuildAntennaPatternData(zipFile.Entries[0].Open());
                }

                using (var fileStream = this.CreateCDBSRequest("if_sta.zip"))
                {
                    ZipArchive zipFile = new ZipArchive(fileStream, ZipArchiveMode.Read);
                    cdbsIfStaData = this.BuildIfStaData(zipFile.Entries[0].Open());
                }

                // as per CDBS Data Extraction logic
                var engDataLp1 = (from cdbsTableEntity in cdbsTvEngData_Tmp
                                  where cdbsTableEntity.EngRecordType == "C" && cdbsTableEntity.TvDomStatus == "LIC"
                                        && (cdbsTableEntity.VsdService == "DT" ||
                                            cdbsTableEntity.VsdService == "DC" ||
                                            cdbsTableEntity.VsdService == "CA" ||
                                            cdbsTableEntity.VsdService == "LD" ||
                                            cdbsTableEntity.VsdService == "DD" ||
                                            cdbsTableEntity.VsdService == "TX")
                                        && cdbsTableEntity.StationChannel >= 2 && cdbsTableEntity.StationChannel <= 51
                                        && cdbsTableEntity.KeyTv.IndexOf("-DD-0") == -1
                                  select cdbsTableEntity).ToList();

                var engDataLp2 = (from cdbsTableEntity in cdbsTvEngData_Tmp
                                  where cdbsTableEntity.EngRecordType == "P" && cdbsTableEntity.TvDomStatus == "APP"
                                        && (cdbsTableEntity.VsdService == "DT" ||
                                            cdbsTableEntity.VsdService == "DC" ||
                                            cdbsTableEntity.VsdService == "CA" ||
                                            cdbsTableEntity.VsdService == "LD" ||
                                            cdbsTableEntity.VsdService == "DD" ||
                                            cdbsTableEntity.VsdService == "TX")
                                        && cdbsTableEntity.StationChannel >= 2 && cdbsTableEntity.StationChannel <= 51
                                        && cdbsTableEntity.KeyTv.IndexOf("-DD-0") == -1
                                  select cdbsTableEntity).ToList();

                // Get the STA data
                var engDataSTA = (from cdbsTableEntity in cdbsTvEngData_Tmp
                                  join appRow in applicationData on cdbsTableEntity.ApplicationId equals appRow.ApplicationId
                                  join appTrackRow in appTrackingData on cdbsTableEntity.ApplicationId equals appTrackRow.ApplicationId
                                  join facrow in facilityData on cdbsTableEntity.FacilityId equals facrow.FacilityId into joinLP1Fac
                                  from joinFacrow in joinLP1Fac.DefaultIfEmpty()
                                  join antennaRow in antennaPatternData on cdbsTableEntity.AntennaId equals antennaRow.AntennaId into joinAntenna
                                  from joinAntennaRow in joinAntenna.DefaultIfEmpty()
                                  where cdbsTableEntity.FacilityId > 1
                                  && appRow.AppType == "STA"
                                 && (joinFacrow != null && !string.IsNullOrEmpty(joinFacrow.FacCallsign) && joinFacrow.FacCallsign.ToUpper().Trim().IndexOf("D") != 0 && (joinFacrow.FacStatus == "LICEN" || joinFacrow.FacStatus == "CPOFF"))
                                  && (!string.IsNullOrEmpty(appTrackRow.CpExpDate) && Convert.ToDateTime(appTrackRow.CpExpDate) >= DateTime.Now)
                                  select new { tv = cdbsTableEntity, facility = joinFacrow, appTrack = appTrackRow, application = appRow, antenna = joinAntennaRow }).ToList();

                // Get the STAX data
                var staxData = (from ifSta in cdbsIfStaData
                                join appTrackRow in appTrackingData on ifSta.ApplicationId equals appTrackRow.ApplicationId
                                join appRow in applicationData on appTrackRow.ApplicationId equals appRow.ApplicationId
                                join facrow in facilityData on appRow.FacilityId equals facrow.FacilityId
                                where appRow.AppType == "STAX"
                                && (appRow.AppService == "CA" ||
                                     appRow.AppService == "DC" ||
                                     appRow.AppService == "DD" ||
                                     appRow.AppService == "DS" ||
                                     appRow.AppService == "DT" ||
                                     appRow.AppService == "DX" ||
                                     appRow.AppService == "LD" ||
                                     appRow.AppService == "TX")
                                && (!string.IsNullOrEmpty(appTrackRow.CpExpDate))
                                && Convert.ToDateTime(appTrackRow.CpExpDate) >= DateTime.Now
                                select new { sta = ifSta, facility = facrow, appTrack = appTrackRow, application = appRow }).ToList();

                var query_staxData = (from ifStaRow in staxData
                                      join appRow in applicationData on ifStaRow.sta.RefAppArn equals appRow.AppArn
                                      join tvEngRow in cdbsTvEngData_Tmp on appRow.ApplicationId equals tvEngRow.ApplicationId
                                      join antennaRow in antennaPatternData on tvEngRow.AntennaId equals antennaRow.AntennaId into joinAntenna
                                      from joinAntennaRow in joinAntenna.DefaultIfEmpty()
                                      join facrow in facilityData on appRow.FacilityId equals facrow.FacilityId into joinLP1Fac
                                      from joinFacrow in joinLP1Fac.DefaultIfEmpty()
                                      where (joinFacrow != null && !joinFacrow.FacCallsign.StartsWith("D") && joinFacrow.FacStatus == "LICEN")
                                      select new { sta = ifStaRow.sta, facility = joinFacrow, appTrack = ifStaRow.appTrack, application = appRow, tv = tvEngRow, antenna = joinAntennaRow }).ToList();

                //// STAX records have precedence over STA records
                foreach (var staData in staxData)
                {
                    int index = engDataSTA.FindIndex(obj => obj.application.FacCallsign == staData.application.FacCallsign);
                    if (index > -1)
                    {
                        engDataSTA.RemoveAt(index);
                    }
                }

                // query lp1 records with full information 
                var query_lp1 = (from lprow in engDataLp1
                                 join facrow in facilityData on lprow.FacilityId equals facrow.FacilityId into joinLP1Fac
                                 from joinFacrow in joinLP1Fac.DefaultIfEmpty()
                                 join appTrackRow in appTrackingData on lprow.ApplicationId equals appTrackRow.ApplicationId into innerData0
                                 from joinAppTrackRow in innerData0.DefaultIfEmpty()
                                 join appRow in applicationData on joinAppTrackRow.ApplicationId equals appRow.ApplicationId into appJoin
                                 from appJoinRow in appJoin.DefaultIfEmpty()
                                 join antennaRow in antennaPatternData on lprow.AntennaId equals antennaRow.AntennaId into joinAntenna
                                 from joinAntennaRow in joinAntenna.DefaultIfEmpty()
                                 where joinFacrow != null && (!joinFacrow.FacCallsign.StartsWith("D") && (joinFacrow.FacStatus == "LICEN" || joinFacrow.FacStatus == "CPOFF") && joinFacrow.FacCountry == "US")
                                 select new { tv = lprow, facility = joinFacrow, appTrack = joinAppTrackRow, application = appJoinRow, antenna = joinAntennaRow }).ToList();

                // query lp2 records with full information
                var query_lp2 = (from lprow in engDataLp2
                                 join facrow in facilityData on lprow.FacilityId equals facrow.FacilityId into joinLP1Fac
                                 from joinFacrow in joinLP1Fac.DefaultIfEmpty()
                                 join appTrackRow in appTrackingData on lprow.ApplicationId equals appTrackRow.ApplicationId into innerData0
                                 from joinAppTrackRow in innerData0.DefaultIfEmpty()
                                 join appRow in applicationData on joinAppTrackRow.ApplicationId equals appRow.ApplicationId into appJoin
                                 from appJoinRow in appJoin.DefaultIfEmpty()
                                 join antennaRow in antennaPatternData on lprow.AntennaId equals antennaRow.AntennaId into joinAntenna
                                 from joinAntennaRow in joinAntenna.DefaultIfEmpty()
                                 where joinFacrow != null && (!joinFacrow.FacCallsign.StartsWith("D") && (joinFacrow.FacStatus == "LICEN" || joinFacrow.FacStatus == "CPOFF") && joinFacrow.FacCountry == "US")
                                 select new { tv = lprow, facility = joinFacrow, appTrack = joinAppTrackRow, application = appJoinRow, antenna = joinAntennaRow }).ToList();

                // Create list of US-TV-Stations from LP1 data
                List<CDBSTvEngData> cdbsUSStations = new List<CDBSTvEngData>();
                CDBSTvEngData cdbsTvEngDataLP1;
                foreach (var engDataRecord in query_lp1.GroupBy(obj => obj.tv.RowKey))
                {
                    var engRecord = engDataRecord.FirstOrDefault();
                    cdbsTvEngDataLP1 = engRecord.tv;
                    cdbsTvEngDataLP1.MergeFacilityData(engRecord.facility);
                    cdbsTvEngDataLP1.MergeApplicationData(engRecord.application);
                    if (cdbsTvEngDataLP1.AntennaId > 0 && engRecord.antenna == null)
                    {
                        cdbsTvEngDataLP1.AntennaId = 0;
                    }
                    else
                    {
                        cdbsTvEngDataLP1.MergeAntennaPatternsData(engRecord.antenna);
                    }

                    cdbsTvEngDataLP1.MergeAppTrackingData(engRecord.appTrack);
                    cdbsTvEngDataLP1.EntityType = Constants.EntityTypeUSStation;
                    cdbsUSStations.Add(cdbsTvEngDataLP1);
                }

                // Merge LP2 data to US-TV-Stations list
                int entity;
                CDBSTvEngData cdbsTvEngDataLP2;
                foreach (var engRecord in query_lp2)
                {
                    cdbsTvEngDataLP2 = engRecord.tv;
                    cdbsTvEngDataLP2.MergeFacilityData(engRecord.facility);
                    cdbsTvEngDataLP2.MergeApplicationData(engRecord.application);
                    if (cdbsTvEngDataLP2.AntennaId > 0 && engRecord.antenna == null)
                    {
                        cdbsTvEngDataLP2.AntennaId = 0;
                    }
                    else
                    {
                        cdbsTvEngDataLP2.MergeAntennaPatternsData(engRecord.antenna);
                    }

                    cdbsTvEngDataLP2.MergeAppTrackingData(engRecord.appTrack);
                    cdbsTvEngDataLP2.EntityType = Constants.EntityTypeUSStation;

                    // If a record containing EngRecordType = "C" And TvDomStatus = "LIC" is found then that record is updated using the EngRecordType = "P" And TvDomStatus = "APP" record
                    entity = cdbsUSStations.FindIndex(obj => obj.FacilityId == engRecord.tv.FacilityId && obj.VsdService == engRecord.tv.VsdService && obj.SiteNumber == engRecord.tv.SiteNumber);
                    if (entity >= 0)
                    {
                        cdbsUSStations.RemoveAt(entity);
                    }

                    cdbsUSStations.Add(cdbsTvEngDataLP2);
                }

                query_lp2.Clear();

                //// Merge STA and STAX data to US-TV-Stations list
                ////#region "Merge STA and STAX"

                CDBSTvEngData engEntity;
                CDBSTvEngData cdbsTvEngDataSTA;

                // Merge STA data
                foreach (var engRecordData in engDataSTA.GroupBy(obj => obj.tv.RowKey))
                {
                    var engRecord = engRecordData.FirstOrDefault();
                    cdbsTvEngDataSTA = engRecord.tv;
                    cdbsTvEngDataSTA.MergeFacilityData(engRecord.facility);
                    cdbsTvEngDataSTA.MergeApplicationData(engRecord.application);
                    if (cdbsTvEngDataSTA.AntennaId > 0 && engRecord.antenna == null)
                    {
                        cdbsTvEngDataSTA.AntennaId = 0;
                    }
                    else
                    {
                        cdbsTvEngDataSTA.MergeAntennaPatternsData(engRecord.antenna);
                    }

                    cdbsTvEngDataSTA.MergeAppTrackingData(engRecord.appTrack);
                    cdbsTvEngDataSTA.EntityType = Constants.EntityTypeUSStation;

                    // If a record is not found in TV eng data than add it
                    engEntity = cdbsUSStations.FirstOrDefault(obj => obj.KeyTv == engRecord.tv.KeyTv); // && obj.StationChannel == engRecord.application.StationChannel);
                    if (engEntity != null)
                    {
                        if (engEntity.StationChannel != engRecord.application.StationChannel)
                        {
                        }
                    }
                    else
                    {
                        cdbsUSStations.Add(cdbsTvEngDataSTA);
                    }
                }

                engDataSTA.Clear();

                // Merge STAX data
                foreach (var engRecordData in query_staxData.GroupBy(obj => obj.tv.RowKey))
                {
                    var engRecord = engRecordData.FirstOrDefault();
                    cdbsTvEngDataSTA = engRecord.tv;
                    cdbsTvEngDataSTA.MergeFacilityData(engRecord.facility);
                    cdbsTvEngDataSTA.MergeApplicationData(engRecord.application);
                    if (cdbsTvEngDataSTA.AntennaId > 0 && engRecord.antenna == null)
                    {
                        cdbsTvEngDataSTA.AntennaId = 0;
                    }
                    else
                    {
                        cdbsTvEngDataSTA.MergeAntennaPatternsData(engRecord.antenna);
                    }

                    cdbsTvEngDataSTA.MergeAppTrackingData(engRecord.appTrack);
                    cdbsTvEngDataSTA.EntityType = Constants.EntityTypeUSStation;

                    // If a record is not found in TV eng data than add it
                    engEntity = cdbsUSStations.FirstOrDefault(obj => obj.KeyTv == engRecord.tv.KeyTv); // && obj.StationChannel == engRecord.application.StationChannel);
                    if (engEntity != null)
                    {
                        if (engEntity.StationChannel != engRecord.application.StationChannel)
                        {
                            cdbsTvEngDataSTA.RowKey = cdbsTvEngDataSTA.RowKey + "-STAX-" + engRecord.application.StationChannel;
                            cdbsUSStations.Add(cdbsTvEngDataSTA);
                        }
                    }
                    else
                    {
                        cdbsUSStations.Add(cdbsTvEngDataSTA);
                    }
                }

                query_staxData.Clear();

                ////#endregion
                //// --end of Merge STA and STAX

                // Get Parent data for each Tv Station
                CDBSFacility parentFacility;
                CDBSTvEngData parentTvEngData;
                foreach (var currentTvStation in cdbsUSStations.FindAll(obj => obj.ParentFacilityId != 0))
                {
                    parentFacility = facilityData.FirstOrDefault(obj => obj.FacilityId == currentTvStation.ParentFacilityId);
                    if (parentFacility != null)
                    {
                        currentTvStation.ParentCallSign = parentFacility.FacCallsign;
                    }

                    parentTvEngData = cdbsUSStations.FirstOrDefault(obj => obj.FacilityId == currentTvStation.ParentFacilityId); // && obj.AntennaId == tvStation.AntennaId && obj.ApplicationId == tvStation.ApplicationId);
                    if (parentTvEngData != null)
                    {
                        currentTvStation.ParentLatitude = parentTvEngData.Latitude;
                        currentTvStation.ParentLongitude = parentTvEngData.Longitude;
                        currentTvStation.ParentKeyTv = parentTvEngData.KeyTv;
                    }
                }

                ////this.DumpFile(cdbsUSStations, "CDBSUsStations");

                var queryMexican = from cdbsRecord in cdbsTvEngData_Tmp
                                   join facrow in facilityData on cdbsRecord.FacilityId equals facrow.FacilityId
                                   join antennarow in antennaPatternData on cdbsRecord.AntennaId equals antennarow.AntennaId into joinAntennaData
                                   from antennaDataRow in joinAntennaData.DefaultIfEmpty()
                                   where facrow.FacCountry == "MX" && cdbsRecord.EngRecordType == "C" && cdbsRecord.FacilityId > 0 && cdbsRecord.TvDomStatus == "GRANT"
                                   && facrow.FacCallsign[0] == 'X' && cdbsRecord.StationChannel >= 2 && cdbsRecord.StationChannel <= 51
                                   select new { tv = cdbsRecord, facility = facrow, antennaRow = antennaDataRow };

                foreach (var finalRow in queryMexican.GroupBy(obj => obj.tv.RowKey))
                {
                    var firstRow = finalRow.FirstOrDefault();
                    var row = firstRow.tv;
                    row.MergeFacilityData(firstRow.facility);
                    if (row.AntennaId > 0 && firstRow.antennaRow == null)
                    {
                        row.AntennaId = 0;
                    }
                    else
                    {
                        row.MergeAntennaPatternsData(firstRow.antennaRow);
                    }

                    row.EntityType = Constants.EntityTypeMexicoStation;
                    cdbsUSStations.Add(row);
                }

                var incumbents = this.CommonDalc.FetchEntity<CDBSTvEngData>(Utils.GetRegionalTableName(TableName), null);

                var inActiveRecords = incumbents.Where(obj => cdbsUSStations.FirstOrDefault(obj2 => obj2.KeyTv == obj.KeyTv) == null).ToList();

                var changedRecords = (from existingRow in incumbents
                                      join filerow in cdbsUSStations on existingRow.KeyTv equals filerow.KeyTv
                                      where (filerow.TvDomStatus == existingRow.TvDomStatus || filerow.FacilityId == existingRow.FacilityId) && (existingRow.Height != filerow.Height || existingRow.Channel != filerow.Channel || existingRow.TxPower != filerow.TxPower || existingRow.ParentFacilityId != filerow.ParentFacilityId || (!string.IsNullOrEmpty(filerow.CallSign) && existingRow.CallSign != filerow.CallSign))
                                      select filerow).ToList();

                var newRecords = cdbsUSStations.Where(obj => incumbents.FirstOrDefault(obj2 => obj2.KeyTv == obj.KeyTv) == null).ToList();

                foreach (var cdbsrecord in newRecords.Where(obj => obj.Height == 0.0))
                {
                    var elevation = this.TerrainElevation.CalculateElevation(new Location(cdbsrecord.Latitude, cdbsrecord.Longitude));
                    if (cdbsrecord.HagRcMtr != 0.0)
                    {
                        cdbsrecord.Height = elevation.InMeter() + cdbsrecord.HagRcMtr;
                    }
                    else
                    {
                        var stationHaat = this.RegionCalculation.CalculateStationHAAT(new Location(cdbsrecord.Latitude, cdbsrecord.Longitude));
                        cdbsrecord.Height = (cdbsrecord.HaatRcMtr - stationHaat.AzimuthRadialHaats.Average(obj => obj.Value)) + elevation.InMeter();
                    }
                }

                if (inActiveRecords.Any())
                {
                    this.DalcRegionSync.DeleteCDBSData(TableName, inActiveRecords);
                }

                if (changedRecords.Any())
                {
                    status = this.DalcRegionSync.UpdateCDBSData(TableName, changedRecords);
                }

                if (newRecords.Any())
                {
                    status = this.DalcRegionSync.UpdateCDBSData(TableName, newRecords);
                }

                this.DalcRegionSync.SetDatabaseSyncStatus("CDBS", TableName, status == 1);
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Error " + ex.ToString());
                this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Failure, stopwatch.Elapsed.Milliseconds, LogMethodName);
                this.DalcRegionSync.SetDatabaseSyncStatus("CDBS", TableName, false);
            }

            // End Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Ends " + LogMethodName);
            this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Success, stopwatch.Elapsed.Milliseconds, LogMethodName);
        }

        /// <summary>
        /// Processes the CDBS translator data.
        /// </summary>
        public void ProcessCDBSTranslatorData()
        {
            const string LogMethodName = "FccRegionSync.ProcessCDBSTranslatorData";

            Stopwatch stopwatch = new Stopwatch();
            stopwatch.Start();

            // Begin  Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Enter " + LogMethodName);

            int status = 0;
            try
            {
                List<CDBSTvEngData> cdbsUSStations = this.CommonDalc.FetchEntity<CDBSTvEngData>(Utils.GetRegionalTableName(Constants.CDBSUSMexicoTVEngDataTableName), null);
                List<DynamicTableEntity> translatorWaivers = this.CommonDalc.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.TranslatorWaiverCallSignTableName), null);
                List<TranslatorData> translatorDataRecords = null;
                List<TranslatorInput> translatorInputs = null;

                // Get all updated tv translators
                using (var fileStream = this.CreateCDBSRequest("ws_translator_data.zip"))
                {
                    ZipArchive zipFile = new ZipArchive(fileStream, ZipArchiveMode.Read);
                    translatorDataRecords = this.BuildWSTranslatorData(zipFile.Entries[0].Open());
                }

                // Get all updated tv translators
                using (var fileStream = this.CreateCDBSRequest("ws_translator_input_channels.zip"))
                {
                    ZipArchive zipFile = new ZipArchive(fileStream, ZipArchiveMode.Read);
                    translatorInputs = this.BuildUpdatedTranslatorsData(zipFile.Entries[0].Open());
                }

                var query_ParentStations = this.GetParentStations(cdbsUSStations);
                var query_Translators = this.GetTranslators(cdbsUSStations);

                List<CDBSTvEngData> translatorRecords = new List<CDBSTvEngData>();
                foreach (var translator in query_Translators)
                {
                    //Check for updates
                    List<CDBSTvEngData> updatedParentTranslators = new List<CDBSTvEngData>();
                    List<KeyValuePair<string, int>> updatedTranslatorInputs = new List<KeyValuePair<string, int>>();

                    if (this.CheckTranslatorUpdates(translator, translatorDataRecords, translatorInputs, ref updatedTranslatorInputs))
                    {
                        updatedParentTranslators = this.GetUpdatedParentTranslators(updatedTranslatorInputs, query_ParentStations, translator);

                        if (updatedParentTranslators.Count == 0)
                        {
                            //// updates are available, but corresponding parent is not available
                            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, string.Format("Skipping translator {0} as there are not correspoding parent is available for updated translator input", translator.CallSign));
                            continue;
                        }
                    }
                    else
                    {
                        var matchingParentStations = query_ParentStations.Where(x => x.FacilityId == translator.ParentFacilityId).ToList();

                        if (matchingParentStations.Count > 0)
                        {                            
                            updatedParentTranslators.AddRange(matchingParentStations);
                        }
                        else
                        {
                            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, string.Format("Skipping translator {0} as there is no corresponding parent available", translator.CallSign));
                            continue;
                        }
                    }

                    foreach (var parent in updatedParentTranslators)
                    {
                        var translatorCopy = JsonConvert.DeserializeObject<CDBSTvEngData>(JsonConvert.SerializeObject(translator));

                        //Protect Input Channel
                        translatorCopy.Channel = parent.Channel;
                        translatorCopy.ParentCallSign = parent.CallSign;
                        translatorCopy.ParentFacilityId = parent.FacilityId;

                        translatorCopy.Contour = string.Empty;
                        translatorCopy.AntennaPatternData = string.Empty;

                        translatorCopy.ParentLatitude = parent.Latitude;
                        translatorCopy.ParentLongitude = parent.Longitude;
                        translatorCopy.ParentFacilityId = parent.FacilityId;

                        translatorCopy.AzimuthToParent = GeoCalculations.CalculateBearing(new Location(translatorCopy.Latitude, translatorCopy.Longitude), new Location(translatorCopy.ParentLatitude, translatorCopy.ParentLongitude));
                        translatorCopy.RowKey = string.Concat(translatorCopy.RowKey, ";", parent.RowKey);

                        //Add  translator to final list
                        if (translatorRecords.Where(x => x.RowKey == translatorCopy.RowKey).FirstOrDefault() == null)
                        {
                            if (ValidateTranslatorDistence(translatorCopy, parent, translatorWaivers))
                            {
                                translatorRecords.Add(translatorCopy);
                            }
                        }
                    }

                }

                var existingTranslatorData = this.CommonDalc.FetchEntity<CDBSTvEngData>(Utils.GetRegionalTableName(Constants.CDBSTranslatorDataTableName), null);
                this.CommonDalc.DeleteRecords(Constants.CDBSTranslatorDataTableName, existingTranslatorData);
                status = this.CommonDalc.UpdateTable(Constants.CDBSTranslatorDataTableName, translatorRecords);
                this.DalcRegionSync.SetDatabaseSyncStatus("CDBS", Constants.CDBSTranslatorDataTableName, status == 1);

                //update portal contours and portal summary  
                //var translatorRecords = this.CommonDalc.FetchEntity<CDBSTvEngData>(Utils.GetRegionalTableName(Constants.CDBSTranslatorDataTableName), null);
                var portalContours = PortalContourHelper.GetContoursFromTVStations(translatorRecords, IncumbentType.TV_TRANSLATOR);
                this.DalcRegionSync.UpdatePortalContoursAndSummary(portalContours, (int)IncumbentType.TV_TRANSLATOR);
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Error " + ex.ToString());
                this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Failure, stopwatch.Elapsed.Milliseconds, LogMethodName);
                this.DalcRegionSync.SetDatabaseSyncStatus("CDBS", Constants.CDBSTranslatorDataTableName, false);
            }
            // End Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Ends " + LogMethodName);
            this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Success, stopwatch.Elapsed.Milliseconds, LogMethodName);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="cdbsUSStations"></param>
        /// <returns></returns>
        private List<CDBSTvEngData> GetParentStations(List<CDBSTvEngData> cdbsUSStations)
        {
            var query_ParentStations = (from engRow in cdbsUSStations
                                        where engRow.FacStatus == "LICEN"
                                            && ((engRow.SiteNumber <= 1 && engRow.VsdService != "DD") || (engRow.SiteNumber > 0 && engRow.VsdService == "DD"))
                                        select engRow).ToList();


            //remove duplicate parent callsigns which have invalid channel numbers
            //invalid parent callsigns are those which are having vsd service value as "LD" 
            foreach (var group in query_ParentStations.GroupBy(x => x.CallSign))
            {
                if (group.Count() > 1)
                {
                    var ldElements = group.Where(x => x.VsdService == "LD").ToList();

                    if (ldElements.Count > 0)
                    {
                        if (ldElements.Count == group.Count())
                        {
                            continue;
                        }
                        else
                        {
                            foreach (CDBSTvEngData element in ldElements)
                            {
                                query_ParentStations.Remove(element);
                            }
                        }
                    }
                }
            }

            return query_ParentStations;
        }

        /// <summary>
        /// Get translators out of CDBS TVUS Data
        /// </summary>
        /// <param name="cdbsUSStations">CDBS TVUS Data</param>
        /// <returns>List of Translators to be processed</returns>
        private List<CDBSTvEngData> GetTranslators(List<CDBSTvEngData> cdbsUSStations)
        {
            // query lp1 records with full information 
            string[] allowedTypes = new[] { "LD", "TX", "DC" };

            return (from engRow in cdbsUSStations
                    where allowedTypes.Contains(engRow.VsdService)
                    && (engRow.FacStatus == "LICEN" || engRow.FacStatus == "CPOFF")
                    && engRow.ParentFacilityId > 0
                    select engRow).ToList();
        }

        /// <summary>
        /// verify whether translator is falling with in 80 kms range or not from parent contours
        /// </summary>
        /// <param name="translator">translator</param>
        /// <param name="parentStation">parent station</param>
        /// <param name="translatorWaivers">waivers list</param>
        /// <returns>bool value</returns>
        private bool ValidateTranslatorDistence(CDBSTvEngData translator, CDBSTvEngData parentStation, List<DynamicTableEntity> translatorWaivers)
        {
            bool translatorInWaiverList = translatorWaivers.Any(obj => obj.RowKey == translator.CallSign);//|| obj.Properties["TransmitCallsign"].StringValue == parenStation.CallSign
            Contour contour = null;
            var transmitterLocation = new Location(parentStation.Latitude, parentStation.Longitude);
            var receiverLocation = new Location(translator.Latitude, translator.Longitude);

            if (!string.IsNullOrEmpty(parentStation.Contour))
            {
                try
                {
                    contour = JsonConvert.DeserializeObject<Contour>(parentStation.Contour);
                }
                catch
                {
                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, string.Format("Skipping translator {0} because of invalid contours in parent callsign {1}", translator.CallSign, parentStation.CallSign));
                    return false;
                }
            }
            else
            {
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, string.Format("Skipping translator {0} because of no contours in parent callsign {1}", translator.CallSign, parentStation.CallSign));
                return false;
            }

            if (!translatorInWaiverList)
            {
                if (!GeoCalculations.IsValidTranslatorDistance(contour.ContourPoints, transmitterLocation, receiverLocation, this.Logger, translator.CallSign))
                {
                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, string.Format("Skipping translator {0} because of invalid transaltor distance with {1} ({2}, {3})", translator.CallSign, parentStation.CallSign, translator.Channel, parentStation.Channel));
                    return false;
                }
            }
            else
            {
                if (GeoCalculations.IsPointInPolygon(contour.ContourPoints, receiverLocation))
                {
                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, string.Format("Skipping translator {0} in waiver list  because it is falling inside contours of parent with callsign {1}", translator.CallSign, parentStation.CallSign));
                    return false;
                }

                this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, string.Format("Translator {0} is in waiver List", translator.CallSign));
            }

            return true;
        }

        /// <summary>
        /// check whther translator callsign is present in latest translator data records recently downloaded.
        /// If yes, check corresponding whitespace Id is there in latest translator inputs.
        /// If yes, list out all those inputs and assign it to updatedTranslatorInputs
        /// </summary>
        /// <param name="translator">processing translator</param>
        /// <param name="translatorDataRecords">latest translator data records downloaded from CDBS</param>
        /// <param name="translatorInputRecords">latest translator inputs downloaded from CDBS</param>
        /// <param name="updatedTranslatorInputs">latest translator inputs available for given translator</param>
        /// <returns>bool value representing whether updated translator inputs are available</returns>
        private bool CheckTranslatorUpdates(CDBSTvEngData translator, List<TranslatorData> translatorDataRecords, List<TranslatorInput> translatorInputRecords, ref List<KeyValuePair<string, int>> updatedTranslatorInputs)
        {
            //Find Any Updates Available
            var translatorData = translatorDataRecords.FirstOrDefault(x => x.Facility_Id == translator.FacilityId);

            if (translatorData != null)
            {
                var requiredInputs = translatorInputRecords.Where(x => x.Whitespace_Id == translatorData.Whitespaces_Id).ToList();

                if (requiredInputs.Count > 0)
                {
                    foreach (var translatorInput in requiredInputs)
                    {
                        CDBSTvEngData updatedParentRecord = new CDBSTvEngData();

                        if (translatorInput.DelivaryMethod == "v" || translatorInput.DelivaryMethod == "t")
                        {
                            updatedTranslatorInputs.Add(new KeyValuePair<string, int>(translatorInput.PrimaryCallSign, translatorInput.PrimaryChannel));
                        }
                        else if (translatorInput.DelivaryMethod == "m" && translatorInput.ProgramOriginalChannel > 0)
                        {
                            updatedTranslatorInputs.Add(new KeyValuePair<string, int>(translatorInput.ProgramOriginalCallsign, translatorInput.ProgramOriginalChannel));
                        }
                        else if (translatorInput.DelivaryMethod == "s")
                        {
                            continue;
                        }
                    }

                    return true;
                }
            }

            return false;
        }

        private List<CDBSTvEngData> GetUpdatedParentTranslators(List<KeyValuePair<string, int>> updatedTranslatorInputs, List<CDBSTvEngData> parentList, CDBSTvEngData translator)
        {
            List<CDBSTvEngData> updatedParents = new List<CDBSTvEngData>();

            foreach (var translatorInput in updatedTranslatorInputs)
            {
                //Find if updated parent is there in cdbsStations
                var machingParents = parentList.Where(x => x.CallSign == translatorInput.Key && x.Channel == translatorInput.Value).ToList();

                //If Yes
                if (machingParents.Count > 0)
                {
                    if (machingParents.Count == 1)
                    {
                        updatedParents.Add(machingParents[0]);
                    }
                    else
                    {
                        // If there is ambiguity consider nearest one
                        CDBSTvEngData requiredParent = new CDBSTvEngData();
                        double mindistance = 0.0;

                        foreach (var parent in machingParents)
                        {
                            Location translatorLocation = new Location
                            {
                                Latitude = translator.Latitude,
                                Longitude = translator.Longitude
                            };

                            Location parentLocation = new Location
                            {
                                Latitude = parent.Latitude,
                                Longitude = parent.Longitude
                            };


                            var distance = GeoCalculations.GetDistance(translatorLocation, parentLocation).InKm();

                            if (mindistance == 0.0 || (distance < mindistance))
                            {
                                mindistance = distance;
                                requiredParent = parent;
                            }
                        }

                        updatedParents.Add(requiredParent);
                    }
                }
            }

            return updatedParents;
        }


        /// <summary>
        /// Processes the CDBS canadian data.
        /// </summary>
        public void ProcessCDBSCanadianData()
        {
            const string LogMethodName = "FccRegionSync.ProcessCDBSCanadianData";

            Stopwatch stopwatch = new Stopwatch();
            stopwatch.Start();

            // Begin  Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Enter " + LogMethodName);

            int status = 0;

            try
            {
                List<CDBSTvEngData> tvstatio = new List<CDBSTvEngData>();
                List<string[]> antennaPatternStat = new List<string[]>();
                List<string[]> tvapatKey = new List<string[]>();
                List<string[]> tvapatDesc = new List<string[]>();
                List<CDBSAntennaPattern> antennaPatterns = new List<CDBSAntennaPattern>();

                Action<string[]> tvstatioFunc = (items) =>
                {
                    ////#0             |1       |2   |3        |4        |5    |6       |7        |8     |9     |10     |11      |12     |13    |14      |15      |16       |17    |18      |19      |20      |21      |22    |23     |24      |25        |26        |27       |28       |29        |30       |31    |32    |33    |34    |35    |36    |37    |38        |39       |40        |41     |EOL
                    ////#CALLS_BANR_KEY|PROVINCE|CITY|CALL_SIGN|FREQUENCY|CLASS|LATITUDE|LONGITUDE|BANNER|LIMITE|NETWORK|ANT_MODE|BC_MODE|OFFSET|OFF_PREC|BRDR_LAT|BRDR_LONG|BORDER|CAN_LAND|USA_LAND|FRE_LAND|ST_CREAT|ST_MOD|OK_DUMP|DOC_FILE|DEC_NUMBER|UNATTENDED|CERT_NUMB|CLOSE_CAP|ALLOC_ZONE|BEAM_TILT|EHAATT|ERPVAV|ERPVPK|ERPAAV|ERPAPK|ERPVTA|ERPATA|GROUND_LEV|OVERALL_H|RAD_CENTER|CHANNEL|EOL

                    // skip header row
                    if (items[0] == "#CALLS_BANR_KEY")
                    {
                        return;
                    }

                    // check banner
                    if (!(items[8] == "AU" || items[8] == "OP" || items[8] == "TO"))
                    {
                        return;
                    }

                    // check border
                    if (items[17].ToInt32() > 400)
                    {
                        return;
                    }

                    CDBSTvEngData row = new CDBSTvEngData();
                    row.CallBannerKey = items[0];
                    row.CallSign = items[3];
                    row.Border = items[17].ToInt32();
                    row.LatDeg = items[6].Substring(0, 2).ToInt32();
                    row.LatMin = items[6].Substring(2, 2).ToInt32();
                    row.LatSec = items[6].Substring(4, items[6].Length - 5).ToInt32();

                    row.LonDeg = items[7].Substring(0, 2).ToInt32();
                    row.LonMin = items[7].Substring(2, 2).ToInt32();
                    row.LonSec = items[7].Substring(4, items[6].Length - 5).ToInt32();

                    row.Latitude = GeoCalculations.GetLattitudeInDegrees(row.LatDeg, row.LatMin, row.LatSec, "N");
                    row.Longitude = GeoCalculations.GetLongitudeInDegrees(row.LonDeg, row.LonMin, row.LonSec, "W");

                    // if ERPATA is 1 or 2 Digital else Analog
                    var erpata = items[37].ToInt32();

                    if (erpata == 1 || erpata == 2)
                    {
                        row.VsdService = "DT";
                    }
                    else
                    {
                        row.VsdService = "AN";
                    }

                    var power = items[33].ToDouble();
                    if (power == 0)
                    {
                        power = items[32].ToDouble();
                        if (power <= 0)
                        {
                            power = 1000.0;
                        }
                    }

                    // conver to kw
                    row.TxPower = power / 1000.0;

                    // height of antenna above terrain
                    double ehaat = items[31].ToDouble();
                    if (ehaat == 0)
                    {
                        row.Height = 30;
                    }
                    else
                    {
                        row.Height = ehaat;
                    }

                    row.RcamslHorizMtr = row.Height;
                    row.HagRcMtr = items[39].ToDouble();
                    row.HaatRcMtr = items[38].ToDouble();

                    row.Channel = items[41].ToInt32();
                    if (row.Channel > 51 || row.Channel < 2)
                    {
                        return;
                    }

                    row.PartitionKey = "CDBS";
                    row.RowKey = row.CallBannerKey;
                    row.EntityType = Constants.EntityTypeCanadaStation;
                    tvstatio.Add(row);
                };

                Action<string[]> tvapatStatFunc = (items) =>
                {
                    ////#0             |1       |EOL
                    ////#CALLS_BANR_KEY|PATT_KEY|EOL
                    if (items[0] == "#CALLS_BANR_KEY")
                    {
                        return;
                    }

                    antennaPatternStat.Add(items);
                };

                Action<string[]> antennaPatternKeyFunc = (items) =>
                {
                    ////#0             |1       |EOL
                    ////#CALLS_BANR_KEY|PATT_KEY|EOL
                    if (items[0] == "#CALLS_BANR_KEY")
                    {
                        return;
                    }

                    tvapatKey.Add(items);
                };

                Action<string[]> antennaPatternDescFunc = (items) =>
                {
                    ////#0       |1      |2        |3        |4     |5        |6          
                    ////#PATT_KEY|HOR_VER|PATT_NUMB|PATT_TYPE|PUNITS|NUMPOINTS|PATT_DATE|EOL
                    if (items[0] == "#PATT_KEY")
                    {
                        return;
                    }

                    tvapatDesc.Add(items);
                };

                Action<string[]> antennaPatternFunc = (items) =>
                {
                    ////#0       |1      |2        |3        |4     |5        |6          
                    ////#PATT_KEY|ANGLE  |GAIN     |EOL
                    if (items[0] == "#PATT_KEY")
                    {
                        return;
                    }

                    CDBSAntennaPattern row = new CDBSAntennaPattern();
                    row.PartitionKey = "CDBS_CANADA";
                    row.AntennaId = items[0].ToInt32();
                    row.Azimuth = items[1].ToDouble();
                    row.FieldValue = items[2].ToDouble();
                    row.RowKey = row.AntennaId.ToString();
                    antennaPatterns.Add(row);
                };

                Dictionary<string, Action<string[]>> recordsList = new Dictionary<string, Action<string[]>>();

                recordsList.Add("tv_statio.txt", tvstatioFunc);
                recordsList.Add("tv_apatdesc.txt", antennaPatternDescFunc);
                recordsList.Add("tv_apatstat.txt", tvapatStatFunc);
                recordsList.Add("tv_apatkey.txt", antennaPatternKeyFunc);
                recordsList.Add("tv_apatdat.txt", antennaPatternFunc);

                using (var fileStream = this.CreateCDBSRequest("ca_tv_data.zip", true))
                {
                    ZipArchive zipFile = new ZipArchive(fileStream, ZipArchiveMode.Read);
                    foreach (var action in recordsList)
                    {
                        var zipEntry = zipFile.Entries.FirstOrDefault(obj => obj.Name == action.Key);
                        using (TextReader rdr = new StreamReader(zipEntry.Open()))
                        {
                            string dataLine;
                            while ((dataLine = rdr.ReadLine()) != null)
                            {
                                action.Value(dataLine.Split(new[] { "|" }, StringSplitOptions.None));
                            }
                        }
                    }
                }

                var groupAntennaPatternDetails = antennaPatterns.GroupBy(obj => obj.AntennaId).ToList();

                var antennaPatternKeys = (from tvengRecord in tvstatio
                                          join antennaPatternStatRow in antennaPatternStat on tvengRecord.CallBannerKey equals antennaPatternStatRow[0] into patternKeys
                                          from patternKeyRecord in patternKeys.DefaultIfEmpty()
                                          select new { tveng = tvengRecord, patternKey = patternKeyRecord }).ToList();

                var groupCallBanners = (from patternKeyRecord in antennaPatternKeys.Where(obj => obj.patternKey != null)
                                        join tvapatdescRow in tvapatDesc on patternKeyRecord.patternKey[1] equals tvapatdescRow[0] into patternData
                                        from patternDataRecord in patternData.DefaultIfEmpty()
                                        select new { tvpatDesc = patternDataRecord, tvEng = patternKeyRecord.tveng }).GroupBy(obj => obj.tvEng.CallBannerKey);

                List<CDBSTvEngData> finalRecords = new List<CDBSTvEngData>();
                List<CDBSAntennaPattern> allowedAntennaPatterns = new List<CDBSAntennaPattern>();

                foreach (var callBannerRecord in antennaPatternKeys.Where(obj => obj.patternKey == null))
                {
                    finalRecords.Add(callBannerRecord.tveng);
                }

                foreach (var callBannerRecord in groupCallBanners)
                {
                    var antennaDesc = callBannerRecord.FirstOrDefault();
                    var engRecord = antennaDesc.tvEng;

                    var antennaPattRecord = callBannerRecord.FirstOrDefault(obj => obj.tvpatDesc[3] == "PRECISE");
                    if (antennaPattRecord == null)
                    {
                        antennaPattRecord = callBannerRecord.FirstOrDefault(obj => obj.tvpatDesc[3] == "BRIEF");
                        if (antennaPattRecord == null)
                        {
                            antennaPattRecord = callBannerRecord.FirstOrDefault(obj => obj.tvpatDesc[3] == "THEORETICAL");
                        }
                    }

                    if (antennaPattRecord != null)
                    {
                        var matchingAntennaPattern = groupAntennaPatternDetails.FirstOrDefault(obj => obj.Key == antennaPattRecord.tvpatDesc[0].ToInt32());
                        CDBSAntennaPattern appPattern = matchingAntennaPattern.FirstOrDefault().Copy<CDBSAntennaPattern>();
                        engRecord.AntennaId = appPattern.AntennaId;
                        appPattern.Patterns = JsonSerialization.SerializeObject(matchingAntennaPattern);
                        allowedAntennaPatterns.Add(appPattern);
                    }

                    finalRecords.Add(engRecord);
                }

                if (finalRecords.Any())
                {
                    status = this.CommonDalc.UpdateTable(Constants.CDBSCanadaTvEngDataTableName, finalRecords);
                }

                this.DalcRegionSync.SetDatabaseSyncStatus("CDBS", Constants.CDBSCanadaTvEngDataTableName, status == 1);
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Error " + ex.ToString());
                this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Failure, stopwatch.Elapsed.Milliseconds, LogMethodName);
                this.DalcRegionSync.SetDatabaseSyncStatus("CDBS", Constants.CDBSCanadaTvEngDataTableName, false);
            }

            // End Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Ends " + LogMethodName);
            this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Success, stopwatch.Elapsed.Milliseconds, LogMethodName);
        }

        /// <summary>Processes the ULS files.</summary>
        public void ProcessULS()
        {
            this.ProcessBroadcastAuxiliaryStations();

            this.ProcessPlCmrsStations();

            this.ProcessLicensedLPAuxData();

            this.ProcessUnLicensedLPAuxData();
        }

        /// <summary>
        /// Processes the broadcast auxiliary stations.
        /// </summary>
        public void ProcessBroadcastAuxiliaryStations()
        {
            Stopwatch stopwatch = new Stopwatch();
            stopwatch.Start();

            const string LogMethodName = "FccRegionSync.ProcessBroadcastAuxiliaryStations";

            // Begin  Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Enter " + LogMethodName);

            List<ULSRecord> recordHD = new List<ULSRecord>();
            List<ULSRecord> recordFR = new List<ULSRecord>();
            List<ULSRecord> recordLO = new List<ULSRecord>();
            List<ULSRecord> recordBC = new List<ULSRecord>();

            var hdrecordMapping = ULSMapping.GetRecordMapping(ULSRecordType.HD);
            var frrecordMapping = ULSMapping.GetRecordMapping(ULSRecordType.FR);
            var lorecordMapping = ULSMapping.GetRecordMapping(ULSRecordType.LO);
            var bcrecordMapping = ULSMapping.GetRecordMapping(ULSRecordType.BC);

            Action<string[]> recordHDFunc = (items) =>
            {
                ULSRecord row = new ULSRecord();

                row.RadioServiceCode = items[hdrecordMapping["RadioServiceCode"]];
                row.LicenseStatus = items[hdrecordMapping["LicenseStatus"]];
                row.UniqueSystemIdentifier = items[hdrecordMapping["UniqueSystemIdentifier"]].ToInt32();

                if (row.LicenseStatus != "A")
                {
                    return;
                }

                ////TV Pickup - TP
                ////TV STL - TV Studio Transmitter Link - TS
                ////TV Relay - TV Intercity Relay - TI
                ////TV Translator Relay - TT
                ////TV Microwave Booster - TB
                // Filter Low Power Auzillary Services
                bool isValid = row.RadioServiceCode == "TP" || row.RadioServiceCode == "TS" || row.RadioServiceCode == "TI" || row.RadioServiceCode == "TT" || row.RadioServiceCode == "TB"; //// || row.RadioServiceCode == "RP";

                if (!isValid)
                {
                    return;
                }

                row.CallSign = items[hdrecordMapping["CallSign"]];
                row.RowKey = row.UniqueSystemIdentifier.ToString();
                row.PartitionKey = "ULS";

                recordHD.Add(row);
            };

            Action<string[]> recordFRFunc = (items) =>
            {
                ULSRecord row = new ULSRecord();

                row.UniqueSystemIdentifier = items[frrecordMapping["UniqueSystemIdentifier"]].ToInt32();
                row.FrequencyAssigned = (int)items[frrecordMapping["FrequencyAssigned"]].ToDouble();
                row.TxPower = items[frrecordMapping["EIRP"]].ToDouble();
                row.Channel = Conversion.FrequencyToChannel(row.FrequencyAssigned);

                if (row.Channel == 0 || row.Channel < 2 || row.Channel > 51)
                {
                    return;
                }

                row.ClassStationCode = items[frrecordMapping["ClassStationCode"]];
                recordFR.Add(row);
            };

            Action<string[]> recordLOFunc = (items) =>
            {
                ULSRecord row = new ULSRecord();

                row.UniqueSystemIdentifier = items[lorecordMapping["UniqueSystemIdentifier"]].ToInt32();

                row.LocationNumber = items[lorecordMapping["LocationNumber"]].ToInt32();
                row.LocationClassCode = items[lorecordMapping["LocationClassCode"]];

                row.Longitude = GeoCalculations.GetLongitudeInDegrees(items[lorecordMapping["LongitudeDegrees"]].ToDouble(), items[lorecordMapping["LongitudeMinutes"]].ToDouble(), items[lorecordMapping["LongitudeSeconds"]].ToDouble(), items[lorecordMapping["LongitudeDirection"]]);
                row.Latitude = GeoCalculations.GetLattitudeInDegrees(items[lorecordMapping["LatitudeDegrees"]].ToDouble(), items[lorecordMapping["LatitudeMinutes"]].ToDouble(), items[lorecordMapping["LatitudeSeconds"]].ToDouble(), items[lorecordMapping["LatitudeDirection"]]);

                if (row.Longitude == 0.0 || row.Latitude == 0.0)
                {
                    return;
                }

                row.State = items[lorecordMapping["LocationState"]];
                row.RadiusOfOperation = items[lorecordMapping["RadiusOfOperation"]].ToDouble();
                recordLO.Add(row);
            };

            Action<string[]> recordBCFunc = (items) =>
            {
                ULSRecord row = new ULSRecord();

                row.UniqueSystemIdentifier = items[bcrecordMapping["UniqueSystemIdentifier"]].ToInt32();
                row.BroadcastCallSign = items[bcrecordMapping["BroadcastCallSign"]];
                row.FacilityIDOfParentStation = items[bcrecordMapping["FacilityIDOfParentStation"]].ToInt32();
                if (row.FacilityIDOfParentStation == 0)
                {
                    return;
                }

                row.BroadcastCity = items[bcrecordMapping["BroadcastCity"]];
                row.BroadcastState = items[bcrecordMapping["BroadcastState"]];
                row.RadioServiceCodeOfParentStation = items[bcrecordMapping["RadioServiceCodeOfParentStation"]];

                recordBC.Add(row);
            };

            Dictionary<ULSRecordType, Action<string[]>> recordsList = new Dictionary<ULSRecordType, Action<string[]>>();

            recordsList.Add(ULSRecordType.HD, recordHDFunc);
            recordsList.Add(ULSRecordType.BC, recordBCFunc);
            recordsList.Add(ULSRecordType.LO, recordLOFunc);
            recordsList.Add(ULSRecordType.FR, recordFRFunc);

            var fileTypes = new List<ULSFileType>() { ULSFileType.LMCast, ULSFileType.LMicro };
            var status = 1;

            try
            {
                long contentLength = 0;

                foreach (var ulsFileType in fileTypes)
                {
                    using (var fileStream = this.CreateULSRequest(ulsFileType, Utils.Configuration["IsULSIncremental"].ToBool(), out contentLength))
                    {
                        if (Utils.Configuration["IsULSIncremental"].ToBool() && contentLength <= 160)
                        {
                            continue;
                        }

                        ZipArchive zipFile = new ZipArchive(fileStream, ZipArchiveMode.Read);

                        string dataLine;

                        foreach (var action in recordsList)
                        {
                            var zipEntry = zipFile.Entries.FirstOrDefault(obj => obj.Name == (action.Key.ToString() + ".dat"));
                            using (TextReader rdr = new StreamReader(zipEntry.Open()))
                            {
                                while ((dataLine = rdr.ReadLine()) != null)
                                {
                                    action.Value(dataLine.Split(new[] { "|" }, StringSplitOptions.None));
                                }
                            }
                        }
                    }

                    var joinedData = from hdrow in recordHD
                                     join frrow in recordFR on hdrow.UniqueSystemIdentifier equals frrow.UniqueSystemIdentifier
                                     join lorow in recordLO on frrow.UniqueSystemIdentifier equals lorow.UniqueSystemIdentifier
                                     join bcrow in recordBC on lorow.UniqueSystemIdentifier equals bcrow.UniqueSystemIdentifier
                                     select new { hd = hdrow, fr = frrow, lo = lorow, bc = bcrow };

                    var groupedJoined = joinedData.GroupBy(obj => new { obj.hd.UniqueSystemIdentifier });

                    List<ULSRecord> ulsRecords = new List<ULSRecord>();

                    foreach (var groupData in groupedJoined)
                    {
                        var groupRecord = groupData.FirstOrDefault();
                        var ulsRecord = groupRecord.hd;

                        ulsRecord.MergeBroadcastCallSignData(groupRecord.bc);
                        ulsRecord.MergeFrequencyData(groupRecord.fr);
                        foreach (var location in groupData.Select(obj => obj.lo))
                        {
                            ulsRecord.MergeLocationData(location);
                        }

                        ulsRecord.AzimuthToParent = GeoCalculations.CalculateBearing(ulsRecord.Location, ulsRecord.TxLocation);
                        ulsRecord.KeyHoleRadiusMtrs = GeoCalculations.GetDistance(ulsRecord.Location, ulsRecord.TxLocation).InMeter();
                        ulsRecords.Add(ulsRecord);
                    }

                    ////this.DumpFile<ULSRecord>(ulsRecords, "broadcastAuxiliary" + ulsFileType.ToString());
                    recordBC.Clear();
                    recordHD.Clear();
                    recordFR.Clear();
                    recordLO.Clear();

                    if (ulsRecords.Count > 0)
                    {
                        status = this.DalcRegionSync.UpdateULSData(Constants.ULSBroadcastAuxiliaryTableName, ulsRecords);
                    }

                    this.DalcRegionSync.SetDatabaseSyncStatus("ULS", Constants.ULSBroadcastAuxiliaryTableName, status == 1);
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Error " + ex.ToString());
                this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Failure, stopwatch.Elapsed.Milliseconds, LogMethodName);
                this.DalcRegionSync.SetDatabaseSyncStatus("ULS", Constants.ULSBroadcastAuxiliaryTableName, false);
            }

            // End Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Ends " + LogMethodName);
            this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Success, stopwatch.Elapsed.Milliseconds, LogMethodName);
        }

        /// <summary>
        /// Processes the PLMRS and CMRS stations.
        /// </summary>
        public void ProcessPlCmrsStations()
        {
            Stopwatch stopwatch = new Stopwatch();
            stopwatch.Start();

            const string LogMethodName = "FccRegionSync.ProcessPlCmrsStations";

            // Begin  Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Enter " + LogMethodName);

            List<ULSRecord> recordHD = new List<ULSRecord>();
            List<ULSRecord> recordFR = new List<ULSRecord>();
            List<ULSRecord> recordLO = new List<ULSRecord>();

            // IG to YW belong to "Land Mobile - Private"
            // IK and YK belong to "Land Mobile - Commercial"
            // CD belongs to "Paging - 47 CFR Part 22"
            string[] hdallowedCodes = new[] { "IG", "PW", "YG", "YW", "IK", "YK", "CD" };
            var hdrecordMapping = ULSMapping.GetRecordMapping(ULSRecordType.HD);
            var frrecordMapping = ULSMapping.GetRecordMapping(ULSRecordType.FR);
            var lorecordMapping = ULSMapping.GetRecordMapping(ULSRecordType.LO);
            HashSet<int> ulsSet = new HashSet<int>();
            Dictionary<int, List<int>> ulsToLocationMapper = new Dictionary<int, List<int>>();
            Dictionary<int, List<Tuple<int, double, double>>> ulsMobileToBaseStationMapper = new Dictionary<int, List<Tuple<int, double, double>>>();

            Action<string[]> recordHDFunc = (items) =>
            {
                if (items[hdrecordMapping["LicenseStatus"]] != "A")
                {
                    return;
                }

                // check developmental_or_sta = "S", "M" or "N"
                var devMode = items[hdrecordMapping["DevelopmentalOrSTAOrDemonstration"]];
                if (!(devMode == "S" || devMode == "M" || devMode == "N"))
                {
                    return;
                }

                if (!hdallowedCodes.Contains(items[hdrecordMapping["RadioServiceCode"]]))
                {
                    return;
                }

                ULSRecord row = new ULSRecord();
                row.RadioServiceCode = items[hdrecordMapping["RadioServiceCode"]];
                row.DevOrSTAMode = items[hdrecordMapping["DevelopmentalOrSTAOrDemonstration"]];
                row.LicenseStatus = items[hdrecordMapping["LicenseStatus"]];
                row.UniqueSystemIdentifier = items[hdrecordMapping["UniqueSystemIdentifier"]].ToInt32();
                row.CallSign = items[hdrecordMapping["CallSign"]];
                row.RowKey = row.UniqueSystemIdentifier.ToString();
                row.PartitionKey = "ULS";

                recordHD.Add(row);
            };

            Action<string[]> recordFRFunc = (items) =>
            {
                var uniqueSystemIdentifier = items[frrecordMapping["UniqueSystemIdentifier"]].ToInt32();
                if (!ulsSet.Contains(uniqueSystemIdentifier))
                {
                    return;
                }

                var frequencyAssigned = (int)items[frrecordMapping["FrequencyAssigned"]].ToDouble();

                // frequency_assigned =>470 and =<512 from the FR table
                if (!(frequencyAssigned >= 470 && frequencyAssigned <= 512))
                {
                    return;
                }

                // logic to store all the the operating frequencies location numbers.
                if (!ulsToLocationMapper.ContainsKey(uniqueSystemIdentifier))
                {
                    ulsToLocationMapper.Add(uniqueSystemIdentifier, new List<int>());
                }

                int locationNumber = items[frrecordMapping["LocationNumber"]].ToInt32();

                if (!ulsToLocationMapper[uniqueSystemIdentifier].Contains(locationNumber))
                {
                    ulsToLocationMapper[uniqueSystemIdentifier].Add(locationNumber);
                }

                ULSRecord row = new ULSRecord();
                row.UniqueSystemIdentifier = items[frrecordMapping["UniqueSystemIdentifier"]].ToInt32();
                row.TxPower = items[frrecordMapping["EIRP"]].ToDouble();
                row.FrequencyNumber = items[frrecordMapping["FrequencyNumber"]].ToInt32();
                row.FrequencyLocationNumber = locationNumber;
                row.LocationNumber = locationNumber;
                row.FrequencyAssigned = (int)items[frrecordMapping["FrequencyAssigned"]].ToDouble();
                row.Channel = Conversion.FrequencyToChannel(row.FrequencyAssigned);
                row.ClassStationCode = items[frrecordMapping["ClassStationCode"]];

                recordFR.Add(row);
            };

            Action<string[]> recordLOFunc = (items) =>
            {
                var uniqueSystemIdentifier = items[lorecordMapping["UniqueSystemIdentifier"]].ToInt32();
                if (!ulsSet.Contains(uniqueSystemIdentifier))
                {
                    return;
                }

                ULSRecord row = new ULSRecord();

                row.UniqueSystemIdentifier = items[lorecordMapping["UniqueSystemIdentifier"]].ToInt32();
                row.CallSign = items[lorecordMapping["CallSign"]];
                row.CorrespondingFixedLocation = items[lorecordMapping["CorrespondingFixedLocation"]];

                int locationNumber = items[lorecordMapping["LocationNumber"]].ToInt32();

                bool existEntryInFRRecord = ulsToLocationMapper.ContainsKey(uniqueSystemIdentifier);

                // Discard LO entry, if there isn't corresponding FR entry.
                if (!existEntryInFRRecord)
                {
                    return;
                }

                // Discard LO entry, if the LO entry does not correspond to the operating TV Frequencies in FR.
                if (!ulsToLocationMapper[uniqueSystemIdentifier].Contains(locationNumber))
                {
                    return;
                }

                row.LocationNumber = locationNumber;
                row.City = items[lorecordMapping["LocationCity"]];

                row.Longitude = GeoCalculations.GetLongitudeInDegrees(items[lorecordMapping["LongitudeDegrees"]].ToDouble(), items[lorecordMapping["LongitudeMinutes"]].ToDouble(), items[lorecordMapping["LongitudeSeconds"]].ToDouble(), items[lorecordMapping["LongitudeDirection"]]);
                row.Latitude = GeoCalculations.GetLattitudeInDegrees(items[lorecordMapping["LatitudeDegrees"]].ToDouble(), items[lorecordMapping["LatitudeMinutes"]].ToDouble(), items[lorecordMapping["LatitudeSeconds"]].ToDouble(), items[lorecordMapping["LatitudeDirection"]]);

                bool tobeProtected = false;

                if (string.IsNullOrEmpty(row.CorrespondingFixedLocation) && (row.Longitude.CompareTo(0.0) != 0 && row.Latitude.CompareTo(0.0) != 0))
                {
                    tobeProtected = true;

                    if (!ulsMobileToBaseStationMapper.ContainsKey(uniqueSystemIdentifier))
                    {
                        ulsMobileToBaseStationMapper.Add(uniqueSystemIdentifier, new List<Tuple<int, double, double>>());
                    }

                    ulsMobileToBaseStationMapper[uniqueSystemIdentifier].Add(new Tuple<int, double, double>(locationNumber, row.Latitude, row.Longitude));
                }
                else if (!string.IsNullOrEmpty(row.CorrespondingFixedLocation) && (row.Longitude.CompareTo(0.0) == 0 && row.Latitude.CompareTo(0.0) == 0))
                {
                    tobeProtected = true;

                    if ((!ulsMobileToBaseStationMapper.ContainsKey(uniqueSystemIdentifier)) || (ulsMobileToBaseStationMapper.ContainsKey(uniqueSystemIdentifier) && !ulsMobileToBaseStationMapper[uniqueSystemIdentifier].Any()))
                    {
                        return;
                    }

                    var baseStationCoordinates = ulsMobileToBaseStationMapper[uniqueSystemIdentifier].Where(loc => loc.Item1 == int.Parse(row.CorrespondingFixedLocation));

                    if (!baseStationCoordinates.Any())
                    {
                        return;
                    }

                    var cordinates = baseStationCoordinates.First();

                    row.Latitude = cordinates.Item2;
                    row.Longitude = cordinates.Item3;
                }
                else
                {
                    // If the CorrespondingFixedLocation Not Null and there are co-ordinates present it should be ignored for processing purposes.
                    string errorMessage = string.Format("{0} | Error: Invalid entry found in the LO file. ULS UID:{1}, CorrespondingFixedLocation:{2}, Latitude :{3}, Longitude:{4}", DateTime.UtcNow, uniqueSystemIdentifier, row.CorrespondingFixedLocation, row.Latitude, row.Longitude);

                    this.Logger.Log(TraceEventType.Warning, LoggingMessageId.RegistrationSyncManagerGenericMessage, errorMessage);

                    return;
                }

                if (!tobeProtected)
                {
                    return;
                }

                row.State = items[lorecordMapping["LocationState"]];
                row.RadiusOfOperation = items[lorecordMapping["RadiusOfOperation"]].ToDouble();
                recordLO.Add(row);
            };

            Dictionary<ULSRecordType, Action<string[]>> recordsList = new Dictionary<ULSRecordType, Action<string[]>>();

            recordsList.Add(ULSRecordType.HD, recordHDFunc);
            recordsList.Add(ULSRecordType.FR, recordFRFunc);
            recordsList.Add(ULSRecordType.LO, recordLOFunc);

            var fileTypes = new List<ULSFileType>() { ULSFileType.LMPriv, ULSFileType.LMComm, ULSFileType.IPaging };
            var status = 1;

            try
            {
                List<ULSRecord> totalULSRecords = new List<ULSRecord>();
                long contentLength = 0;

                foreach (var ulsFileType in fileTypes)
                {
                    using (var fileStream = this.CreateULSRequest(ulsFileType, Utils.Configuration["IsULSIncremental"].ToBool(), out contentLength))
                    {
                        if (Utils.Configuration["IsULSIncremental"].ToBool() && contentLength <= 160)
                        {
                            continue;
                        }

                        ZipArchive zipFile = new ZipArchive(fileStream, ZipArchiveMode.Read);

                        string dataLine;

                        foreach (var action in recordsList)
                        {
                            var zipEntry = zipFile.Entries.FirstOrDefault(obj => obj.Name == (action.Key.ToString() + ".dat"));
                            using (TextReader rdr = new StreamReader(zipEntry.Open()))
                            {
                                while ((dataLine = rdr.ReadLine()) != null)
                                {
                                    action.Value(dataLine.Split(new[] { "|" }, StringSplitOptions.None));
                                }
                            }

                            if (action.Key == ULSRecordType.HD)
                            {
                                for (int i = 0; i < recordHD.Count; i++)
                                {
                                    ulsSet.Add(recordHD[i].UniqueSystemIdentifier);
                                }
                            }
                        }
                    }

                    var joinedData = from hdrow in recordHD
                                     join frrow in recordFR on hdrow.UniqueSystemIdentifier equals frrow.UniqueSystemIdentifier
                                     join lorow in recordLO on new { frrow.UniqueSystemIdentifier, frrow.LocationNumber } equals new { lorow.UniqueSystemIdentifier, lorow.LocationNumber }
                                     select new { hd = hdrow, fr = frrow, lo = lorow };

                    // Group by Unique System Identifier (call sign) and Latitude and Longitude (i.e All LO entries operates at a particular location).
                    var groupedJoined = joinedData.GroupBy(obj => new { obj.hd.UniqueSystemIdentifier, obj.lo.Latitude, obj.lo.Longitude });

                    List<ULSRecord> ulsRecords = new List<ULSRecord>();

                    foreach (var groupData in groupedJoined)
                    {
                        // Group by location number and channel to provide protection to base station operation and mobile operations having Co-Channels and Adjacent channels.
                        var channelwiseGroup = groupData.GroupBy(obj => new { obj.lo.LocationNumber, obj.fr.Channel });

                        foreach (var record in channelwiseGroup)
                        {
                            var groupRecord = record.FirstOrDefault();

                            var ulsRecord = groupRecord.hd.Copy();
                            ulsRecord.MergeFrequencyData(groupRecord.fr);
                            ulsRecord.MergePLMRSLocationData(groupRecord.lo);

                            ulsRecord.RowKey = string.Concat(ulsRecord.RowKey, ";", ulsRecord.Channel, ";", ulsRecord.Latitude, ";", ulsRecord.Longitude, ";", ulsRecord.FrequencyLocationNumber, ";", ulsRecord.FrequencyNumber);

                            double distance = 0.0;

                            bool isBaseStation = string.IsNullOrWhiteSpace(ulsRecord.CorrespondingFixedLocation);
                            bool shouldBeProtected = false;

                            if (isBaseStation)
                            {
                                shouldBeProtected = this.IsValidForTbandProtection(ulsRecord.Location, ulsRecord.Channel, out distance);
                            }
                            else
                            {
                                ChannelType channelType = ChannelType.CoChannel;

                                var baseStationData = groupData.Where(entry => entry.hd.UniqueSystemIdentifier == ulsRecord.UniqueSystemIdentifier && entry.lo.LocationNumber == int.Parse(ulsRecord.CorrespondingFixedLocation));

                                if (baseStationData.Any(data => data.fr.Channel == ulsRecord.Channel))
                                {
                                    channelType = ChannelType.CoChannel;
                                }
                                else
                                {
                                    // TODO: Should there be a condition check (baseStationDataChannel + or - 1) == ulsRecord.Channel ?
                                    channelType = ChannelType.AdjacentChannel;
                                }

                                shouldBeProtected = this.IsValidForTbandProtection(ulsRecord.Location, ulsRecord.Channel, out distance, channelType, ulsRecord.RadiusOfOperation);
                            }

                            if (!shouldBeProtected)
                            {
                                continue;
                            }

                            ulsRecord.DistanceToBaseStation = (int)distance;
                            ulsRecords.Add(ulsRecord);
                        }
                    }

                    recordHD.Clear();
                    recordFR.Clear();
                    recordLO.Clear();

                    totalULSRecords.AddRange(ulsRecords);
                }

                var existingUlsData = this.CommonDalc.FetchEntity<ULSRecord>(Utils.GetRegionalTableName(Constants.ULSPLCMRSTableName), null);
                var inActiveRecords = existingUlsData.Where(obj => totalULSRecords.FirstOrDefault(obj2 => obj2.RowKey == obj.RowKey) == null).ToList();

                if (inActiveRecords.Any())
                {
                    this.DalcRegionSync.DeleteULSData(Constants.ULSPLCMRSTableName, inActiveRecords);
                }

                if (totalULSRecords.Count > 0)
                {
                    status = this.CommonDalc.UpdateTable<ULSRecord>(Constants.ULSPLCMRSTableName, totalULSRecords);
                }

                this.DalcRegionSync.SetDatabaseSyncStatus("ULS", Constants.ULSPLCMRSTableName, status == 1);
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Error " + ex.ToString());
                this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Failure, stopwatch.Elapsed.Milliseconds, LogMethodName);
                this.DalcRegionSync.SetDatabaseSyncStatus("ULS", Constants.ULSPLCMRSTableName, false);
            }

            // End Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Ends " + LogMethodName);
            this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Success, stopwatch.Elapsed.Milliseconds, LogMethodName);
        }

        /// <summary>
        /// Processes the licensed LPAUX data.
        /// </summary>
        public void ProcessLicensedLPAuxData()
        {
            Stopwatch stopwatch = new Stopwatch();
            stopwatch.Start();

            const string LogMethodName = "FccRegionSync.ProcessLicensedLPAuxData";

            // Begin  Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Enter " + LogMethodName);

            List<ULSRecord> recordHD = new List<ULSRecord>();
            List<ULSRecord> recordEN = new List<ULSRecord>();
            List<ULSRecord> recordBC = new List<ULSRecord>();

            Action<string[]> recordHDFunc = (items) =>
            {
                var recordMapping = ULSMapping.GetRecordMapping(ULSRecordType.HD);
                ULSRecord row = new ULSRecord();

                row.RadioServiceCode = items[recordMapping["RadioServiceCode"]];
                row.LicenseStatus = items[recordMapping["LicenseStatus"]];
                row.UniqueSystemIdentifier = items[recordMapping["UniqueSystemIdentifier"]].ToInt32();

                if (row.LicenseStatus != "A")
                {
                    return;
                }

                //// LV - Low Power Wireless Assist Video Devices
                //// LP - Broadcast Auxiliary Low Power
                if (!(row.RadioServiceCode == "LP" || row.RadioServiceCode == "LV"))
                {
                    return;
                }

                row.CallSign = items[recordMapping["CallSign"]];
                row.RowKey = row.CallSign;
                row.PartitionKey = "ULS";

                recordHD.Add(row);
            };

            Action<string[]> recordENFunc = (items) =>
            {
                if (items.Length < 27)
                {
                    return;
                }

                var recordMapping = ULSMapping.GetRecordMapping(ULSRecordType.EN);
                ULSRecord row = new ULSRecord();

                row.EntityTypeCode = items[recordMapping["EntityType"]];
                row.UniqueSystemIdentifier = items[recordMapping["UniqueSystemIdentifier"]].ToInt32();
                row.RegEntityName = items[recordMapping["EntityName"]];
                row.RegFirstName = items[recordMapping["FirstName"]];
                row.RegLastName = items[recordMapping["LastName"]];
                row.RegPhone = items[recordMapping["Phone"]];
                row.RegFax = items[recordMapping["Fax"]];
                row.RegEmail = items[recordMapping["Email"]];
                row.RegStreetAddress = items[recordMapping["StreetAddress"]];
                row.RegCity = items[recordMapping["City"]];
                row.State = items[recordMapping["State"]];
                row.RegZipCode = items[recordMapping["ZipCode"]];
                row.RegPOBox = items[recordMapping["POBox"]];
                ////row.FCCRegistrationNumber = items[recordMapping["FCCRegistrationNumber(FRN)"]];
                row.StatusCode = items[recordMapping["StatusCode"]];

                recordEN.Add(row);
            };

            Action<string[]> recordBCFunc = (items) =>
            {
                ULSRecord row = new ULSRecord();
                var recordMapping = ULSMapping.GetRecordMapping(ULSRecordType.BC);

                row.UniqueSystemIdentifier = items[recordMapping["UniqueSystemIdentifier"]].ToInt32();
                row.BroadcastCallSign = items[recordMapping["BroadcastCallSign"]];
                row.RadioServiceCodeOfParentStation = items[recordMapping["RadioServiceCodeOfParentStation"]];
                if (!(row.RadioServiceCodeOfParentStation == "DT" || row.RadioServiceCodeOfParentStation == "AM" || row.RadioServiceCodeOfParentStation == "FM"))
                {
                    return;
                }

                recordBC.Add(row);
            };

            Dictionary<ULSRecordType, Action<string[]>> recordsList = new Dictionary<ULSRecordType, Action<string[]>>();

            recordsList.Add(ULSRecordType.HD, recordHDFunc);
            recordsList.Add(ULSRecordType.BC, recordBCFunc);
            recordsList.Add(ULSRecordType.EN, recordENFunc);

            var fileTypes = new List<ULSFileType>() { ULSFileType.LMCast };
            var status = 1;
            List<ULSRecord> totalUlsRecords = new List<ULSRecord>();

            try
            {
                long contentLength = 0;

                foreach (var ulsFileType in fileTypes)
                {
                    recordHD.Clear();
                    recordBC.Clear();
                    recordEN.Clear();

                    using (var fileStream = this.CreateULSRequest(ulsFileType, Utils.Configuration["IsULSIncremental"].ToBool(), out contentLength))
                    {
                        if (Utils.Configuration["IsULSIncremental"].ToBool() && contentLength <= 160)
                        {
                            continue;
                        }

                        ZipArchive zipFile = new ZipArchive(fileStream, ZipArchiveMode.Read);

                        string dataLine;

                        foreach (var action in recordsList)
                        {
                            var zipEntry = zipFile.Entries.FirstOrDefault(obj => obj.Name == (action.Key.ToString() + ".dat"));
                            using (TextReader rdr = new StreamReader(zipEntry.Open()))
                            {
                                while ((dataLine = rdr.ReadLine()) != null)
                                {
                                    action.Value(dataLine.Split(new[] { "|" }, StringSplitOptions.None));
                                }
                            }
                        }
                    }

                    var joinedData = from hdrow in recordHD
                                     join enrow in recordEN on hdrow.UniqueSystemIdentifier equals enrow.UniqueSystemIdentifier into joinEnData
                                     from joinenrow in joinEnData.DefaultIfEmpty()
                                     join bcrow in recordBC on hdrow.UniqueSystemIdentifier equals bcrow.UniqueSystemIdentifier
                                     select new { hd = hdrow, en = joinenrow, bc = bcrow };

                    var groupedJoined = joinedData.GroupBy(obj => new { obj.hd.CallSign });

                    List<ULSRecord> ulsRecords = new List<ULSRecord>();

                    foreach (var groupData in groupedJoined)
                    {
                        var groupRecord = groupData.FirstOrDefault();
                        var ulsRecord = groupRecord.hd.Copy();
                        ulsRecord.MergeBroadcastCallSignData(groupRecord.bc);
                        foreach (var registration in groupData.Select(obj => obj.en))
                        {
                            ulsRecord.MergeEntityRegistrationData(registration);
                        }

                        ulsRecords.Add(ulsRecord);
                    }

                    recordHD.Clear();
                    recordEN.Clear();
                    recordBC.Clear();

                    totalUlsRecords.AddRange(ulsRecords);
                }

                ////DumpFile<ULSRecord>(ulsRecords, "LicensedLPAuxData");
                if (totalUlsRecords.Count > 0)
                {
                    status = this.DalcRegionSync.UpdateULSData(Constants.ULSLicensedLPAuxTableName, totalUlsRecords);
                }

                this.DalcRegionSync.SetDatabaseSyncStatus("ULS", Constants.ULSLicensedLPAuxTableName, status == 1);
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Error " + ex.ToString());
                this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Failure, stopwatch.Elapsed.Milliseconds, LogMethodName);
                this.DalcRegionSync.SetDatabaseSyncStatus("ULS", Constants.ULSLicensedLPAuxTableName, false);
            }

            // End Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Ends " + LogMethodName);
            this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Success, stopwatch.Elapsed.Milliseconds, LogMethodName);
        }

        /// <summary>
        /// Processes the unlicensed LPAUX data.
        /// </summary>
        public void ProcessUnLicensedLPAuxData()
        {
            Stopwatch stopwatch = new Stopwatch();
            stopwatch.Start();

            const string LogMethodName = "FccRegionSync.ProcessUnLicensedLPAuxData";

            // Begin  Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Enter " + LogMethodName);

            List<ULSRecord> recordRD = new List<ULSRecord>();
            List<ULSRecord> recordNA = new List<ULSRecord>();
            List<ULSRecord> recordVN = new List<ULSRecord>();
            List<ULSRecord> recordL1 = new List<ULSRecord>();
            List<ULSRecord> recordCH = new List<ULSRecord>();

            var rdrecordMapping = ULSMapping.GetRecordMapping(ULSRecordType.RD);
            var narecordMapping = ULSMapping.GetRecordMapping(ULSRecordType.NA);
            var vnrecordMapping = ULSMapping.GetRecordMapping(ULSRecordType.VN);
            var l1recordMapping = ULSMapping.GetRecordMapping(ULSRecordType.L1);
            var chrecordMapping = ULSMapping.GetRecordMapping(ULSRecordType.CH);

            Action<string[]> recordRDFunc = (items) =>
            {
                if (items[rdrecordMapping["ApplicationStatus"]] != "Q")
                {
                    return;
                }

                ULSRecord row = new ULSRecord();
                row.RadioServiceCode = items[rdrecordMapping["RadioServiceCode"]];
                row.ULSFileNumber = items[rdrecordMapping["ULSFileNumber"]];
                row.OwnerName = string.Concat(items[rdrecordMapping["CertifierFirstName"]], ",", items[rdrecordMapping["CertifierLastName"]]);
                row.UniqueSystemIdentifier = items[rdrecordMapping["RegistrationUniqueSystemIdentifier"]].ToInt32();
                row.GrantDate = items[rdrecordMapping["GrantDate"]].ToDateTime("mm/dd/yyyy");
                row.ExpireDate = items[rdrecordMapping["ExpiredDate"]].ToDateTime("mm/dd/yyyy");
                row.MaxWirelessMicrophones = items[rdrecordMapping["MaxWirelessMicrophone"]];
                row.RowKey = row.ULSFileNumber;
                row.PartitionKey = "ULS";
                recordRD.Add(row);
            };

            Action<string[]> recordNAFunc = (items) =>
            {
                ULSRecord row = new ULSRecord();

                row.UniqueSystemIdentifier = items[narecordMapping["UniqueSystemIdentifier"]].ToInt32();
                row.EntityTypeCode = items[narecordMapping["EntityTypeCode"]];
                row.RegEntityName = items[narecordMapping["EntityName"]];
                row.RegFirstName = items[narecordMapping["FirstName"]];
                row.RegLastName = items[narecordMapping["LastName"]];
                row.RegFax = items[narecordMapping["Fax"]];
                row.RegEmail = items[narecordMapping["Email"]];
                row.RegStreetAddress = items[narecordMapping["StreetAddress"]];
                row.RegCity = items[narecordMapping["City"]];
                row.RegState = items[narecordMapping["State"]];
                row.RegZipCode = items[narecordMapping["ZipCode"]];
                row.RegPOBox = items[narecordMapping["POBox"]];
                row.FCCRegistrationNumber = items[narecordMapping["FCCRegistrationNumber(FRN)"]];

                recordNA.Add(row);
            };

            Action<string[]> recordVNFunc = (items) =>
            {
                ULSRecord row = new ULSRecord();

                row.UniqueSystemIdentifier = items[vnrecordMapping["UniqueSystemIdentifier"]].ToInt32();
                row.VenueName = items[vnrecordMapping["VenueName"]];
                row.VenueTypeCode = items[vnrecordMapping["VenueTypeCode"]];

                recordVN.Add(row);
            };

            Action<string[]> recordL1Func = (items) =>
            {
                ULSRecord row = new ULSRecord();

                row.UniqueSystemIdentifier = items[l1recordMapping["UniqueSystemIdentifier"]].ToInt32();

                row.Longitude = GeoCalculations.GetLongitudeInDegrees(items[l1recordMapping["LongitudeDegrees"]].ToDouble(), items[l1recordMapping["LongitudeMinutes"]].ToDouble(), items[l1recordMapping["LongitudeSeconds"]].ToDouble(), items[l1recordMapping["LongitudeDirection"]]);
                row.Latitude = GeoCalculations.GetLattitudeInDegrees(items[l1recordMapping["LatitudeDegrees"]].ToDouble(), items[l1recordMapping["LatitudeMinutes"]].ToDouble(), items[l1recordMapping["LatitudeSeconds"]].ToDouble(), items[l1recordMapping["LatitudeDirection"]]);

                if (row.Longitude.CompareTo(0.0) == 0 || row.Latitude.CompareTo(0.0) == 0)
                {
                    return;
                }

                recordL1.Add(row);
            };

            Action<string[]> recordCHFunc = (items) =>
            {
                ULSRecord row = new ULSRecord();
                row.ULSFileNumber = items[rdrecordMapping["ULSFileNumber"]];
                row.UniqueSystemIdentifier = items[chrecordMapping["UniqueSystemIdentifier"]].ToInt32();
                row.Channel = items[chrecordMapping["TvChannelNumber"]].ToInt32();
                recordCH.Add(row);
            };

            Dictionary<ULSRecordType, Action<string[]>> recordsList = new Dictionary<ULSRecordType, Action<string[]>>();

            recordsList.Add(ULSRecordType.RD, recordRDFunc);
            recordsList.Add(ULSRecordType.NA, recordNAFunc);
            recordsList.Add(ULSRecordType.VN, recordVNFunc);
            recordsList.Add(ULSRecordType.L1, recordL1Func);
            recordsList.Add(ULSRecordType.CH, recordCHFunc);

            var fileTypes = new List<ULSFileType>() { ULSFileType.UnLicensedMicrophone };
            var status = 1;

            try
            {
                long contentLength = 0;

                foreach (var ulsFileType in fileTypes)
                {
                    recordRD.Clear();
                    recordNA.Clear();
                    recordL1.Clear();
                    recordVN.Clear();
                    recordCH.Clear();

                    using (var fileStream = this.CreateULSRequest(ulsFileType, Utils.Configuration["IsULSIncremental"].ToBool(), out contentLength))
                    {
                        if (Utils.Configuration["IsULSIncremental"].ToBool() && contentLength <= 160)
                        {
                            status = 1;
                            continue;
                        }

                        ZipArchive zipFile = new ZipArchive(fileStream, ZipArchiveMode.Read);

                        string dataLine;

                        foreach (var action in recordsList)
                        {
                            var zipEntry = zipFile.Entries.FirstOrDefault(obj => obj.Name == (action.Key.ToString() + ".dat"));
                            using (TextReader rdr = new StreamReader(zipEntry.Open()))
                            {
                                while ((dataLine = rdr.ReadLine()) != null)
                                {
                                    action.Value(dataLine.Split(new[] { "|" }, StringSplitOptions.None));
                                }
                            }
                        }
                    }

                    var joinedData = from rdrow in recordRD
                                     join narow in recordNA on rdrow.UniqueSystemIdentifier equals narow.UniqueSystemIdentifier
                                     join vnrow in recordVN on rdrow.UniqueSystemIdentifier equals vnrow.UniqueSystemIdentifier
                                     join l1row in recordL1 on rdrow.UniqueSystemIdentifier equals l1row.UniqueSystemIdentifier
                                     join chrow in recordCH on rdrow.UniqueSystemIdentifier equals chrow.UniqueSystemIdentifier
                                     select new { rd = rdrow, na = narow, vn = vnrow, l1 = l1row, ch = chrow };

                    var groupedJoined = joinedData.GroupBy(obj => obj.rd.ULSFileNumber);

                    List<ULSRecord> ulsRecords = new List<ULSRecord>();

                    foreach (var groupData in groupedJoined)
                    {
                        var groupRecord = groupData.FirstOrDefault();
                        var ulsRecord = groupRecord.rd.Copy();
                        ulsRecord.MergeNAData(groupRecord.na);
                        ulsRecord.MergeVenueInformation(groupRecord.vn);
                        ulsRecord.MergeLocationData(groupRecord.l1);
                        ulsRecord.Channels = groupData.Select(obj => obj.ch.Channel).Distinct().ToArray();
                        ulsRecords.Add(ulsRecord);
                    }

                    ////this.DumpFile<ULSRecord>(ulsRecords, "ULSUnLicensedLpAux");
                    recordRD.Clear();
                    recordNA.Clear();
                    recordVN.Clear();
                    recordL1.Clear();

                    if (ulsRecords.Count > 0)
                    {
                        status = this.DalcRegionSync.UpdateULSData(Constants.ULSUnLicensedLPAuxTableName, ulsRecords);
                    }

                    this.DalcRegionSync.SetDatabaseSyncStatus("ULS", Constants.ULSUnLicensedLPAuxTableName, status == 1);
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Error " + ex.ToString());
                this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Failure, stopwatch.Elapsed.Milliseconds, LogMethodName);
                this.DalcRegionSync.SetDatabaseSyncStatus("ULS", Constants.ULSUnLicensedLPAuxTableName, false);
            }

            // End Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Ends " + LogMethodName);
            this.Auditor.Audit(AuditId.RegionSync, AuditStatus.Success, stopwatch.Elapsed.Milliseconds, LogMethodName);
        }

        /// <summary>
        /// Creates the request.
        /// </summary>
        /// <param name="appendUri">The append URI.</param>
        /// <param name="isCanadianData">if set to <c>true</c> [is canadian data].</param>
        /// <returns>returns WebRequest.</returns>
        private Stream CreateCDBSRequest(string appendUri, bool isCanadianData = false)
        {
            string uri = @"http://transition.fcc.gov/ftp/Bureaus/MB/Databases/cdbs/";

            if (isCanadianData)
            {
                uri = @"http://data.fcc.gov/download/white-space-database-administration/" + appendUri;
            }
            else
            {
                if (!string.IsNullOrEmpty(appendUri))
                {
                    uri = uri + appendUri;
                }
            }

            long contentLength = 0;

            return this.NetFileStream.DownloadFile(uri, out contentLength);
        }

        /// <summary>
        /// Creates the ULS request.
        /// </summary>
        /// <param name="ulsFileType">Type of the ULS file type.</param>
        /// <param name="isIncremental">if set to <c>true</c> [is incremental].</param>
        /// <param name="contentLength">Length of the content.</param>
        /// <returns>returns WebRequest.</returns>
        private Stream CreateULSRequest(ULSFileType ulsFileType, bool isIncremental, out long contentLength)
        {
            string ftpPartialUri = @"http://wireless.fcc.gov/uls/data/daily/";
            string ftpCompleteUri = @"http://wireless.fcc.gov/uls/data/complete/";

            switch (ulsFileType)
            {
                case ULSFileType.LMPriv:
                    ftpPartialUri = ftpPartialUri + "l_lp_" + DateTime.Now.DayOfWeek.ToString().ToLower().Substring(0, 3) + ".zip";
                    ftpCompleteUri = ftpCompleteUri + "l_LMpriv.zip";
                    break;

                case ULSFileType.LMComm:
                    ftpPartialUri = ftpPartialUri + "l_lc_" + DateTime.Now.DayOfWeek.ToString().ToLower().Substring(0, 3) + ".zip";
                    ftpCompleteUri = ftpCompleteUri + "l_LMcomm.zip";
                    break;

                case ULSFileType.LMicro:
                    ftpPartialUri = ftpPartialUri + "l_mw_" + DateTime.Now.DayOfWeek.ToString().ToLower().Substring(0, 3) + ".zip";
                    ftpCompleteUri = ftpCompleteUri + "l_micro.zip";
                    break;

                case ULSFileType.LMCast:
                    ftpPartialUri = ftpPartialUri + "l_lb_" + DateTime.Now.DayOfWeek.ToString().ToLower().Substring(0, 3) + ".zip";
                    ftpCompleteUri = ftpCompleteUri + "l_LMbcast.zip";
                    break;

                case ULSFileType.UnLicensedMicrophone:
                    ftpPartialUri = ftpPartialUri + "a_um_" + DateTime.Now.DayOfWeek.ToString().ToLower().Substring(0, 3) + ".zip";
                    ftpCompleteUri = ftpCompleteUri + "a_um.zip";
                    break;

                case ULSFileType.IPaging:
                    ftpPartialUri = ftpPartialUri + "l_pg_" + DateTime.Now.DayOfWeek.ToString().ToLower().Substring(0, 3) + ".zip";
                    ftpCompleteUri = ftpCompleteUri + "l_paging.zip";
                    break;
            }

            if (isIncremental)
            {
                return this.NetFileStream.DownloadFile(ftpPartialUri, out contentLength);
            }
            else
            {
                return this.NetFileStream.DownloadFile(ftpCompleteUri, out contentLength);
            }
        }

        /// <summary>
        /// Filters the TBAND protection.
        /// </summary>
        /// <param name="point">The point.</param>
        /// <param name="channel">The channel.</param>
        /// <param name="tbandDistance">The TBAND distance.</param>
        /// <param name="channelType">The type of Channel i.e Co-Channel or Adjacent Channel.</param>
        /// <param name="operationRadius">The operation radius of Mobile operation.</param>
        /// <returns><c>true</c> if [is valid for TBAND protection] [the specified state]; otherwise, <c>false</c>.</returns>
        private bool IsValidForTbandProtection(Location point, int channel, out double tbandDistance, ChannelType channelType = ChannelType.None, double operationRadius = 0)
        {
            tbandDistance = 0.0;
            const double BaseStationMinDistanceInKm = 80;
            const double CoChannelMinDistanceInKm = 134;
            const double AdjcentChannelMinDistanceInKm = 131;

            if (this.uhfBandProtectedLocations == null)
            {
                this.uhfBandProtectedLocations = this.CommonDalc.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.TBandProtectionTable), null);
            }

            bool isValid = true;
            foreach (var curProtectedLocation in this.uhfBandProtectedLocations)
            {
                var channels = curProtectedLocation.Properties["Channel"].StringValue.Split(new[] { "," }, StringSplitOptions.None).Select(obj => obj.ToInt32());
                if (!channels.Contains(channel))
                {
                    isValid = true;
                    continue;
                }

                var tbandLocation = GeoCalculations.GetLocation(curProtectedLocation["Latitude"].StringValue, curProtectedLocation["Longitude"].StringValue);
                var distance = GeoCalculations.GetDistance(point, tbandLocation);

                double distanceInKm = distance.InKm();

                // if distance is less than 80 km from Metro station then no need to add
                if (distanceInKm < BaseStationMinDistanceInKm)
                {
                    isValid = false;
                    break;
                }

                // Co-channel and Adjacent channel protection.
                if ((channelType == ChannelType.CoChannel) && (distanceInKm + operationRadius < CoChannelMinDistanceInKm))
                {
                    isValid = false;
                    break;
                }
                else if ((channelType == ChannelType.AdjacentChannel) && (distanceInKm + operationRadius < AdjcentChannelMinDistanceInKm))
                {
                    isValid = false;
                    break;
                }

                if (tbandDistance == 0)
                {
                    tbandDistance = distanceInKm;
                }

                tbandDistance = Math.Min(tbandDistance, distanceInKm);
            }

            return isValid;
        }

        /// <summary>
        /// calculated distance to TBand Station.
        /// </summary>
        /// <param name="state">The state.</param>
        /// <param name="point">The point.</param>
        /// <returns>returns System.Double.</returns>
        private double TBandDistance(string state, Location point)
        {
            if (this.uhfBandProtectedLocations == null)
            {
                this.uhfBandProtectedLocations = this.CommonDalc.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.TBandProtectionTable), null);
            }

            var curProtectedLocation = this.uhfBandProtectedLocations.FirstOrDefault(obj => obj["State"].StringValue == state);

            if (curProtectedLocation != null)
            {
                var tbandLocation = GeoCalculations.GetLocation(curProtectedLocation["Latitude"].StringValue, curProtectedLocation["Longitude"].StringValue);

                var distance = GeoCalculations.GetDistance(point, tbandLocation);

                // if distance is less than 80 km from Metro station then no need to add
                return distance.InKm();
            }

            return 0;
        }

        /// <summary>
        /// Merges the data.
        /// </summary>
        private void MergeData()
        {
            const string LogMethodName = "FccRegionSync.MergeData";

            // Begin  Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Enter " + LogMethodName);

            var tableNames = new[] { Constants.ULSBroadcastAuxiliaryTableName, Constants.CDBSTranslatorDataTableName, Constants.ULSPLCMRSTableName };
            try
            {
                var existingData = this.CommonDalc.FetchEntity<TableEntity>(Utils.GetRegionalTableName(Constants.MergedCDBSDataTableName), null);

                foreach (var mergedData in existingData.GroupBy(obj => obj.PartitionKey))
                {
                    this.CommonDalc.DeleteRecords(Constants.MergedCDBSDataTableName, mergedData.ToList());
                }

                foreach (var tableName in tableNames)
                {
                    var records = this.CommonDalc.FetchEntity<MergedData>(Utils.GetRegionalTableName(tableName), null);

                    foreach (var tablerow in records)
                    {
                        tablerow.MergedDataIdentifier = tableName;
                    }

                    foreach (var mergedData in records.GroupBy(obj => obj.PartitionKey))
                    {
                        TableBatchOperation batchOperation = new TableBatchOperation();

                        foreach (var incumbent in mergedData)
                        {
                            batchOperation.Add(TableOperation.InsertOrReplace(incumbent));

                            if (batchOperation.Count == 100)
                            {
                                this.CommonDalc.InsertBatch(Constants.MergedCDBSDataTableName, batchOperation);
                                batchOperation.Clear();
                            }
                        }

                        if (batchOperation.Count > 0)
                        {
                            this.CommonDalc.InsertBatch(Constants.MergedCDBSDataTableName, batchOperation);
                            batchOperation.Clear();
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerGenericMessage, string.Format("Error in {0}, Error :{1}", LogMethodName, ex.ToString()));
            }

            // End  Log Transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Exit " + LogMethodName);
        }

        #region Helper Methods

        /// <summary>
        /// Parse the data segment.
        /// </summary>
        /// <param name="dataSegment">The data segment.</param>
        /// <returns>returns System.String[][].</returns>
        private string[] ParseDataSegment(string dataSegment)
        {
            return dataSegment.Split(new[] { '|' }, StringSplitOptions.None);
        }

        #endregion
    }
}
