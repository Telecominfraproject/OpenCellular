// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.PAWS.Service.Controllers
{
    using System;
    using System.Diagnostics;
    using System.Net.Http;
    using System.Web.Http;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;
    using Microsoft.Whitespace.PAWS.Service.ActionFilters;
    using Microsoft.Whitespace.PAWS.Service.Utilities;

    /// <summary>
    /// Represents a PawsAPI Controller.
    /// </summary>
    public class PawsApiController : ApiController
    {
        /// <summary>
        /// variable to have method name for Logger
        /// </summary>
        private string methodName = "PawsApiController.Post()";

        /// <summary>
        /// variable to have audit id
        /// </summary>
        private AuditId auditId;

        /// <summary>
        /// variable to have audit method name
        /// </summary>
        private string auditMethodName;

        /// <summary>
        /// variable to have stopwatch
        /// </summary>
        private Stopwatch stopWatch;

        /// <summary>
        /// variable to have elapsed time
        /// </summary>
        private long elapsedTime;

        /// <summary>
        /// Gets or sets IPawsBL interface.
        /// </summary>
        [Dependency]
        public IPawsBL PawsblMgr { get; set; }

        /// <summary>
        /// Gets or sets IAuditor Interface
        /// </summary>
        [Dependency]
        public IAuditor PawsAuditor { get; set; }

        /// <summary>
        /// Gets or sets ILogger Interface
        /// </summary>
        [Dependency]
        public ILogger PawsLogger { get; set; }

        /// <summary>
        /// Post method of the PawsAPIController class.
        /// </summary>
        /// <param name="httpRequest">The HTTP request.</param>
        /// <returns>response of registration</returns>
        [HttpPost]
        [ValidateLocation]
        public PawsResponse Post(HttpRequestMessage httpRequest)
        {
            PawsResponse postResponse = new PawsResponse();
            string methodId = string.Empty;
            Request requestparam = new Request();
            try
            {
                // Begin Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Enter " + this.methodName);

                // Begin elapsed time calculation
                this.stopWatch = new Stopwatch();
                this.stopWatch.Start();

                try
                {
                    requestparam = JsonSerialization.DeserializeString<Request>(httpRequest.Content.ReadAsStringAsync().Result);
                }
                catch (Exception)
                {
                    return new PawsResponse
                    {
                        JsonRpc = "2.0",
                        Error = new Result
                        {
                            Code = "-201",
                            Message = Constants.InvalidRequest
                        }
                    };
                }

                postResponse = this.Router(requestparam);

                if (postResponse.Error != null)
                {
                    postResponse.Error.Version = requestparam.Params.Version;
                }

                // Populate audit id based on method in the request
                string auditMethod;

                this.auditId = PawsUtil.GetAuditId(requestparam.Method, out auditMethod);
                this.auditMethodName = auditMethod;

                // End Audit transaction
                this.PawsAuditor.UserId = this.PawsLogger.UserId;
                this.PawsAuditor.TransactionId = this.PawsLogger.TransactionId;
                this.PawsAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                // End Elapsed Time Calculation
                this.stopWatch.Stop();
                this.elapsedTime = this.stopWatch.ElapsedMilliseconds;
                if (postResponse.Result == null && postResponse.Error.Code != null)
                {
                    this.PawsAuditor.Audit(this.auditId, AuditStatus.Failure, this.elapsedTime, this.auditMethodName + " failed");
                }
                else if (postResponse.Error == null && postResponse.Result.Type != null)
                {
                    this.PawsAuditor.Audit(this.auditId, AuditStatus.Success, this.elapsedTime, this.auditMethodName + " passed");
                }

                // End Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Exit " + this.methodName);
                return postResponse;
            }
            catch (Exception exception)
            {
                // Log transaction Failure
                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.PAWSGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.PawsAuditor.UserId = this.PawsLogger.UserId;
                this.PawsAuditor.TransactionId = this.PawsLogger.TransactionId;
                this.PawsAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                string auditMethod;

                this.auditId = PawsUtil.GetAuditId(requestparam.Method, out auditMethod);
                this.auditMethodName = auditMethod;

                // End Elapsed Time Calculation
                this.stopWatch.Stop();
                this.elapsedTime = this.stopWatch.ElapsedMilliseconds;
                this.PawsAuditor.Audit(this.auditId, AuditStatus.Failure, this.elapsedTime, this.auditMethodName + " failed");
                return new PawsResponse
                {
                    JsonRpc = "2.0",
                    Error = new Result
                    {
                        Code = "-201",
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// This method routes to the subsequent method depends on method name in the request
        /// </summary>
        /// <param name="request">Router method request</param>
        /// <returns>response of the device registration</returns>
        internal PawsResponse Router(Request request)
        {
            PawsResponse response;
            switch (request.Method)
            {
                case Constants.MethodNameRegister:
                    {
                        // Route to Register method in Paws Manager
                        response = this.PawsblMgr.Register(request.Params);
                        break;
                    }

                case Constants.MethodNameInit:
                    {
                        // Route to Initialize method in Paws Manager
                        response = this.PawsblMgr.Initialize(request.Params);
                        break;
                    }

                case Constants.MethodNameValidateDevice:
                    {
                        // Route to Validate method in Paws Manager
                        response = this.PawsblMgr.ValidateDevice(request.Params);
                        break;
                    }

                case Constants.MethodNameNotify:
                    {
                        // Route to Spectrum Usage Notify method in Paws Manager
                        response = this.PawsblMgr.NotifySpectrumUsage(request.Params);
                        break;
                    }

                case Constants.MethodNameAvailableSpectrum:
                    {
                        // Route to Available Spectrum method in Paws Manager
                        response = this.PawsblMgr.AvailableSpectrum(request.Params);
                        break;
                    }

                case Constants.MethodNameAvailableSpectrumBatch:
                    {
                        // Route to Available Spectrum Batch method in Paws Manager
                        response = this.PawsblMgr.AvailableSpectrumBatch(request.Params);
                        break;
                    }

                case Constants.MethodNameInterferenceQuery:
                    {
                        // Route to Interference Query method in Paws Manager
                        response = this.PawsblMgr.InterferenceQuery(request.Params);
                        break;
                    }

                case "":
                    {
                        return new PawsResponse
                        {
                            JsonRpc = "2.0",
                            Error = new Result
                            {
                                Code = "-32601",
                                Message = "Method name is missing"
                            }
                        };
                    }

                case null:
                    {
                        return new PawsResponse
                        {
                            JsonRpc = "2.0",
                            Error = new Result
                            {
                                Code = "-32600",
                                Message = "Invalid Request"
                            }
                        };
                    }

                default:
                    {
                        return new PawsResponse
                        {
                            JsonRpc = "2.0",
                            Error = new Result
                            {
                                Code = "-32601",
                                Message = Constants.ExceptionMessageInvalidMethod
                            }
                        };
                    }
            }

            response.Id = request.Id;
            return response;
        }
    }
}
