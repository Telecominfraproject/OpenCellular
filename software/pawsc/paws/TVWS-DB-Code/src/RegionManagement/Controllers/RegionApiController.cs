// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.RegionManagement.Controllers
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Net.Http;
    using System.Net.Http.Headers;
    using System.Text;
    using System.Web;
    using System.Web.Http;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.Storage.Table;
    using Constants = Entities.Constants;
    using OAuthProvider = Microsoft.WhiteSpaces.AzureOAuthProvider;

    /// <summary>
    /// Defines all the get/post/put/delete methods for API Controller.
    /// </summary>
    public class RegionApiController : ApiController
    {
        /// <summary>
        /// string constant to indicate anonymous user.
        /// </summary>
        private const string AnonymousUser = "anonymous";

        /// <summary>
        /// string constant to indicate admin tool.
        /// </summary>
        private const string AdminTool = "admin tool";

        /// <summary>
        /// Initializes a new instance of the <see cref="RegionApiController" /> class.
        /// </summary>
        public RegionApiController()
        {
        }

        /// <summary>
        /// Gets or sets the IDriverRegionManagement.
        /// </summary>
        [Dependency]
        public IDriverRegionManagement RegionManagementDriver { get; set; }

        /// <summary>
        /// Gets or sets IAuditor Interface
        /// </summary>
        [Dependency]
        public IAuditor RegionManagementAuditor { get; set; }

        /// <summary>
        /// Gets or sets the OAUTH authentication user manager.
        /// </summary>
        /// <value>The OAUTH authentication user manager.</value>
        [Dependency]
        public OAuthProvider.IUserManager OAuthUserManager { get; set; }

        /// <summary>
        /// Gets or sets IUserManager Interface
        /// </summary>
        [Dependency]
        public IWhitespaceUserManager UserManager { get; set; }

        /// <summary>
        /// Gets or sets ILogger Interface
        /// </summary>
        [Dependency]
        public ILogger RegionManagementLogger { get; set; }

        /// <summary>
        /// Gets or sets IPawsValidator Interface
        /// </summary>
        [Dependency]
        public IRegionManagementValidator RegionManagementValidator { get; set; }

        /// <summary>
        /// Add method of Region Management Controller
        /// </summary>
        /// <param name="requestMessage">The request message.</param>
        /// <returns>returns string.</returns>
        [HttpPost]
        [UserAuthorization(Roles = "Admin,SuperAdmin,DeviceVendor,Licensee,PortalUser")]
        public RegionManagementResponse RegisterDevice(HttpRequestMessage requestMessage)
        {
            string requestUserId = string.Empty;
            const string MethodName = "RegionApiController.RegisterDevice";
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start();

            try
            {
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter " + MethodName);

                requestUserId = this.GetUserIdFromRequest();

                var request = JsonSerialization.DeserializeString<Request>(requestMessage.Content.ReadAsStringAsync().Result);

                // Route to Add Incumbent Info method in Region Management driver
                RegionManagementResponse response = this.RegionManagementDriver.RegisterDevice(request.Params, requestUserId);

                // End Audit transaction
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                // End Elapsed Time Calculation
                stopWatch.Stop();
                if (response.Result == null && response.Error.Code != null)
                {
                    this.RegionManagementAuditor.Audit(AuditId.ManagementRegisterDevice, AuditStatus.Failure, stopWatch.ElapsedMilliseconds, MethodName + " failed");
                }
                else
                {
                    this.RegionManagementAuditor.Audit(AuditId.ManagementRegisterDevice, AuditStatus.Success, stopWatch.ElapsedMilliseconds, MethodName + " passed");
                }

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + MethodName);
                return response;
            }
            catch (Exception exception)
            {
                // Log transaction Failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                this.RegionManagementAuditor.Audit(AuditId.ManagementRegisterDevice, AuditStatus.Failure, stopWatch.ElapsedMilliseconds, MethodName + " failed");
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// Delete method of Region Management Controller
        /// </summary>
        /// <param name="requestMessage">The request message.</param>
        /// <returns>returns string.</returns>
        [HttpPost]
        [UserAuthorization(Roles = "Admin,SuperAdmin,Licensee,DeviceVendor,PortalUser")]
        public RegionManagementResponse DeleteIncumbent(HttpRequestMessage requestMessage)
        {
            string requestUserId = string.Empty;
            const string MethodName = "RegionApiController.DeleteIncumbent()";
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start();

            try
            {
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter " + MethodName);
                requestUserId = this.GetUserIdFromRequest();

                // Request input stream
                var incumbent = JsonSerialization.DeserializeString<Request>(requestMessage.Content.ReadAsStringAsync().Result);

                // Route to Delete Incumbent Info method in Region Management driver
                // Check whether the User is licensee or admin else cannot delete incumbents
                RegionManagementResponse response = this.RegionManagementDriver.DeleteIncumbentInfo(incumbent.Params);

                // End Audit transaction
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                // End Elapsed Time Calculation
                stopWatch.Stop();
                if (response.Result == null && response.Error.Code != null)
                {
                    this.RegionManagementAuditor.Audit(AuditId.ManagementDeleteIncumbentData, AuditStatus.Failure, stopWatch.ElapsedMilliseconds, MethodName + " failed");
                }
                else
                {
                    this.RegionManagementAuditor.Audit(AuditId.ManagementDeleteIncumbentData, AuditStatus.Success, stopWatch.ElapsedMilliseconds, MethodName + " passed");
                }

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + MethodName);
                return response;
            }
            catch (Exception exception)
            {
                // Log transaction Failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                this.RegionManagementAuditor.Audit(AuditId.ManagementDeleteIncumbentData, AuditStatus.Failure, stopWatch.ElapsedMilliseconds, MethodName + " failed");
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// Get method of Region Management Controller
        /// </summary>
        /// <param name="requestMessage">The request message.</param>
        /// <returns>returns string.</returns>
        [HttpPost]
        [UserAuthorization(Roles = "Admin,SuperAdmin,Licensee,DeviceVendor,PortalUser")]
        public RegionManagementResponse GetDeviceList(HttpRequestMessage requestMessage)
        {
            const string LogMethodName = "RegionManagement.GetDeviceList";
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start();

            string requestUserId = string.Empty;
            try
            {
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter " + LogMethodName);
                RegionManagementResponse resp = null;
                List<ProtectedDevice> protectedDevices = new List<ProtectedDevice>();

                requestUserId = this.GetUserIdFromRequest();
                var incumbent = JsonSerialization.DeserializeString<Request>(requestMessage.Content.ReadAsStringAsync().Result);

                // Validations
                List<string> errorMessages;

                // Validate Parameters
                if (!this.RegionManagementValidator.ValidateGetDeviceList(incumbent.Params, out errorMessages))
                {
                    return ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetDeviceListResponse, errorMessages.ToArray());
                }

                // Route to Get Incumbent Info method in Region Management driver
                protectedDevices = this.RegionManagementDriver.GetDeviceList(incumbent.Params);

                if (protectedDevices.Count != 0)
                {
                    resp = new RegionManagementResponse
                    {
                        Result = new Result
                        {
                            Type = Constants.TypeGetDeviceListResponse,
                            Message = "Device List",
                            DeviceList = protectedDevices.ToArray(),
                        }
                    };
                }
                else if (protectedDevices.Count == 0)
                {
                    resp = new RegionManagementResponse
                    {
                        Result = new Result
                        {
                            Type = Constants.TypeGetDeviceListResponse,
                            Message = Constants.ErrorMessageNoData,
                        }
                    };
                }
                else
                {
                    resp = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetDeviceListResponse, Constants.ErrorMessageServerError);
                }

                // End Audit transaction
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                // End Elapsed Time Calculation
                stopWatch.Stop();
                this.RegionManagementAuditor.Audit(AuditId.ManagementGetDeviceList, AuditStatus.Success, stopWatch.ElapsedMilliseconds, LogMethodName + " passed");

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + LogMethodName);
                return resp;
            }
            catch (Exception exception)
            {
                // Log transaction Failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                this.RegionManagementAuditor.Audit(AuditId.ManagementGetDeviceList, AuditStatus.Failure, stopWatch.ElapsedMilliseconds, LogMethodName + " failed");
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// Get method of Region Management Controller
        /// </summary>
        /// <param name="requestMessage">The request message.</param>
        /// <returns>returns string.</returns>
        [HttpPost]
        public RegionManagementResponse GetChannelList(HttpRequestMessage requestMessage)
        {
            string requestUserId = string.Empty;
            const string LogMethodName = "RegionApiController.GetChannelList";
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start();

            try
            {
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter " + LogMethodName);
                RegionManagementResponse resp = null;
                requestUserId = this.GetUserIdFromRequest();

                Request incumbent = JsonSerialization.DeserializeString<Request>(requestMessage.Content.ReadAsStringAsync().Result);

                // Validations
                List<string> errorMessages;

                // Validate Parameters
                if (!this.RegionManagementValidator.ValidateGetChannelListRequest(incumbent.Params, out errorMessages))
                {
                    return ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetChannelsResponse, errorMessages.ToArray());
                }

                // Route to get free channels method in Region Management driver
                // Check whether the User is licensee or admin else cannot delete incumbents
                var driverResponse = this.RegionManagementDriver.GetChannelList(incumbent.Params);

                if (driverResponse != null && driverResponse.Result != null)
                {
                    resp = new RegionManagementResponse
                    {
                        Result = new Result
                        {
                            Type = Constants.TypeGetChannelsResponse,
                            Message = "Free Channels List",
                            ChannelInfo = driverResponse.Result.Channels,
                            ChannelsInCSV = driverResponse.Result.ChannelsInCSV,
                            IntermediateResults1 = driverResponse.Result.IntermediateResults1,
                            MasterOperationParameters = driverResponse.Result.MasterOperationParameters,
                            UniqueId = driverResponse.Result.UniqueId,
                            RequestType = driverResponse.Result.RequestType,
                            StartTime = driverResponse.Result.StartTime,
                            EndTime = driverResponse.Result.EndTime,
                        }
                    };
                }

                // End Audit transaction
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                // End Elapsed Time Calculation
                stopWatch.Stop();
                this.RegionManagementAuditor.Audit(AuditId.ManagementGetChannelList, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, LogMethodName + " passed");

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + LogMethodName);
                return resp;
            }
            catch (Exception exception)
            {
                stopWatch.Stop();

                // Log transaction Failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                this.RegionManagementAuditor.Audit(AuditId.ManagementGetChannelList, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, LogMethodName + " failed");
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// ExcludeChannel method of Region Management Controller
        /// </summary>
        /// <param name="requestMessage">The request message.</param>
        /// <returns>returns string.</returns>
        [HttpPost]
        [UserAuthorization(Roles = "Admin,SuperAdmin")]
        public RegionManagementResponse ExcludeChannel(HttpRequestMessage requestMessage)
        {
            string requestUserId = string.Empty;
            const string MethodName = "RegionApiController.ExcludeChannel()";
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start();

            try
            {
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter " + MethodName);
                requestUserId = this.GetUserIdFromRequest();
                RegionManagementResponse response = null;

                var request = JsonSerialization.DeserializeString<Request>(requestMessage.Content.ReadAsStringAsync().Result);
                response = this.RegionManagementDriver.ExcludeChannels(request.Params);

                // End Audit transaction
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                
                stopWatch.Stop();
                if (response.Result == null && response.Error.Code != null)
                {
                    this.RegionManagementAuditor.Audit(AuditId.ManagementExcludeChannel, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " failed");
                }
                else
                {
                    this.RegionManagementAuditor.Audit(AuditId.ManagementExcludeChannel, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " passed");
                }

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + MethodName);
                return response;
            }
            catch (Exception exception)
            {
                stopWatch.Stop();
                
                // Log transaction Failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                this.RegionManagementAuditor.Audit(AuditId.ManagementExcludeChannel, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " failed");
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// ExcludeIds method of Region Management Controller
        /// </summary>
        /// <param name="requestMessage">The request message.</param>
        /// <returns>returns string.</returns>
        [HttpPost]
        [UserAuthorization(Roles = "Admin,SuperAdmin")]
        public RegionManagementResponse ExcludeIds(HttpRequestMessage requestMessage)
        {
            string requestUserId = string.Empty;
            const string LogMethodName = "RegionApiController.ExcludeIds()";
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start();
            try
            {
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter " + LogMethodName);
                requestUserId = this.GetUserIdFromRequest();

                var request = JsonSerialization.DeserializeString<Request>(requestMessage.Content.ReadAsStringAsync().Result);
                RegionManagementResponse response = this.RegionManagementDriver.ExcludeIds(request.Params);

                // End Audit transaction
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                stopWatch.Stop();
                if (response.Result == null && response.Error.Code != null)
                {
                    this.RegionManagementAuditor.Audit(AuditId.ManagementExcludeIds, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, LogMethodName + " failed");
                }
                else
                {
                    this.RegionManagementAuditor.Audit(AuditId.ManagementExcludeIds, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, LogMethodName + " passed");
                }

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + LogMethodName);
                return response;
            }
            catch (Exception exception)
            {
                stopWatch.Stop();
                
                // Log transaction Failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                this.RegionManagementAuditor.Audit(AuditId.ManagementExcludeIds, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, LogMethodName + " failed");
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// Get method of Region Management Controller
        /// </summary>
        /// <param name="requestMessage">The request message.</param>
        /// <returns>returns list.</returns>
        //// TODO: Not sure this method requires authorization ? 
        //// As this been consumed in Finder page where authorization is not mandatory
        [HttpPost]
        public RegionManagementResponse GetIncumbents(HttpRequestMessage requestMessage)
        {
            string requestUserId = string.Empty;
            const string MethodName = "RegionApiController.GetIncumbents";
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start();
            try
            {
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter " + MethodName);
                RegionManagementResponse response = null;
                requestUserId = this.GetUserIdFromRequest();

                ////ToDo uncomment when integrating AzureOAuthProvider and remove test line
                ////var userDetails = this.GetUserDetailsFromRequest();

                // Request input stream
                var request = JsonSerialization.DeserializeString<Request>(requestMessage.Content.ReadAsStringAsync().Result);

                List<string> errorMessages;

                if (request.Params == null || string.IsNullOrEmpty(request.Params.IncumbentType))
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetIncumbentsResponse, Constants.ErrorMessageIncumbentTypeRequired);
                    return response;
                }

                var incumbentType = Conversion.ToIncumbentType(request.Params.IncumbentType);
                if (incumbentType == IncumbentType.None)
                {
                    return ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetIncumbentsResponse, Constants.ErrorMessageInvalidIncumbentType);
                }

                // Validate Parameters
                if (!this.RegionManagementValidator.ValidateRequiredIncumbentType(request.Params, out errorMessages))
                {
                    return ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetIncumbentsResponse, errorMessages.ToArray());
                }

                // Route to Get Incumbent Info method in Region Management driver
                ////ToDo uncomment when integrating AzureOAuthProvider and remove test line
                ////bool isAdmin = false;
                ////foreach (var accessDetail in userDetails.AccessInfo)
                ////{
                ////    if (accessDetail.AccessLevel == Microsoft.WhiteSpaces.Common.AccessLevels.Admin || accessDetail.AccessLevel == Microsoft.WhiteSpaces.Common.AccessLevels.SuperAdmin)
                ////    {
                ////        isAdmin = true;
                ////    }
                ////}

                bool isAdmin = true;
                object[] incumbents = null;

                if (isAdmin)
                {
                    incumbents = this.RegionManagementDriver.GetIncumbents(request.Params.IncumbentType, request.Params.RegistrationId);
                }

                if (incumbents == null)
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetIncumbentsResponse, Constants.ErrorMessageNoData);
                }
                else
                {
                    response = new RegionManagementResponse
                    {
                        Result = new Result
                        {
                            Type = Constants.TypeGetIncumbentsResponse,
                            Message = "Incumbents List",
                            IncumbentList = incumbents,
                        }
                    };
                }

                // End Audit transaction
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                // End Elapsed Time Calculation
                stopWatch.Stop();
                this.RegionManagementAuditor.Audit(AuditId.ManagementGetIncumbentData, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " passed");

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + MethodName);
                return response;
            }
            catch (Exception exception)
            {
                // Log transaction Failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                this.RegionManagementAuditor.Audit(AuditId.ManagementGetIncumbentData, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " failed");
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// Gets the licensed LPAUX information.
        /// </summary>
        /// <returns>returns RegionManagementResponse.</returns>
        public RegionManagementResponse GetULSCallSigns()
        {
            const string MethodName = "RegionApiController.GetULSCallSigns()";
            string requestUserId = string.Empty;
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start();
            try
            {
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter " + MethodName);
                RegionManagementResponse response = null;
                requestUserId = this.GetUserIdFromRequest();

                var incumbents = this.RegionManagementDriver.GetLpAuxLicenseInfo("LICEN");

                if (incumbents == null)
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetULSCallSignsResponse, Constants.ErrorMessageServerError);
                }
                else if (incumbents.Count > 0)
                {
                    response = new RegionManagementResponse
                    {
                        Result = new Result
                        {
                            Type = Constants.TypeGetULSCallSignsResponse,
                            Message = "LPAUX License List",
                            LpAuxLicenses = incumbents.ToArray()
                        }
                    };
                }

                // End Audit transaction
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                // End Elapsed Time Calculation
                stopWatch.Stop();
                this.RegionManagementAuditor.Audit(AuditId.RegionManagementGetULSCallSigns, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " passed");

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + MethodName);
                return response;
            }
            catch (Exception exception)
            {
                stopWatch.Stop();
                
                // Log transaction Failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                this.RegionManagementAuditor.Audit(AuditId.RegionManagementGetULSCallSigns, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " failed");
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// Gets the licensed LPAUX information.
        /// </summary>
        /// <returns>returns RegionManagementResponse.</returns>
        public RegionManagementResponse GetULSFileNumbers()
        {
            const string MethodName = "RegionApiController.GetULSFileNumbers()";
            string requestUserId = string.Empty;
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start(); 
            try
            {
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter " + MethodName);
                RegionManagementResponse response = null;
                requestUserId = this.GetUserIdFromRequest();

                var incumbents = this.RegionManagementDriver.GetLpAuxLicenseInfo();
                if (incumbents == null || incumbents.Count == 0)
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetULSFileNumbersResponse, Constants.ErrorMessageNoData);
                }
                else if (incumbents.Count > 0)
                {
                    response = new RegionManagementResponse
                    {
                        Result = new Result
                        {
                            Type = Constants.TypeGetULSFileNumbersResponse,
                            Message = "Unlicense LPAUX ULSFileNumbers",
                            LpAuxLicenses = incumbents.ToArray()
                        }
                    };
                }

                // End Audit transaction
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                // End Elapsed Time Calculation
                stopWatch.Stop();
                this.RegionManagementAuditor.Audit(AuditId.RegionManagementGetULSFileNumbers, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " passed");

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + MethodName);
                return response;
            }
            catch (Exception exception)
            {
                stopWatch.Stop();
                
                // Log transaction Failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                this.RegionManagementAuditor.Audit(AuditId.RegionManagementGetULSFileNumbers, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " failed");
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// Searches the TV stations.
        /// </summary>
        /// <param name="requestMessage">The request message.</param>
        /// <returns>returns RegionManagementResponse.</returns>
        public RegionManagementResponse SearchMvpdCallSigns(HttpRequestMessage requestMessage)
        {
            const string MethodName = "RegionApiController.SearchMvpdCallSigns()";
            string requestUserId = string.Empty;
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start(); 
            try
            {
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter " + MethodName);
                RegionManagementResponse response = null;
                requestUserId = this.GetUserIdFromRequest();
                var request = JsonSerialization.DeserializeString<Request>(requestMessage.Content.ReadAsStringAsync().Result);

                var resp = this.RegionManagementDriver.SearchMVPDCallSigns(request.Params);

                if (resp.Result != null)
                {
                    response = new RegionManagementResponse
                    {
                        Result = new Result
                        {
                            Type = Constants.TypeGetSearchMVPDCallSignsResponse,
                            Message = "MvpdCallSigns",
                            SearchMVPDCallSigns = resp.Result.SearchMVPDCallSigns
                        }
                    };
                }
                else
                {
                    return resp;
                }

                // End Audit transaction
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                // End Elapsed Time Calculation
                stopWatch.Stop();
                this.RegionManagementAuditor.Audit(AuditId.RegionManagementSearchMVPDCallSigns, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " passed");

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + MethodName);
                return response;
            }
            catch (Exception exception)
            {
                stopWatch.Stop();
                
                // Log transaction Failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                this.RegionManagementAuditor.Audit(AuditId.RegionManagementSearchMVPDCallSigns, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " failed");
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// Gets MVPD Callsign information
        /// </summary>
        /// <param name="requestedCallsign">callsign</param>
        /// <returns>returns callsign information</returns>
       public RegionManagementResponse GetMVPDCallSignInfo(string callsign)
        {
            const string MethodName = "RegionApiController.GetMVPDCallSignInfo()";
            string requestUserId = string.Empty;
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start();
            try
            {
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter " + MethodName);
                requestUserId = this.GetUserIdFromRequest();               

                RegionManagementResponse response  = this.RegionManagementDriver.GetMVPDCallSignInfo(callsign);                

                // End Audit transaction
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                // End Elapsed Time Calculation
                stopWatch.Stop();
                this.RegionManagementAuditor.Audit(AuditId.RegionMangamentGetCallsignInfo, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " passed");

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + MethodName);
                return response;
            }
            catch (Exception exception)
            {
                stopWatch.Stop();

                // Log transaction Failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                this.RegionManagementAuditor.Audit(AuditId.RegionMangamentGetCallsignInfo, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " failed");
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// Get method of Region Management Controller
        /// </summary>
        /// <param name="requestMessage">The request message.</param>
        /// <returns>returns list.</returns>
        [HttpPost]
        public RegionManagementResponse GetPublicDataWithEvents(HttpRequestMessage requestMessage)
        {
            string requestUserId = string.Empty;
            const string MethodName = "RegionApiController.GetPublicDataWithEvents()";
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start();
            try
            {
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter " + MethodName);
                RegionManagementResponse response = null;

                requestUserId = this.GetUserIdFromRequest();

                Request request = JsonSerialization.DeserializeString<Request>(requestMessage.Content.ReadAsStringAsync().Result);

                if (request.Params == null || string.IsNullOrEmpty(request.Params.IncumbentType))
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetPublicDataWithEventsResponse, Constants.ErrorMessageIncumbentTypeRequired);
                    return response;
                }

                var incumbents = this.RegionManagementDriver.GetProtectedEntityWithEvents(request.Params.IncumbentType);

                if (incumbents == null)
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetPublicDataWithEventsResponse, Constants.ErrorMessageWrongIncumbentType);
                }
                else if (incumbents.Count > 0)
                {
                    response = new RegionManagementResponse
                    {
                        Result = new Result
                        {
                            Type = Constants.TypeGetPublicDataWithEventsResponse,
                            Message = "Incumbents List",
                            IncumbentList = incumbents.Cast<object>().ToArray(),
                        }
                    };
                }
                
                // End Audit transaction
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                // End Elapsed Time Calculation
                stopWatch.Stop();
                this.RegionManagementAuditor.Audit(AuditId.RegionManagementGetPublicDataWithEvents, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " passed");

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + MethodName);
                return response;
            }
            catch (Exception exception)
            {
                stopWatch.Stop();

                // Log transaction Failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                this.RegionManagementAuditor.Audit(AuditId.RegionManagementGetPublicDataWithEvents, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " failed");
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// Get method of Region Management Controller
        /// </summary>
        /// <param name="requestMessage">The request message.</param>
        /// <returns>returns list.</returns>
        [HttpPost]
        public RegionManagementResponse GetPublicData(HttpRequestMessage requestMessage)
        {
            string requestUserId = string.Empty;
            const string MethodName = "RegionApiController.GetPublicData()";
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start();
            try
            {
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter " + MethodName);
                RegionManagementResponse response = null;

                requestUserId = this.GetUserIdFromRequest();

                Request request = JsonSerialization.DeserializeString<Request>(requestMessage.Content.ReadAsStringAsync().Result);

                if (request.Params == null || string.IsNullOrEmpty(request.Params.IncumbentType))
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetPublicDataResponse, Constants.ErrorMessageIncumbentTypeRequired);
                    return response;
                }

                var incumbents = this.RegionManagementDriver.GetProtectedEntity(request.Params.IncumbentType);

                if (incumbents == null)
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetPublicDataResponse, Constants.ErrorMessageWrongIncumbentType);
                }
                else if (incumbents.Count > 0)
                {
                    response = new RegionManagementResponse
                    {
                        Result = new Result
                        {
                            Type = Constants.TypeGetPublicDataResponse,
                            Message = "Incumbents List",
                            IncumbentList = incumbents.Cast<object>().ToArray(),
                        }
                    };
                }
                else
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetPublicDataResponse, Constants.ErrorMessageNoData);
                }

                // End Audit transaction
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                // End Elapsed Time Calculation
                stopWatch.Stop();
                this.RegionManagementAuditor.Audit(AuditId.PublicData, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " passed");

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + MethodName);
                return response;
            }
            catch (Exception exception)
            {
                // Log transaction Failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                this.RegionManagementAuditor.Audit(AuditId.PublicData, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " failed");
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// Gets the authorized device models.
        /// </summary>
        /// <returns>returns RegionManagementResponse.</returns>
        [HttpGet]
        public RegionManagementResponse GetAuthorizedDeviceModels()
        {
            string requestUserId = string.Empty;
            const string MethodName = "RegionApiController.GetAuthorizedDeviceModels()";
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start();
            try
            {
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter " + MethodName);

                RegionManagementResponse response = null;
                requestUserId = this.GetUserIdFromRequest();

                var deviceRecords = this.RegionManagementDriver.GetAuthorizedDevices();

                if (deviceRecords == null || deviceRecords.Count == 0)
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetAuthorizedDevicesInfo, Constants.ErrorMessageNoData);
                }
                else if (deviceRecords.Count != 0)
                {
                    response = new RegionManagementResponse
                    {
                        Result = new Result
                        {
                            Type = Constants.TypeGetAuthorizedDevicesInfo,
                            Message = "Authorized Device List",
                            IncumbentList = deviceRecords.Cast<object>().ToArray()
                        }
                    };
                }

                // End Audit transaction
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                // End Elapsed Time Calculation
                stopWatch.Stop();
                this.RegionManagementAuditor.Audit(AuditId.RegionManagementGetAuthorizedDevicesInfo, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " passed");

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + MethodName);
                return response;
            }
            catch (Exception exception)
            {
                // Log transaction Failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                this.RegionManagementAuditor.Audit(AuditId.ManagementGetIncumbentData, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " failed");
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// Gets the contour data.
        /// </summary>
        /// <param name="requestMessage">The request message.</param>
        /// <returns>returns RegionManagementResponse.</returns>
        [HttpPost]
        public RegionManagementResponse GetContourData(HttpRequestMessage requestMessage)
        {
            string requestUserId = string.Empty;
            const string MethodName = "RegionApiController.GetContourData";
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start();
            try
            {
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter " + MethodName);
                RegionManagementResponse response = null;

                requestUserId = this.GetUserIdFromRequest();

                Request request = JsonSerialization.DeserializeString<Request>(requestMessage.Content.ReadAsStringAsync().Result);
                List<string> errorMessages = null;

                if (!this.RegionManagementValidator.ValidateGetContourDataRequest(request.Params, out errorMessages))
                {
                    return ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetContourDataResponse, errorMessages.ToArray());
                }

                var contourData = this.RegionManagementDriver.GetContourData(request.Params);

                if (contourData == null)
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetContourDataResponse, Constants.ErrorMessageContourCallSignNotFound);
                }
                else if (contourData.Count > 0)
                {
                    response = new RegionManagementResponse
                    {
                        Result = new Result
                        {
                            Type = Constants.TypeGetContourDataResponse,
                            Message = "Contour Points",
                            ContourData = contourData.ToArray(),
                        }
                    };
                }
                else
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetContourDataResponse, Constants.ErrorMessageNoData);
                }

                // End Audit transaction
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

                // End Elapsed Time Calculation
                stopWatch.Stop();
                this.RegionManagementAuditor.Audit(AuditId.RegionManagementGetContourData, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " passed");

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + MethodName);
                return response;
            }
            catch (Exception exception)
            {
                // Log transaction Failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

                // Audit transaction Failure
                this.RegionManagementAuditor.UserId = requestUserId;
                this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
                this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
                this.RegionManagementAuditor.Audit(AuditId.RegionManagementGetContourData, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, MethodName + " failed");
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Message = exception.Message
                    }
                };
            }
        }

        /// <summary>
        /// Add method of Region Management Controller
        /// </summary>
        /// <returns>returns RegionManagementResponse.</returns>
        [HttpPost]
        [UserAuthorization(Roles = "Licensee,Admin,SuperAdmin")]
        public RegionManagementResponse AddUser()
        {
            RegionManagementResponse response = null;
            return response;
            ////string requestUserId = string.Empty;
            ////try
            ////{
            ////    // Begin Log transaction
            ////    this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter RegionApiController.AddUser()");

            ////    // Begin elapsed time calculation
            ////    this.stopWatch = new Stopwatch();
            ////    this.stopWatch.Start();

            ////    // Request input stream
            ////    var request = HttpContext.Current.Request;
            ////    var bytes = new byte[request.InputStream.Length];
            ////    request.InputStream.Read(bytes, 0, bytes.Length);
            ////    request.InputStream.Position = 0;
            ////    string jsonRequest = Encoding.ASCII.GetString(bytes);
            ////    User userDetails = JsonSerialization.DeserializeString<User>(jsonRequest);

            ////    // Populate Audit fields
            ////    this.auditId = AuditId.ManagementRegisterUser;
            ////    this.auditMethodName = this.addUserAuditMethodName;

            ////    requestUserId = this.GetUserIdFromRequest();

            ////    userDetails.CreateBy = requestUserId;
            ////    userDetails.CreationDate = DateTime.Now;
            ////    userDetails.UpdatedBy = requestUserId;
            ////    userDetails.UpdatedOn = DateTime.Now;
            ////    List<string> errorMessages;

            ////    //// Validate User object
            ////    if (!this.UserManager.IsValid(userDetails, out errorMessages))
            ////    {
            ////        return ErrorHelper.CreateRegionErrorResponse(Constants.TypeAddUsersResponse, errorMessages.ToArray());
            ////    }

            ////    //// Validate Region Access object if exist
            ////    if (userDetails.Access != null)
            ////    {
            ////        foreach (RegionAccess access in userDetails.Access)
            ////        {
            ////            if (!this.UserManager.IsValid(access, out errorMessages) || access.AccessLevel == AccessLevel.None)
            ////            {
            ////                return ErrorHelper.CreateRegionErrorResponse(Constants.TypeAddUsersResponse, errorMessages.ToArray());
            ////            }
            ////        }
            ////    }

            ////    string resp = this.UserManager.AddUser(userDetails);
            ////    RegionManagementResponse response = null;

            ////    if (resp != null)
            ////    {
            ////        response = new RegionManagementResponse
            ////        {
            ////            Result = new Result
            ////            {
            ////                Type = Constants.TypeAddUsersResponse,
            ////                Message = resp,
            ////            }
            ////        };
            ////    }
            ////    else
            ////    {
            ////        response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeAddUsersResponse, Constants.ErrorMessageServerError);
            ////    }

            ////    // End Audit transaction
            ////    this.RegionManagementAuditor.UserId = requestUserId;
            ////    this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            ////    this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

            ////    // End Elapsed Time Calculation
            ////    stopWatch.Stop();
            ////    (int)stopWatch.Elapsed.TotalMilliseconds = (int)stopWatch.Elapsed.TotalMilliseconds;
            ////    if (response.Result == null && response.Error.Code != null)
            ////    {
            ////        this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " failed");
            ////    }
            ////    else if (response.Error == null && response.Result.Type != null)
            ////    {
            ////        this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " passed");
            ////    }

            ////    // End Log transaction
            ////    this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + "RegionApiController.AddUser()");
            ////    return response;
            ////}
            ////catch (Exception exception)
            ////{
            ////    // Log transaction Failure
            ////    this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

            ////    // Audit transaction Failure
            ////    this.RegionManagementAuditor.UserId = requestUserId;
            ////    this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            ////    this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
            ////    this.auditId = AuditId.ManagementRegisterUser;
            ////    this.auditMethodName = this.addUserAuditMethodName;
            ////    this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " failed");
            ////    return new RegionManagementResponse
            ////    {
            ////        Error = new Result
            ////        {
            ////            Type = Constants.TypeAddUsersResponse,
            ////            Message = exception.Message
            ////        }
            ////    };
            ////}
        }

        /// <summary>
        /// Add method of Region Management Controller
        /// </summary>
        /// <param name="userId">The user identifier.</param>
        /// <returns>returns string</returns>
        [HttpGet]
        [UserAuthorization(Roles = "Licensee,Admin,SuperAdmin")]
        public RegionManagementResponse GetUser(string userId)
        {
            RegionManagementResponse response = null;
            return response;
            ////RegionManagementResponse resp = null;
            ////try
            ////{
            ////    // Begin Log transaction
            ////    this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter RegionApiController.GetUser()");

            ////    // Begin elapsed time calculation
            ////    this.stopWatch = new Stopwatch();
            ////    this.stopWatch.Start();

            ////    // Populate Audit fields
            ////    this.auditId = AuditId.ManagementGetUserInformation;
            ////    this.auditMethodName = this.getUserAuditMethodName;

            ////    if (userId == null)
            ////    {
            ////        return ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetUsersResponse, Constants.ErrorMessageUserIdRequired);
            ////    }

            ////    // Route to Add Incumbent Info method in Region Management driver
            ////    User response = this.UserManager.GetUser(userId);
            ////    if (response != null)
            ////    {
            ////        resp = new RegionManagementResponse
            ////        {
            ////            Result = new Result
            ////            {
            ////                Type = Constants.TypeGetUsersResponse,
            ////                Message = "Users",
            ////                User = response,
            ////            }
            ////        };
            ////    }
            ////    else if (response == null)
            ////    {
            ////        resp = new RegionManagementResponse
            ////        {
            ////            Result = new Result
            ////            {
            ////                Type = Constants.TypeGetUsersResponse,
            ////                Message = Constants.ErrorMessageNoData,
            ////            }
            ////        };
            ////    }

            ////    // End Audit transaction
            ////    this.RegionManagementAuditor.UserId = this.GetUserIdFromRequest();
            ////    this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            ////    this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

            ////    // End Elapsed Time Calculation
            ////    stopWatch.Stop();
            ////    (int)stopWatch.Elapsed.TotalMilliseconds = (int)stopWatch.Elapsed.TotalMilliseconds;
            ////    if (resp.Result == null && resp.Error.Code != null)
            ////    {
            ////        this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " failed");
            ////    }
            ////    else if (resp.Error == null && resp.Result.Type != null)
            ////    {
            ////        this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " passed");
            ////    }

            ////    this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + "RegionApiController.GetUser()");
            ////    return resp;
            ////}
            ////catch (Exception exception)
            ////{
            ////    // Log transaction Failure
            ////    this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

            ////    // Audit transaction Failure
            ////    this.RegionManagementAuditor.UserId = userId;
            ////    this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            ////    this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
            ////    this.auditId = AuditId.ManagementGetUserInformation;
            ////    this.auditMethodName = this.getUserAuditMethodName;
            ////    this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " failed");
            ////    return new RegionManagementResponse
            ////    {
            ////        Error = new Result
            ////        {
            ////            Type = Constants.TypeGetUsersResponse,
            ////            Message = exception.Message
            ////        }
            ////    };
            ////}
        }

        /// <summary>
        /// Add method of Region Management Controller
        /// </summary>
        /// <param name="userId">The user identifier.</param>
        /// <returns>returns RegionManagementResponse</returns>
        [HttpDelete]
        [UserAuthorization(Roles = "Admin,SuperAdmin")]
        public RegionManagementResponse DeleteUser(string userId)
        {
            RegionManagementResponse response = null;
            return response;
            ////RegionManagementResponse response = null;
            ////try
            ////{
            ////    // Begin Log transaction
            ////    this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter RegionApiController.DeleteUser()");

            ////    // Begin elapsed time calculation
            ////    this.stopWatch = new Stopwatch();
            ////    this.stopWatch.Start();

            ////    // Populate Audit fields
            ////    this.auditId = AuditId.ManagementDeleteUser;
            ////    this.auditMethodName = this.deleteUserAuditMethodName;

            ////    string authorizedUserId = this.GetUserIdFromRequest();

            ////    if (userId == null)
            ////    {
            ////        return ErrorHelper.CreateRegionErrorResponse(Constants.TypeDeleteUsersResponse, Constants.ErrorMessageUserIdRequired);
            ////    }

            ////    // Route to Add Incumbent Info method in Region Management driver
            ////    bool resp = this.UserManager.DeleteUser(userId, authorizedUserId);

            ////    if (resp)
            ////    {
            ////        response = new RegionManagementResponse
            ////        {
            ////            Result = new Result
            ////            {
            ////                Type = Constants.TypeDeleteUsersResponse,
            ////                Message = "User Deleted Successfully",
            ////            }
            ////        };
            ////    }
            ////    else
            ////    {
            ////        response = new RegionManagementResponse
            ////       {
            ////           Result = new Result
            ////           {
            ////               Type = Constants.TypeDeleteUsersResponse,
            ////               Message = "User not found",
            ////           }
            ////       };
            ////    }

            ////    // End Audit transaction
            ////    this.RegionManagementAuditor.UserId = authorizedUserId;
            ////    this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            ////    this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

            ////    // End Elapsed Time Calculation
            ////    stopWatch.Stop();
            ////    (int)stopWatch.Elapsed.TotalMilliseconds = (int)stopWatch.Elapsed.TotalMilliseconds;
            ////    if (response.Result == null && response.Error.Code != null)
            ////    {
            ////        this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " failed");
            ////    }
            ////    else if (response.Error == null && response.Result.Type != null)
            ////    {
            ////        this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " passed");
            ////    }

            ////    // End Log transaction
            ////    this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + "RegionApiController.DeleteUser()");
            ////    return response;
            ////}
            ////catch (Exception exception)
            ////{
            ////    // Log transaction Failure
            ////    this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

            ////    // Audit transaction Failure
            ////    this.RegionManagementAuditor.UserId = userId;
            ////    this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            ////    this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
            ////    this.auditId = AuditId.ManagementDeleteUser;
            ////    this.auditMethodName = this.deleteUserAuditMethodName;
            ////    this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " failed");
            ////    return new RegionManagementResponse
            ////    {
            ////        Error = new Result
            ////        {
            ////            Type = Constants.TypeDeleteUsersResponse,
            ////            Message = exception.Message
            ////        }
            ////    };
            ////}
        }

        /// <summary>
        /// Add method of Region Management Controller
        /// </summary>
        /// <returns>returns RegionManagementResponse.</returns>
        [HttpPut]
        [UserAuthorization(Roles = "Admin,SuperAdmin")]
        public RegionManagementResponse UpdateUser()
        {
            RegionManagementResponse response = null;
            return response;
            ////RegionManagementResponse response = null;
            ////try
            ////{
            ////    // Begin Log transaction
            ////    this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter RegionApiController.UpdateUser()");

            ////    // Begin elapsed time calculation
            ////    this.stopWatch = new Stopwatch();
            ////    this.stopWatch.Start();

            ////    // Request input stream
            ////    var request = HttpContext.Current.Request;
            ////    var bytes = new byte[request.InputStream.Length];
            ////    request.InputStream.Read(bytes, 0, bytes.Length);
            ////    request.InputStream.Position = 0;
            ////    string jsonRequest = Encoding.ASCII.GetString(bytes);
            ////    User user = JsonSerialization.DeserializeString<User>(jsonRequest);
            ////    string[] authorizedUserId = HttpContext.Current.Request.Headers.GetValues("Authorization").ToArray();

            ////    user.UpdatedBy = authorizedUserId[0];
            ////    user.UpdatedOn = DateTime.Now;

            ////    this.auditId = AuditId.ManagementGetUserInformation;
            ////    this.auditMethodName = this.updateUserAuditMethodName;

            ////    List<string> errorMessages;

            ////    //// Validate User object
            ////    if (!this.UserManager.IsValid(user, out errorMessages))
            ////    {
            ////        return ErrorHelper.CreateRegionErrorResponse(Constants.TypeUpdateUsersResponse, errorMessages.ToArray());
            ////    }

            ////    //// Validate Region Access object if exist
            ////    if (user.Access != null)
            ////    {
            ////        foreach (RegionAccess access in user.Access)
            ////        {
            ////            if (!this.UserManager.IsValid(access, out errorMessages) || access.AccessLevel == AccessLevel.None)
            ////            {
            ////                return ErrorHelper.CreateRegionErrorResponse(Constants.TypeUpdateUsersResponse, errorMessages.ToArray());
            ////            }
            ////        }
            ////    }

            ////    // Route to Add Incumbent Info method in Region Management driver
            ////    string resp = this.UserManager.UpdateUser(user);
            ////    if (resp != null)
            ////    {
            ////        response = new RegionManagementResponse
            ////        {
            ////            Result = new Result
            ////            {
            ////                Type = Constants.TypeUpdateUsersResponse,
            ////                Message = "User updated successfully",
            ////            }
            ////        };
            ////    }
            ////    else
            ////    {
            ////        response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeUpdateUsersResponse, "Records not found");
            ////    }

            ////    // End Audit transaction
            ////    this.RegionManagementAuditor.UserId = authorizedUserId[0];
            ////    this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            ////    this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

            ////    // End Elapsed Time Calculation
            ////    stopWatch.Stop();
            ////    (int)stopWatch.Elapsed.TotalMilliseconds = (int)stopWatch.Elapsed.TotalMilliseconds;
            ////    if (response.Result == null && response.Error.Code != null)
            ////    {
            ////        this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " failed");
            ////    }
            ////    else if (response.Error == null && response.Result.Type != null)
            ////    {
            ////        this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " passed");
            ////    }

            ////    // End Log transaction
            ////    this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + "RegionApiController.UpdateUser()");
            ////    return response;
            ////}
            ////catch (Exception exception)
            ////{
            ////    // Log transaction Failure
            ////    this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

            ////    // Audit transaction Failure
            ////    this.RegionManagementAuditor.UserId = HttpContext.Current.Request.Headers.GetValues("Authorization")[0];
            ////    this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            ////    this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
            ////    this.auditId = AuditId.ManagementUpdateUser;
            ////    this.auditMethodName = this.updateUserAuditMethodName;
            ////    this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " failed");
            ////    return new RegionManagementResponse
            ////    {
            ////        Error = new Result
            ////        {
            ////            Type = Constants.TypeUpdateUsersResponse,
            ////            Message = exception.Message
            ////        }
            ////    };
            ////}
        }

        /// <summary>
        /// Add method of Region Management Controller
        /// </summary>
        /// <returns>returns RegionManagementResponse.</returns>
        [HttpPost]
        [UserAuthorization(Roles = "Admin,SuperAdmin")]
        public RegionManagementResponse RequestElevatedAccess()
        {
            RegionManagementResponse response = null;
            return response;
            ////string methodId = string.Empty;
            ////try
            ////{
            ////    // Begin Log transaction
            ////    this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter RegionApiController.RequestElevationAccess()");

            ////    // Begin elapsed time calculation
            ////    this.stopWatch = new Stopwatch();
            ////    this.stopWatch.Start();

            ////    // Request input stream
            ////    var request = HttpContext.Current.Request;
            ////    var bytes = new byte[request.InputStream.Length];
            ////    request.InputStream.Read(bytes, 0, bytes.Length);
            ////    request.InputStream.Position = 0;
            ////    string jsonRequest = Encoding.ASCII.GetString(bytes);
            ////    ElevatedAccessRequest userElevatedAccessRequest = JsonSerialization.DeserializeString<ElevatedAccessRequest>(jsonRequest);
            ////    List<string> errorMessages;

            ////    //// Validate User object
            ////    if (!this.UserManager.IsValid(userElevatedAccessRequest, out errorMessages))
            ////    {
            ////        return ErrorHelper.CreateRegionErrorResponse(Constants.TypeRequestElevatedResponse, errorMessages.ToArray());
            ////    }

            ////    string[] authorizedUserId = HttpContext.Current.Request.Headers.GetValues("Authorization").ToArray();
            ////    userElevatedAccessRequest.UpdatedBy = authorizedUserId[0];
            ////    userElevatedAccessRequest.UpdatedOn = DateTime.Now;

            ////    // Populate Audit fields
            ////    this.auditId = AuditId.ManagementRegisterUser;
            ////    this.auditMethodName = this.requestElevatedAccessAuditMethodName;

            ////    // Route to Add Incumbent Info method in Region Management driver
            ////    string resp = this.UserManager.RequestElevatedAccess(userElevatedAccessRequest);
            ////    RegionManagementResponse response = null;

            ////    if (resp != null)
            ////    {
            ////        response = new RegionManagementResponse
            ////        {
            ////            Result = new Result
            ////            {
            ////                Type = Constants.TypeRequestElevatedResponse,
            ////                Message = "Request for Elevated Access level is stored.",
            ////            }
            ////        };
            ////    }
            ////    else
            ////    {
            ////        response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeRequestElevatedResponse, Constants.ErrorMessageServerError);
            ////    }

            ////    // End Audit transaction
            ////    this.RegionManagementAuditor.UserId = authorizedUserId[0];
            ////    this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            ////    this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

            ////    // End Elapsed Time Calculation
            ////    stopWatch.Stop();
            ////    (int)stopWatch.Elapsed.TotalMilliseconds = (int)stopWatch.Elapsed.TotalMilliseconds;
            ////    if (response.Result == null && response.Error.Code != null)
            ////    {
            ////        this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " failed");
            ////    }
            ////    else if (response.Error == null && response.Result.Type != null)
            ////    {
            ////        this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " passed");
            ////    }

            ////    // End Log transaction
            ////    this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + "RegionApiController.AddUser()");
            ////    return response;
            ////}
            ////catch (Exception exception)
            ////{
            ////    // Log transaction Failure
            ////    this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

            ////    // Audit transaction Failure
            ////    this.RegionManagementAuditor.UserId = HttpContext.Current.Request.Headers.GetValues("Authorization")[0];
            ////    this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            ////    this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
            ////    this.auditId = AuditId.ManagementRegisterUser;
            ////    this.auditMethodName = this.addUserAuditMethodName;
            ////    this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " failed");
            ////    return new RegionManagementResponse
            ////    {
            ////        Error = new Result
            ////        {
            ////            Type = Constants.TypeRequestElevatedResponse,
            ////            Message = exception.Message
            ////        }
            ////    };
            ////}
        }

        /// <summary>
        /// Add method of Region Management Controller
        /// </summary>
        /// <returns>returns RegionManagementResponse.</returns>
        [HttpPost]
        [UserAuthorization(Roles = "Admin,SuperAdmin")]
        public RegionManagementResponse GrantAccess()
        {
            RegionManagementResponse response = null;
            return response;
            ////string methodId = string.Empty;
            ////try
            ////{
            ////    // Begin Log transaction
            ////    this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Enter RegionApiController.GrantAccess()");

            ////    // Begin elapsed time calculation
            ////    this.stopWatch = new Stopwatch();
            ////    this.stopWatch.Start();

            ////    // Request input stream
            ////    var request = HttpContext.Current.Request;
            ////    var bytes = new byte[request.InputStream.Length];
            ////    request.InputStream.Read(bytes, 0, bytes.Length);
            ////    request.InputStream.Position = 0;
            ////    string jsonRequest = Encoding.ASCII.GetString(bytes);
            ////    ElevatedAccessRequest userElevatedAccessRequest = JsonSerialization.DeserializeString<ElevatedAccessRequest>(jsonRequest);

            ////    List<string> errorMessages;

            ////    //// Validate User object
            ////    if (!this.UserManager.IsValid(userElevatedAccessRequest, out errorMessages))
            ////    {
            ////        return ErrorHelper.CreateRegionErrorResponse(Constants.TypeGrantAccessResponse, errorMessages.ToArray());
            ////    }

            ////    // Populate Audit fields
            ////    this.auditId = AuditId.ManagementRegisterUser;
            ////    this.auditMethodName = this.grantAccessAuditMethodName;
            ////    string[] authorizedUserId = HttpContext.Current.Request.Headers.GetValues("Authorization").ToArray();

            ////    // Route to Add Incumbent Info method in Region Management driver
            ////    string resp = this.UserManager.GrantAccess(userElevatedAccessRequest, authorizedUserId[0]);
            ////    RegionManagementResponse response = null;

            ////    if (resp != null)
            ////    {
            ////        response = new RegionManagementResponse
            ////        {
            ////            Result = new Result
            ////            {
            ////                Type = Constants.TypeGrantAccessResponse,
            ////                Message = "Request for Grant Access level is stored.",
            ////            }
            ////        };
            ////    }
            ////    else
            ////    {
            ////        response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGrantAccessResponse, Constants.ErrorMessageServerError);
            ////    }

            ////    // End Audit transaction
            ////    this.RegionManagementAuditor.UserId = authorizedUserId[0];
            ////    this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            ////    this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

            ////    // End Elapsed Time Calculation
            ////    stopWatch.Stop();
            ////    (int)stopWatch.Elapsed.TotalMilliseconds = (int)stopWatch.Elapsed.TotalMilliseconds;
            ////    if (response.Result == null && response.Error.Code != null)
            ////    {
            ////        this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " failed");
            ////    }
            ////    else if (response.Error == null && response.Result.Type != null)
            ////    {
            ////        this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " passed");
            ////    }

            ////    // End Log transaction
            ////    this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.RegionManagementGenericMessage, "Exit " + "RegionApiController.AddUser()");
            ////    return response;
            ////}
            ////catch (Exception exception)
            ////{
            ////    // Log transaction Failure
            ////    this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.RegionManagementGenericMessage, exception.ToString());

            ////    // Audit transaction Failure
            ////    this.RegionManagementAuditor.UserId = HttpContext.Current.Request.Headers.GetValues("Authorization")[0];
            ////    this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            ////    this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;
            ////    this.auditId = AuditId.ManagementRegisterUser;
            ////    this.auditMethodName = this.addUserAuditMethodName;
            ////    this.RegionManagementAuditor.Audit(this.auditId, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, this.auditMethodName + " failed");
            ////    return new RegionManagementResponse
            ////    {
            ////        Error = new Result
            ////        {
            ////            Type = Constants.TypeGrantAccessResponse,
            ////            Message = exception.Message
            ////        }
            ////    };
            ////}
        }

        /////// <summary>
        /////// Cancels the registration.
        /////// </summary>
        /////// <returns>returns PMSEResponse.</returns>
        ////[HttpDelete]
        ////[UserAuthorization(Roles = "Licensee,Admin,SuperAdmin")]
        ////public RegionManagementResponse CancelRegistration()
        ////{
        ////    string methodId = string.Empty;
        ////    Request requestparam = null;
        ////    RegionManagementResponse response = new RegionManagementResponse();
        ////    string userId = this.GetUserIdFromRequest();
        ////    try
        ////    {
        ////        // Begin Log transaction
        ////        this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.PMSEGenericMessage, "Enter:" + this.cancelRegistrationMethodName);

        ////        // Begin elapsed time calculation
        ////        this.stopWatch = new Stopwatch();
        ////        this.stopWatch.Start();

        ////        // Request input stream
        ////        var request = HttpContext.Current.Request;
        ////        var bytes = new byte[request.InputStream.Length];
        ////        request.InputStream.Read(bytes, 0, bytes.Length);
        ////        request.InputStream.Position = 0;
        ////        string jsonRequest = Encoding.ASCII.GetString(bytes);
        ////        requestparam = JsonSerialization.DeserializeString<Request>(jsonRequest);

        ////        response = this.RegionManagementDriver.CancelRegistration(requestparam.Params);

        ////        // End Log transaction
        ////        this.RegionManagementAuditor.UserId = userId;
        ////        this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
        ////        this.RegionManagementAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

        ////        // End Elapsed Time Calculation
        ////        stopWatch.Stop();
        ////        (int)stopWatch.Elapsed.TotalMilliseconds = (int)stopWatch.Elapsed.TotalMilliseconds;
        ////        if (response.Result == null && response.Error != null)
        ////        {
        ////            this.RegionManagementAuditor.Audit(AuditId.PMSECancelRegistrationRequest, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, this.cancelRegistrationMethodName + " failed");
        ////        }
        ////        else if (response.Error == null && response.Result != null)
        ////        {
        ////            this.RegionManagementAuditor.Audit(AuditId.PMSECancelRegistrationRequest, AuditStatus.Success, (int)stopWatch.Elapsed.TotalMilliseconds, this.cancelRegistrationMethodName + " passed");
        ////        }

        ////        this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.PMSEGenericMessage, "Exit:" + this.cancelRegistrationMethodName);
        ////    }
        ////    catch (Exception exception)
        ////    {
        ////        // Log transaction Failure
        ////        this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.PMSEGenericMessage, exception.ToString());

        ////        // Audit transaction Failure
        ////        response = new RegionManagementResponse();
        ////        response = ErrorHelper.CreateRegionErrorResponse(Constants.LPAuxRegistration, Constants.ErrorMessageServerError);
        ////        this.RegionManagementAuditor.Audit(AuditId.PMSECancelRegistrationRequest, AuditStatus.Failure, (int)stopWatch.Elapsed.TotalMilliseconds, this.cancelRegistrationMethodName + " failed");
        ////    }

        ////    return response;
        ////}

        /// <summary>
        /// Gets the user identifier from request.
        /// </summary>
        /// <returns>returns userId.</returns>
        private string GetUserIdFromRequest()
        {
            var userDetails = this.GetUserDetailsFromRequest();

            if (userDetails == null && this.IsRequestFromAdminUserAgent())
            {
                return RegionApiController.AdminTool;
            }

            return userDetails == null ? RegionApiController.AnonymousUser : userDetails.MembershipInfo.UserId.ToString();
        }

        /// <summary>
        /// Gets the user details from request.
        /// </summary>
        /// <returns>returns System.String.</returns>
        private OAuthProvider.UserDetails GetUserDetailsFromRequest()
        {
            AuthenticationHeaderValue authorizationHeaderValue = this.Request.Headers.Authorization;

            if (authorizationHeaderValue != null)
            {
                string accesstoken = authorizationHeaderValue.Parameter;
                return this.OAuthUserManager.GetUserDetailsByAccessToken(accesstoken);
            }

            return null;
        }

        /// <summary>
        /// Validates if the current request is from admin tool
        /// </summary>
        /// <returns>Boolean value indicating is request from admin tool or not.</returns>
        private bool IsRequestFromAdminUserAgent()
        {
            if (string.IsNullOrWhiteSpace(UnityMvcActivator.AdminToolProductToken))
            {
                return false;
            }

            HttpHeaderValueCollection<ProductInfoHeaderValue> userAgents = this.Request.Headers.UserAgent;

            // Note: Check if, product token obtained from the current request matches with configured internal admin/developer tool product token.
            return userAgents != null && userAgents.Any(productInfoHeader => string.Compare(productInfoHeader.Product.Name, UnityMvcActivator.AdminToolProductToken, StringComparison.OrdinalIgnoreCase) == 0);
        }
    }
}
