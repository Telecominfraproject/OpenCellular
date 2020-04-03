// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal.Controllers
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Web;
    using System.Web.Mvc;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.BusinessManager;
    using Microsoft.WhiteSpaces.Common;        
    using Microsoft.WindowsAzure.ServiceRuntime;    

    [AllowAnonymous]
    public class TestCaseFileProcessorController : Controller
    {
        private readonly IRegionManager regionManager;

        private string defaultRegion = "United Kingdom";

        public TestCaseFileProcessorController(IRegionManager regionManager)
        {
            Microsoft.WhiteSpaces.Common.Check.IsNotNull(regionManager, "Region Manager");
            this.regionManager = regionManager;
        }

        [Dependency]
        public IAuditor OfcomEvaluationAuditor { get; set; }

        [Dependency]
        public ILogger OfcomEvaluationLogger { get; set; }        

        public ActionResult Index()
        {
            return this.View();
        }

        [HttpPost]
        public PartialViewResult ProcessFiles(FormCollection formCollection)
        {
            string[] links = null;

            var region = CommonUtility.GetRegionByName(this.defaultRegion);
            this.OfcomEvaluationAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
            this.OfcomEvaluationAuditor.TransactionId = this.OfcomEvaluationLogger.TransactionId;

            if (formCollection.AllKeys.Length > 1 && formCollection.AllKeys.Where(x => x.Contains("Stage")).Count() > 0)
            {
                string[] stages = formCollection.AllKeys.Where(x => x.Contains("Stage")).ToArray();
                links = this.regionManager.ProcessStageFiles(stages);

                StringBuilder stagesBuilder = new StringBuilder();

                for (int i = 0; i < stages.Length; i++)
                {
                    stagesBuilder.Append(stages[i]);
                    if (i < stages.Length - 1)
                    {
                        stagesBuilder.Append(", ");
                    }
                }

                if (links.Length > 0)
                {
                    this.OfcomEvaluationAuditor.Audit(AuditId.OfcomEvaluationStageTesting, AuditStatus.Success, default(int), stagesBuilder + " has been executed successfully");
                    this.OfcomEvaluationLogger.Log(TraceEventType.Information, LoggingMessageId.PortalOfcomEvaluation, stagesBuilder + " has been executed successfully");
                }
                else
                {
                    this.OfcomEvaluationAuditor.Audit(AuditId.OfcomEvaluationStageTesting, AuditStatus.Failure, default(int), stagesBuilder + " execution failed");
                    this.OfcomEvaluationLogger.Log(TraceEventType.Error, LoggingMessageId.PortalOfcomEvaluation, stagesBuilder + " execution failed");
                }
            }
            else
            {
                string filePath = string.Empty;

                var files = Request.Files;
                Dictionary<string, Stream> fileStreams = new Dictionary<string, Stream>();

                List<KeyValuePair<string, HttpPostedFileBase>> mapping = new List<KeyValuePair<string, HttpPostedFileBase>>();
                string[] allKeys = files.AllKeys;
                for (int i = 0; i < files.Count; i++)
                {
                    string key = allKeys[i];
                    if (key != null)
                    {
                        HttpPostedFileBase file = this.ChooseFileOrNull(files[i]);
                        string fileName = file.FileName.Substring(file.FileName.LastIndexOf('\\') + 1);
                        fileStreams.Add(fileName, file.InputStream);
                    }
                }

                links = this.regionManager.ProcessInputFiles(fileStreams);

                if (links.Length > 0)
                {
                    this.OfcomEvaluationAuditor.Audit(AuditId.OfcomEvaluationCustomFileTesting, AuditStatus.Success, default(int), "custom file evaluation is successful");
                    this.OfcomEvaluationLogger.Log(TraceEventType.Information, LoggingMessageId.PortalOfcomEvaluation, "custom file evaluation is successful");
                }
                else
                {
                    this.OfcomEvaluationAuditor.Audit(AuditId.OfcomEvaluationCustomFileTesting, AuditStatus.Failure, default(int), "custom file evaluation is failed");
                    this.OfcomEvaluationLogger.Log(TraceEventType.Error, LoggingMessageId.PortalOfcomEvaluation, "custom file evaluation is failed");
                }
            }

            if (links != null && links.Length > 0)
            {
                ViewBag.OutputFiles = links[0];
                ViewBag.TraceFiles = links[1];
            }

            return this.PartialView("DownloadLinks");
        }        

        public PartialViewResult ProcessOperationalParameter(OperationalParametersRequest operationalParams)
        {
            Request request = new Request()
                    {
                        Params = new Parameters()
                        {
                            UniqueId = operationalParams.UniqueId,
                            Location = new GeoLocation()
                            {
                                Point = new Ellipse()
                                {
                                    Center = new Point()
                                    {
                                        Latitude = operationalParams.Latitude.ToString(),
                                        Longitude = operationalParams.Longitude.ToString()
                                    },
                                    SemiMajorAxis = (float)Convert.ToDouble(operationalParams.Latitude_uncertainty),
                                    SemiMinorAxis = (float)Convert.ToDouble(operationalParams.Longitude_uncertainty),
                                }
                            },

                            StartTime = DateTime.Now.ToString(),
                            RequestType = operationalParams.Request_type,
                            Antenna = new AntennaCharacteristics()
                            {
                                Height = Convert.ToDouble(operationalParams.Antenna_Height),
                                HeightType = (HeightType)Enum.Parse(typeof(HeightType), operationalParams.AntennaHeightType)
                            },

                            DeviceDescriptor = new DeviceDescriptor()
                            {
                                EtsiDeviceCategory = operationalParams.Device_category,
                                EtsiEnDeviceEmissionsClass = operationalParams.Emission_class,
                                EtsiEnDeviceType = operationalParams.Device_type
                            }
                        }
                    };

            string responseString = this.regionManager.ProcessOperationalParameters(request);
            var region = CommonUtility.GetRegionByName(this.defaultRegion);
            this.OfcomEvaluationAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
            this.OfcomEvaluationAuditor.TransactionId = this.OfcomEvaluationLogger.TransactionId;

            if (!string.IsNullOrEmpty(responseString))
            {
                string[] responseParams = responseString.Split(',');

                OperationalParametersResponse response = new OperationalParametersResponse
                {
                    Unique_ID = responseParams[0],
                    RequestType = responseParams[1],
                    Time_start = responseParams[2],
                    Time_end = responseParams[3],

                    //----------------------21 - 30 ---------------------------------
                    Channel21_P_PMSE = responseParams[4],
                    Channel21_P1 = responseParams[5],

                    Channel22_P_PMSE = responseParams[6],
                    Channel22_P1 = responseParams[7],

                    Channel23_P_PMSE = responseParams[8],
                    Channel23_P1 = responseParams[9],

                    Channel24_P_PMSE = responseParams[10],
                    Channel24_P1 = responseParams[11],

                    Channel25_P_PMSE = responseParams[12],
                    Channel25_P1 = responseParams[13],

                    Channel26_P_PMSE = responseParams[14],
                    Channel26_P1 = responseParams[15],

                    Channel27_P_PMSE = responseParams[16],
                    Channel27_P1 = responseParams[17],

                    Channel28_P_PMSE = responseParams[18],
                    Channel28_P1 = responseParams[19],

                    Channel29_P_PMSE = responseParams[20],
                    Channel29_P1 = responseParams[21],

                    Channel30_P_PMSE = responseParams[22],
                    Channel30_P1 = responseParams[23],

                    ////----------------------31 - 40 ---------------------------------

                    Channel31_P_PMSE = responseParams[24],
                    Channel31_P1 = responseParams[25],

                    Channel32_P_PMSE = responseParams[26],
                    Channel32_P1 = responseParams[27],

                    Channel33_P_PMSE = responseParams[28],
                    Channel33_P1 = responseParams[29],

                    Channel34_P_PMSE = responseParams[30],
                    Channel34_P1 = responseParams[31],

                    Channel35_P_PMSE = responseParams[32],
                    Channel35_P1 = responseParams[33],

                    Channel36_P_PMSE = responseParams[34],
                    Channel36_P1 = responseParams[35],

                    Channel37_P_PMSE = responseParams[36],
                    Channel37_P1 = responseParams[37],

                    Channel38_P_PMSE = responseParams[38],
                    Channel38_P1 = responseParams[39],

                    Channel39_P_PMSE = responseParams[40],
                    Channel39_P1 = responseParams[41],

                    Channel40_P_PMSE = responseParams[42],
                    Channel40_P1 = responseParams[43],

                    ////----------------------41 - 50 ---------------------------------

                    Channel41_P_PMSE = responseParams[44],
                    Channel41_P1 = responseParams[45],

                    Channel42_P_PMSE = responseParams[46],
                    Channel42_P1 = responseParams[47],

                    Channel43_P_PMSE = responseParams[48],
                    Channel43_P1 = responseParams[49],

                    Channel44_P_PMSE = responseParams[50],
                    Channel44_P1 = responseParams[51],

                    Channel45_P_PMSE = responseParams[52],
                    Channel45_P1 = responseParams[53],

                    Channel46_P_PMSE = responseParams[54],
                    Channel46_P1 = responseParams[55],

                    Channel47_P_PMSE = responseParams[56],
                    Channel47_P1 = responseParams[57],

                    Channel48_P_PMSE = responseParams[58],
                    Channel48_P1 = responseParams[59],

                    Channel49_P_PMSE = responseParams[60],
                    Channel49_P1 = responseParams[61],

                    Channel50_P_PMSE = responseParams[62],
                    Channel50_P1 = responseParams[63],

                    ////----------------------51 - 60 ---------------------------------

                    Channel51_P_PMSE = responseParams[64],
                    Channel51_P1 = responseParams[65],

                    Channel52_P_PMSE = responseParams[66],
                    Channel52_P1 = responseParams[67],

                    Channel53_P_PMSE = responseParams[68],
                    Channel53_P1 = responseParams[69],

                    Channel54_P_PMSE = responseParams[70],
                    Channel54_P1 = responseParams[71],

                    Channel55_P_PMSE = responseParams[72],
                    Channel55_P1 = responseParams[73],

                    Channel56_P_PMSE = responseParams[74],
                    Channel56_P1 = responseParams[75],

                    Channel57_P_PMSE = responseParams[76],
                    Channel57_P1 = responseParams[77],

                    Channel58_P_PMSE = responseParams[78],
                    Channel58_P1 = responseParams[79],

                    Channel59_P_PMSE = responseParams[80],
                    Channel59_P1 = responseParams[81],

                    Channel60_P_PMSE = responseParams[82],
                    Channel60_P1 = responseParams[83],
                };

                this.OfcomEvaluationAuditor.Audit(AuditId.OfcomEvaluationOperationalParameters, AuditStatus.Success, default(int), "Operational parameters evaluation is successful");
                this.OfcomEvaluationLogger.Log(TraceEventType.Information, LoggingMessageId.PortalOfcomEvaluation, "Operational parameters evaluation is successful");

                return this.PartialView("OperationParametersResponsePartial", response);
            }

            this.OfcomEvaluationAuditor.Audit(AuditId.OfcomEvaluationOperationalParameters, AuditStatus.Failure, default(int), "Operational parameters evaluation is failed");
            this.OfcomEvaluationLogger.Log(TraceEventType.Error, LoggingMessageId.PortalOfcomEvaluation, "Operational parameters evaluation is failed");

            return this.PartialView("OperationParametersResponsePartial", null);
        }

        private HttpPostedFileBase ChooseFileOrNull(HttpPostedFileBase rawFile)
        {
            // case 1: there was no <input type="file" ... /> element in the post
            if (rawFile == null)
            {
                return null;
            }

            // case 2: there was an <input type="file" ... /> element in the post, but it was left blank
            if (rawFile.ContentLength == 0 && string.IsNullOrEmpty(rawFile.FileName))
            {
                return null;
            }

            // case 3: the file was posted
            return rawFile;
        }
    }
}
