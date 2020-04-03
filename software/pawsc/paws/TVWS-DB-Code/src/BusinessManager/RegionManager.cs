// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.BusinessManager
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.IO.Compression;
    using System.Linq;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Threading.Tasks;
    using System.Web;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.AzureTableAccess;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Enums;
    using Microsoft.WhiteSpaces.DataAccessManager;
    using Microsoft.WindowsAzure.ServiceRuntime;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;

    public class RegionManager : IRegionManager
    {
        [Dependency]
        public IAuditor Auditor { get; set; }

        [Dependency]
        public ILogger Logger { get; set; }

        private readonly IRegionDataAccess regionDataAccess;

        private readonly IWhitespacesDataClient whitespacesClient;

        private string outputDirectoryPath = string.Empty;

        private string inputDirectoryPath = string.Empty;

        private string zipFilePath = string.Empty;

        public RegionManager(IRegionDataAccess regionDataAccess, IWhitespacesDataClient whitespacesClient)
        {
            this.regionDataAccess = regionDataAccess;
            this.whitespacesClient = whitespacesClient;
        }

        public IEnumerable<MVPDRegistration> GetMvpdRegistrations(string userId)
        {
            var mvpdRegistrations = this.regionDataAccess.GetMvpdRegistrations(userId);

            Parallel.ForEach(
                mvpdRegistrations,
                x =>
                {
                    x.DeSerializeObjectsFromJson();
                });

            return mvpdRegistrations;
        }

        public IEnumerable<LPAuxRegistration> GetLpAuxRegistrations(string userId)
        {
            var lpAuxRegistrations = this.regionDataAccess.GetLpAuxRegistrations(userId);

            Parallel.ForEach(
                lpAuxRegistrations,
                x =>
                {
                    x.DeSerializeObjectsFromJson();

                    if (x.Latitude == default(double) && x.Longitude == default(double))
                    {
                        if (x.PointsArea != null && x.PointsArea.Length == 1)
                        {
                            var position = x.PointsArea.FirstOrDefault();
                            x.Latitude = position.Latitude;
                            x.Longitude = position.Longitude;
                        }
                    }
                });

            return lpAuxRegistrations;
        }

        public IEnumerable<TempBASRegistration> GetTempBasRegistrations(string userId)
        {
            var tempBasRegistrations = this.regionDataAccess.GetTempBasRegistrations(userId);

            Parallel.ForEach(
                tempBasRegistrations,
                x =>
                {
                    x.DeSerializeObjectsFromJson();
                });

            return tempBasRegistrations;
        }

        public void DeleteRegistration(string partitionKey, string rowKey, string etag, RegistrationType registrationType)
        {
            this.regionDataAccess.DeleteRegistration(partitionKey, rowKey, etag, registrationType);
        }

        public bool SaveFeedback(FeedbackInfo info)
        {
            return this.regionDataAccess.SaveFeedback(info);
        }

        public IEnumerable<ExcludedDevice> GetExcludedIdsByRegionCode(string regionCode)
        {
            return this.regionDataAccess.GetExcludedIdsByRegionCode(regionCode);
        }

        public void DeleteExcludedId(string regionCode, string partionKey, string rowKey)
        {
            this.regionDataAccess.DeleteExcludedId(regionCode, partionKey, rowKey);
        }

        public IEnumerable<BlockedChannels> GetExcludedChannelsByRegionCode(string regionCode)
        {
            List<BlockedChannels> blockedChannels = new List<BlockedChannels>();
            object lockObject = new object();

            IEnumerable<ExcludedChannels> excludedChannels = this.regionDataAccess.GetExcludedChannelsByRegionCode(regionCode);

            Parallel.ForEach(
                excludedChannels,
                excludedChannel =>
                {
                    BlockedChannels blockedChannel = GetBlockedChannel(excludedChannel);

                    lock (lockObject)
                    {
                        blockedChannels.Add(blockedChannel);
                    }
                });

            return blockedChannels;
        }

        public void DeleteExcludedChannels(string regionCode, string partitionKey, string rowKey)
        {
            this.regionDataAccess.DeleteExcludedChannels(regionCode, partitionKey, rowKey);
        }

        public BlockedChannels GetBlockedChannel(ExcludedChannels excludedChannel)
        {
            TvSpectrum[] tvSpectrums = JsonHelper.DeserializeObject<TvSpectrum[]>(excludedChannel.ChannelList);

            StringBuilder channelListBuilder = new StringBuilder();
            foreach (TvSpectrum tvSpectrum in tvSpectrums)
            {
                channelListBuilder.Append(tvSpectrum.Channel);
                if (tvSpectrum != tvSpectrums[tvSpectrums.Length - 1])
                {
                    channelListBuilder.Append(", ");
                }
            }

            GeoLocation[] geoLocations = JsonHelper.DeserializeObject<GeoLocation[]>(excludedChannel.Location);

            StringBuilder locationBuilder = new StringBuilder();
            foreach (GeoLocation location in geoLocations)
            {
                Point pointLocation = location.Point.Center;
                locationBuilder.Append(pointLocation.Latitude);
                locationBuilder.Append(",");
                locationBuilder.Append(pointLocation.Longitude);

                if (Array.IndexOf(geoLocations, location) != geoLocations.Length - 1)
                {
                    locationBuilder.Append("<br />");
                }
            }

            return new BlockedChannels
            {
                ChannelList = channelListBuilder.ToString(),
                Locations = locationBuilder.ToString(),
                RecordPartitionKey = excludedChannel.PartitionKey,
                RecordRowKey = excludedChannel.RowKey
            };
        }

        public string[] ProcessInputFiles(Dictionary<string, System.IO.Stream> fileStreams)
        {
            this.CreateInputFileFromStreams(fileStreams);
            this.CreateTempFolders();
            string[] inputFiles = Directory.GetFiles(this.inputDirectoryPath);
            StringBuilder logBuilder = new StringBuilder();

            foreach (string filePath in inputFiles)
            {
                string logText = this.ProcessFile(string.Empty, filePath, string.Empty);
                logBuilder.Append(logText);
            }

            return this.UploadStageTestResults(logBuilder.ToString(), string.Empty);
        }

        public string[] ProcessStageFiles(string[] stages)
        {
            StringBuilder logBuilder = new StringBuilder();
            string stageName = string.Empty;
            this.CreateTempFolders();

            foreach (string stage in stages)
            {
                //if (stage != "Stage1")
                //{
                //    this.ProcessWSDAssignmentFiles(stage);
                //}

                //if (stage == "Stage4")
                //{
                //    this.ProcessUnschdeuledAdjustment();
                //}

                this.ProcessTests(stage, ref logBuilder);
            }

            if (stages.Length > 1)
            {
                stageName = "MultipleStages";
            }
            else
            {
                stageName = stages[0];
            }

            return this.UploadStageTestResults(logBuilder.ToString(), stageName);
        }

        public string ProcessOperationalParameters(Request request)
        {
            RequestParameters parameters = new RequestParameters
            {
                Params = request.Params,
                RegionName = "United Kingdom"
            };

            var result = this.whitespacesClient.GetChannelList(parameters);

            if (result != null)
            {
                return result.ChannelsInCSV;
            }

            return string.Empty;
        }

        private void ProcessTests(string stage, ref StringBuilder logBuilder)
        {
            string fileFormat = string.Empty;
            string[] fileNumbers = null;
            string logText = string.Empty;

            switch (stage)
            {
                case "Stage1":
                    fileFormat = "test cases - stage 1 - lot {0}.csv";
                    fileNumbers = new string[] { "1", "2", "3", "4" };
                    break;

                case "Stage2":
                    fileFormat = "Stage 2 - Lot{0} - Device Parameters.csv";
                    fileNumbers = new string[] { "5A", "5B", "6A", "6C", "7A", "7B", "7C", "8A", "8B" };
                    break;

                case "Stage3":
                    fileFormat = "Stage 3 - Lot{0} - Device Parameters.csv";
                    fileNumbers = new string[] { "9" };
                    break;

                case "Stage4":
                    fileFormat = "Stage 4 - Lot{0} - Device Parameters.csv";
                    fileNumbers = new string[] { "10" };
                    break;

                case "Stage5":
                    fileFormat = "Stage 5 - Lot{0} - Device Parameters.csv";
                    fileNumbers = new string[] { "11", "12", "13", "14" };
                    break;
                case "Stage6":
                    fileFormat = "Stage 6 - Lot{0} - Device Parameters.csv";
                    fileNumbers = new string[] { "15", "16", "17" };
                    break;
            }

            foreach (string fileNumber in fileNumbers)
            {
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.PortalOfcomEvaluation, "processing of stage " + stage + " Device Parameter file Lot " + fileNumber + " started at" + DateTime.Now);

                string filePath = this.GetDirectoryPath(stage) + string.Format(fileFormat, fileNumber);
                logText = this.ProcessFile(fileNumber, filePath, stage);
                logBuilder.Append(logText);

                this.Logger.Log(TraceEventType.Information, LoggingMessageId.PortalOfcomEvaluation, "processing of stage " + stage + " Device Parameter file Lot " + fileNumber + " ended at" + DateTime.Now);
            }
        }

        private byte[] GetBytesFromStream(Stream input)
        {
            byte[] buffer = new byte[16 * 1024];
            using (MemoryStream ms = new MemoryStream())
            {
                int read;
                while ((read = input.Read(buffer, 0, buffer.Length)) > 0)
                {
                    ms.Write(buffer, 0, read);
                }

                return ms.ToArray();
            }
        }

        private void CreateTempFolders()
        {
            // this guard helps not to throw error when running outside emulator in local
            if (RoleEnvironment.IsAvailable)
            {
                var localStorage = RoleEnvironment.GetLocalResource("OutputStorage");
                var rootPath = localStorage.RootPath;

                this.outputDirectoryPath = rootPath + "output";
                this.zipFilePath = rootPath + "output.zip";
            }
            else
            {
                this.outputDirectoryPath = "output";
                this.zipFilePath = "output.zip";
            }

            if (Directory.Exists(this.outputDirectoryPath))
            {
                Directory.Delete(this.outputDirectoryPath, true);
            }

            Directory.CreateDirectory(this.outputDirectoryPath);

            if (File.Exists(this.zipFilePath))
            {
                File.Delete(this.zipFilePath);
            }
        }

        private void CreateInputFileFromStreams(Dictionary<string, System.IO.Stream> fileStreams)
        {
            if (RoleEnvironment.IsAvailable)
            {
                var localStorage = RoleEnvironment.GetLocalResource("OutputStorage");
                var rootPath = localStorage.RootPath;
                this.inputDirectoryPath = rootPath + "input";
            }
            else
            {
                this.inputDirectoryPath = "input";
            }

            if (Directory.Exists(this.inputDirectoryPath))
            {
                Directory.Delete(this.inputDirectoryPath, true);
            }

            Directory.CreateDirectory(this.inputDirectoryPath);

            foreach (var inputStream in fileStreams)
            {
                if (inputStream.Key != "LogFile")
                {
                    string path = this.inputDirectoryPath + "\\" + inputStream.Key;
                    using (var fileStream = new FileStream(path, FileMode.Create, FileAccess.Write))
                    {
                        inputStream.Value.CopyTo(fileStream);
                    }
                }
            }
        }

        private string ProcessFile(string fileNumber, string filePath, string stage)
        {
            if (File.Exists(filePath))
            {
                var csvData = File.ReadAllLines(filePath);
                List<Request> requests = this.GetRequestsFromCsvData(csvData, fileNumber, stage);

                return this.ProcessRequests(requests, fileNumber, stage);
            }

            return string.Empty;
        }

        private List<Request> GetRequestsFromCsvData(string[] csvData, string fileNumber, string stage)
        {
            string pmseTableName = string.Empty;
            List<Request> requests = new List<Request>();
            int stageNumber = 0;

            if (!(stage == "Stage1" || string.IsNullOrEmpty(stage)))
            {
                pmseTableName = string.Format(@"RGN5{0}Lot{1}PMSEassignments", stage, fileNumber);
            }

            if (!string.IsNullOrEmpty(stage))
            {
                stageNumber = Convert.ToInt32(Regex.Match(stage, @"\d+").Value);
            }

            foreach (var csvLine in csvData)
            {
                var csvParams = csvLine.Split(new[] { "," }, StringSplitOptions.None);
                if (csvParams[0] != "Unique_ID" && !string.IsNullOrWhiteSpace(csvParams[0]))
                {
                    double h = Convert.ToDouble(csvParams[5] == string.Empty ? "0" : csvParams[5]);

                    Request request = new Request()
                    {
                        Params = new Parameters()
                        {
                            UniqueId = csvParams[0],
                            Location = new GeoLocation()
                            {
                                Point = new Ellipse()
                                {
                                    Center = new Point()
                                    {
                                        Latitude = csvParams[4],
                                        Longitude = csvParams[3]
                                    },
                                    SemiMajorAxis = (float)csvParams[7].ToDouble(),
                                    SemiMinorAxis = (float)csvParams[8].ToDouble()
                                }
                            },

                            PMSEAssignmentTableName = pmseTableName,
                            TestingStage = stageNumber,
                            Prefsens = csvParams[12].ToDouble(),
                            MaxMasterEIRP = csvParams[13].ToDouble(),
                            IncumbentType = csvParams[11],
                            StartTime = csvParams[2],
                            RequestType = csvParams[1],
                            Antenna = new AntennaCharacteristics()
                            {
                                Height = h,
                                HeightType = (HeightType)Enum.Parse(typeof(HeightType), csvParams[6])
                            },

                            DeviceDescriptor = new DeviceDescriptor()
                            {
                                EtsiDeviceCategory = csvParams[10],
                                EtsiEnDeviceEmissionsClass = csvParams[9],
                                EtsiEnDeviceType = csvParams[11]
                            }
                        }
                    };

                    requests.Add(request);
                }
            }

            return requests;
        }

        private string ProcessRequests(List<Request> requests, string fileNumber, string stage)
        {
            StringBuilder logText = new StringBuilder();

            string outputFilePath = string.Empty;
            string intermediate1Path = string.Empty;
            string masterOperationalPath = string.Empty;

            if (string.IsNullOrEmpty(stage))
            {
                outputFilePath = string.Format(@"{0}\outputLot_{1}.csv", this.outputDirectoryPath, fileNumber);
                intermediate1Path = string.Format(@"{0}\IntermediateResultsLot_{1}.csv", this.outputDirectoryPath, fileNumber);
                masterOperationalPath = string.Format(@"{0}\MasterOperationalParamsLot_{1}.csv", this.outputDirectoryPath, fileNumber);
            }
            else
            {
                string stageDirectory = string.Format(@"{0}\{1}", this.outputDirectoryPath, stage);
                Directory.CreateDirectory(stageDirectory);
                outputFilePath = stageDirectory + "\\outputLot_" + fileNumber + ".csv";
                intermediate1Path = stageDirectory + "\\IntermediateResultsLot_" + fileNumber + ".csv";
                masterOperationalPath = stageDirectory + "\\MasterOperationalParamsLot_" + fileNumber + ".csv";
            }

            File.AppendAllText(outputFilePath, "Unique_ID,RequestType,Time_start,Time_end,Channel21_P_PMSE,Channel21_P1,Channel22_P_PMSE,Channel22_P1,Channel23_P_PMSE,Channel23_P1,Channel24_P_PMSE,Channel24_P1,Channel25_P_PMSE,Channel25_P1,Channel26_P_PMSE,Channel26_P1,Channel27_P_PMSE,Channel27_P1,Channel28_P_PMSE,Channel28_P1,Channel29_P_PMSE,Channel29_P1,Channel30_P_PMSE,Channel30_P1,Channel31_P_PMSE,Channel31_P1,Channel32_P_PMSE,Channel32_P1,Channel33_P_PMSE,Channel33_P1,Channel34_P_PMSE,Channel34_P1,Channel35_P_PMSE,Channel35_P1,Channel36_P_PMSE,Channel36_P1,Channel37_P_PMSE,Channel37_P1,Channel38_P_PMSE,Channel38_P1,Channel39_P_PMSE,Channel39_P1,Channel40_P_PMSE,Channel40_P1,Channel41_P_PMSE,Channel41_P1,Channel42_P_PMSE,Channel42_P1,Channel43_P_PMSE,Channel43_P1,Channel44_P_PMSE,Channel44_P1,Channel45_P_PMSE,Channel45_P1,Channel46_P_PMSE,Channel46_P1,Channel47_P_PMSE,Channel47_P1,Channel48_P_PMSE,Channel48_P1,Channel49_P_PMSE,Channel49_P1,Channel50_P_PMSE,Channel50_P1,Channel51_P_PMSE,Channel51_P1,Channel52_P_PMSE,Channel52_P1,Channel53_P_PMSE,Channel53_P1,Channel54_P_PMSE,Channel54_P1,Channel55_P_PMSE,Channel55_P1,Channel56_P_PMSE,Channel56_P1,Channel57_P_PMSE,Channel57_P1,Channel58_P_PMSE,Channel58_P1,Channel59_P_PMSE,Channel59_P1,Channel60_P_PMSE,Channel60_P1" + Environment.NewLine);
            File.AppendAllText(intermediate1Path, "Unique_ID,Request_type,Time_start,Time_end,Easting,Northing,Distance to PMSE victim  (m),Radius of the Master WSD coverage area d0  (m)" + Environment.NewLine);
            File.AppendAllText(masterOperationalPath, "Unique_ID,Request_type,Time_start,Time_end,Channel21_P_PMSE,Channel21_P1,Channel22_P_PMSE,Channel22_P1,Channel23_P_PMSE,Channel23_P1,Channel24_P_PMSE,Channel24_P1,Channel25_P_PMSE,Channel25_P1,Channel26_P_PMSE,Channel26_P1,Channel27_P_PMSE,Channel27_P1,Channel28_P_PMSE,Channel28_P1,Channel29_P_PMSE,Channel29_P1,Channel30_P_PMSE,Channel30_P1,Channel31_P_PMSE,Channel31_P1,Channel32_P_PMSE,Channel32_P1,Channel33_P_PMSE,Channel33_P1,Channel34_P_PMSE,Channel34_P1,Channel35_P_PMSE,Channel35_P1,Channel36_P_PMSE,Channel36_P1,Channel37_P_PMSE,Channel37_P1,Channel38_P_PMSE,Channel38_P1,Channel39_P_PMSE,Channel39_P1,Channel40_P_PMSE,Channel40_P1,Channel41_P_PMSE,Channel41_P1,Channel42_P_PMSE,Channel42_P1,Channel43_P_PMSE,Channel43_P1,Channel44_P_PMSE,Channel44_P1,Channel45_P_PMSE,Channel45_P1,Channel46_P_PMSE,Channel46_P1,Channel47_P_PMSE,Channel47_P1,Channel48_P_PMSE,Channel48_P1,Channel49_P_PMSE,Channel49_P1,Channel50_P_PMSE,Channel50_P1,Channel51_P_PMSE,Channel51_P1,Channel52_P_PMSE,Channel52_P1,Channel53_P_PMSE,Channel53_P1,Channel54_P_PMSE,Channel54_P1,Channel55_P_PMSE,Channel55_P1,Channel56_P_PMSE,Channel56_P1,Channel57_P_PMSE,Channel57_P1,Channel58_P_PMSE,Channel58_P1,Channel59_P_PMSE,Channel59_P1,Channel60_P_PMSE,Channel60_P1" + Environment.NewLine);

            foreach (var request in requests)
            {
                logText.AppendFormat("Processing UniqueId: {0}", request.Params.UniqueId);
                logText.Append(Environment.NewLine);

                RequestParameters parameters = new RequestParameters
                {
                    Params = request.Params,
                    RegionName = "United Kingdom"
                };

                Stopwatch watch = new Stopwatch();
                watch.Start();

                try
                {
                    var result = this.whitespacesClient.GetChannelList(parameters);
                    File.AppendAllText(outputFilePath, result.ChannelsInCSV + Environment.NewLine);
                    File.AppendAllText(intermediate1Path, result.IntermediateResults1 + Environment.NewLine);
                    File.AppendAllText(masterOperationalPath, result.MasterOperationParameters + Environment.NewLine);
                }
                catch (Exception ex)
                {
                    logText.Append(ex.ToString());
                    logText.Append(Environment.NewLine);
                }

                watch.Stop();
                logText.Append(watch.Elapsed.TotalSeconds);
                logText.Append(Environment.NewLine);
            }

            return logText.ToString();
        }

        private string[] UploadStageTestResults(string logText, string stage)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.PortalOfcomEvaluation, "Uploading output files to blob started at " + DateTime.Now);

            Dictionary<string, Stream> uploadingFiles = new Dictionary<string, Stream>();
            string filePath = string.Empty;
            string fileName = string.Empty;

            if (string.IsNullOrEmpty(stage))
            {
                filePath = "Input" + "_" + DateTime.UtcNow.ToString().Replace('/', '_') + "/";
            }
            else
            {
                filePath = stage + "_" + DateTime.UtcNow.ToString().Replace('/', '_') + "/";
            }

            ZipFile.CreateFromDirectory(this.outputDirectoryPath, this.zipFilePath);
            byte[] bytes = File.ReadAllBytes(this.zipFilePath);
            Stream zipFileStream = new System.IO.MemoryStream(bytes);
            fileName = filePath + "output.zip";
            uploadingFiles.Add(fileName, zipFileStream);

            Stream logStream = new MemoryStream(System.Text.Encoding.UTF8.GetBytes(logText));
            fileName = filePath + "log.txt";
            uploadingFiles.Add(fileName, logStream);

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.PortalOfcomEvaluation, "Uploading output files to blob ended at " + DateTime.Now);


            return this.regionDataAccess.UploadTestResults(uploadingFiles);
        }

        private string GetDirectoryPath(string stage)
        {
            string directoryPath = string.Empty;

            if (string.IsNullOrEmpty(stage))
            {
                directoryPath = "input\\";
            }
            else
            {
                if (RoleEnvironment.IsAvailable)
                {
                    directoryPath = Environment.GetEnvironmentVariable("RoleRoot") + "\\approot\\bin\\App_Data\\OfcomTestEvaluation\\" + stage + "\\";

                    if (!Directory.Exists(directoryPath))
                    {
                        directoryPath = Environment.GetEnvironmentVariable("RoleRoot") + "\\approot\\App_Data\\OfcomTestEvaluation\\" + stage + "\\";
                    }
                }
                else if (HttpContext.Current != null && HttpContext.Current.Server != null)
                {
                    directoryPath = HttpContext.Current.Server.MapPath(@"\\App_Data\\OfcomTestEvaluation\\" + stage + "\\");
                }
                else
                {
                    directoryPath = System.IO.Path.GetFullPath("\\App_Data\\OfcomTestEvaluation\\" + stage + "\\");
                }
            }

            return directoryPath;
        }

        private void ProcessWSDAssignmentFiles(string stage)
        {
            int stageNumber = Convert.ToInt32(Regex.Match(stage, @"\d+").Value);
            string inputFileFormat = "Stage {0} - Lot{1} - PMSE assignments.csv";
            string directoryPath = this.GetDirectoryPath(stage);
            string[] fileNumbers = null;

            switch (stage)
            {
                case "Stage2":
                    fileNumbers = new string[] { "5A", "5B", "6A", "6C", "7A", "7B", "7C", "8A", "8B" };
                    break;

                case "Stage3":
                    fileNumbers = new string[] { "9" };
                    break;

                case "Stage4":
                    fileNumbers = new string[] { "10" };
                    break;

                case "Stage5":
                    fileNumbers = new string[] { "13", "14" };
                    break;

                case "Stage6":
                    fileNumbers = new string[] { "16", "17" };
                    break;
            }

            foreach (string fileNumber in fileNumbers)
            {
                string stage2TableName = string.Format("RGN5{0}Lot{1}PMSEassignments", stage, fileNumber);

                this.Logger.Log(TraceEventType.Information, LoggingMessageId.PortalOfcomEvaluation, "processing of stage " + stage + " PMSE file " + fileNumber + " started at " + DateTime.Now);

                // Get All PmseAssigments
                IEnumerable<PmseAssignment> assignments = this.regionDataAccess.GetAllPmseAssignementEntities(stage2TableName);

                if (assignments.Count() > 0)
                {
                    // delete all assignments
                    this.regionDataAccess.DeletePmseEntities(assignments, stage2TableName);
                }

                string filePath = directoryPath + string.Format(inputFileFormat, stageNumber, fileNumber);

                List<PmseAssignment> pmseAssigments = new List<PmseAssignment>();
                OSGLocation assignmentLocation;
                PmseAssignment pmseAssignment;
                string[] values;

                foreach (var line in File.ReadAllLines(filePath))
                {
                    values = line.Split(',');
                    if (values[0] == "Assignment_ID" || string.IsNullOrEmpty(values[0]))
                    {
                        continue;
                    }

                    assignmentLocation = Conversion.RoundTo10(values[2].ToDouble(), values[3].ToDouble());
                    pmseAssignment = new PmseAssignment();
                    pmseAssignment.PartitionKey = "PMSE";
                    pmseAssignment.RowKey = values[0];
                    pmseAssignment.Assignment_ID = values[0];
                    pmseAssignment.Equipment_Type_ID = values[1];
                    pmseAssignment.OriginalEasting = values[2].ToDouble();
                    pmseAssignment.OriginalNorthing = values[3].ToDouble();
                    pmseAssignment.Easting = assignmentLocation.Easting;
                    pmseAssignment.Northing = assignmentLocation.Northing;
                    pmseAssignment.AntennaHeightMetres = values[4].ToDouble();
                    pmseAssignment.FrequencyMHz = values[5].ToDouble();
                    pmseAssignment.BandwidthMHz = values[6].ToDouble();
                    pmseAssignment.Start = values[7];
                    pmseAssignment.Finish = values[8];
                    pmseAssignment.SituationID = values[9];
                    pmseAssignment.Channel = Conversion.DTTFrequencyToChannel(pmseAssignment.FrequencyMHz);
                    pmseAssignment.ClutterValue = 1;
                    pmseAssigments.Add(pmseAssignment);
                }

                this.regionDataAccess.InsertPmseEntities(pmseAssigments, stage2TableName);

                this.Logger.Log(TraceEventType.Information, LoggingMessageId.PortalOfcomEvaluation, "processing of stage " + stage + " PMSE file " + fileNumber + " ended at " + DateTime.Now);
            }
        }

        private void ProcessUnschdeuledAdjustment()
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.PortalOfcomEvaluation, "processing unscheduled adjustments started at " + DateTime.Now);

            string directoryPath = this.GetDirectoryPath("Stage4");
            string tableName = "RGN5Stage4UnscheduledAdjustments";
            string fileName = "Stage 4 - Unscheduled Adjustments.csv";

            string filePath = directoryPath + fileName;
            try
            {
                // Get All PmseAssigments
                IEnumerable<DTTDataAvailability> dttData = this.regionDataAccess.GetDTTDataAvailability(tableName);

                if (dttData.Count() > 0)
                {
                    // delete all assignments
                    this.regionDataAccess.DeleteDTTDataAvailability(dttData, tableName);
                }

                List<DTTDataAvailability> listPD = new List<DTTDataAvailability>();
                foreach (var line in File.ReadAllLines(filePath))
                {
                    string[] values = line.Split(',');
                    if (values[0] == "Easting" || string.IsNullOrEmpty(values[0]))
                    {
                        continue;
                    }

                    DTTDataAvailability pd = new DTTDataAvailability();
                    var dttvalues = values.Skip(2).Select(obj => obj.ToInt32()).ToArray();
                    pd.DataRecord = Conversion.IntToByte(dttvalues);
                    var test = Conversion.ByteToInt(pd.DataRecord);
                    pd.Easting = values[0].ToInt32();
                    pd.Northing = values[1].ToInt32();
                    pd.PartitionKey = "Stage4Unscheduled";
                    pd.RowKey = string.Format("{0}-{1}", pd.Easting, pd.Northing);

                    listPD.Add(pd);
                }

                this.regionDataAccess.InsertDTTDataAvailability(listPD, tableName);
            }
            catch (Exception)
            {
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.PortalOfcomEvaluation, "processing unscheduled adjustments ended at " + DateTime.Now);
        }
    }
}
