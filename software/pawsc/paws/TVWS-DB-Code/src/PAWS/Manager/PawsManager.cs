// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.PAWS.Manager
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Diagnostics;
    using System.Linq;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;

    /// <summary>
    /// Represents PawsManager
    /// </summary>
    public class PawsManager : IPawsBL
    {
        /// <summary>
        /// private variable to have version
        /// </summary>
        private string version = Utils.Configuration[Constants.PawsApiVersion];

        /// <summary>
        /// Gets or sets the IDriverPaws.
        /// </summary>
        [Dependency]
        public IDriverPaws PawsDriver { get; set; }

        /// <summary>
        /// Gets or sets ILogger Interface
        /// </summary>
        [Dependency]
        public ILogger PawsLogger { get; set; }

        /// <summary>
        /// Gets or sets IPawsValidator Interface
        /// </summary>
        [Dependency]
        public IPawsValidator PawsValidator { get; set; }

        /// <summary>
        /// Initializes Master Device
        /// </summary>
        /// <param name="parameters">parameters coming from master device</param>
        /// <returns>Returns the paws response.</returns>
        public PawsResponse Initialize(Parameters parameters)
        {
            const string LogMethodName = "PawsManager.Initialize(Parameters parameters)";

            //// Begin Log transaction
            this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Enter " + LogMethodName);

            try
            {
                Check.IsNotNull<ParametersBase>(parameters, Constants.ParameterNameParameters);
                InitRequestBase initRequest = parameters.GetInitRequest();

                List<string> errorMessages = null;
                if (!this.PawsValidator.ValidateInitializationRequest(initRequest, out errorMessages))
                {
                    return ErrorHelper.CreateErrorResponse(Constants.TypeInitResponse, errorMessages.ToArray());
                }

                string requestVersion = parameters.Version;
                if (requestVersion != this.version)
                {
                    return ErrorHelper.CreateErrorResponse(Constants.TypeInitResponse, Constants.ErrorMessageDatabaseUnsupported);
                }

                // Route to Initialize method in WhiteSpace Driver
                int resp = this.PawsDriver.Initialize(initRequest);

                PawsResponse initResponse = null;
                switch (resp)
                {
                    case 0:
                        {
                            initResponse = new PawsResponse()
                                           {
                                               JsonRpc = "2.0",
                                               Result = new Result
                                                        {
                                                            Type = Constants.TypeInitResponse,
                                                            Version = this.version,
                                                            RulesetInfo = this.PawsDriver.GetRuleSetInfo(initRequest.DeviceDescriptor)
                                                        }
                                           };
                            break;
                        }

                    case -302:
                        return ErrorHelper.CreateErrorResponse(Constants.TypeInitResponse, Constants.ErrorMessageNotRegistered);
                    default:
                        return ErrorHelper.CreateErrorResponse(Constants.TypeInitResponse, Constants.ErrorMessageServerError);
                }

                // End Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Exit " + LogMethodName);

                // Return Response
                return initResponse;
            }
            catch (Exception ex)
            {
                // Log failed transaction
                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.PAWSGenericMessage, ex.ToString());
                var initResponse = ErrorHelper.CreateExceptionResponse(Constants.TypeInitResponse, ex.ToString());
                return initResponse;
            }
        }

        /// <summary>
        /// Registers Master Device
        /// </summary>
        /// <param name="parameters">parameters coming from master device</param>
        /// <returns>Returns the paws response.</returns>
        public PawsResponse Register(Parameters parameters)
        {
            try
            {
                const string LogMethodName = "PawsManager.Register(Parameters parameters)";

                // Begin Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Enter " + LogMethodName);
                Check.IsNotNull<ParametersBase>(parameters, Constants.ParameterNameParameters);

                string requestVersion = parameters.Version;

                if (requestVersion != this.version)
                {
                    return ErrorHelper.CreateErrorResponse(Constants.TypeRegisterResponse, Constants.ErrorMessageDatabaseUnsupported);
                }

                RegisterRequestBase registerRequest = parameters.GetRegisterRequest();
                List<string> errorMessages;

                if (!this.PawsValidator.ValidateRegisterRequest(registerRequest, out errorMessages))
                {
                    return ErrorHelper.CreateErrorResponse(Constants.TypeRegisterResponse, errorMessages.ToArray());
                }

                // Route to Register method in WhiteSpace Driver
                int resp = this.PawsDriver.Register(registerRequest);
                PawsResponse response = null;

                if (resp == 0)
                {
                    response = new PawsResponse
                               {
                                   JsonRpc = "2.0",
                                   Result = new Result
                                            {
                                                Type = Constants.TypeRegisterResponse,
                                                Version = this.version,
                                                RulesetInfo = this.PawsDriver.GetRuleSetInfo(registerRequest.DeviceDescriptor)
                                            }
                               };
                }
                else
                {
                    response = ErrorHelper.CreateErrorResponse(Constants.TypeRegisterResponse, Constants.ErrorMessageServerError);
                }

                // End Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Exit " + LogMethodName);

                // Return Response
                return response;
            }
            catch (Exception ex)
            {
                // Log failed transaction
                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.PAWSGenericMessage, ex.ToString());
                PawsResponse resp = ErrorHelper.CreateExceptionResponse(Constants.TypeRegisterResponse, ex.ToString());
                return resp;
            }
        }

        /// <summary>
        /// Master Device query for available spectrum
        /// </summary>
        /// <param name="parameters">parameters coming from master device</param>
        /// <returns>Returns the paws response.</returns>
        public PawsResponse AvailableSpectrum(Parameters parameters)
        {
            const string LogMethodName = "PawsManager.AvailableSpectrum(Parameters parameters)";

            // Begin Log transaction
            this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Enter " + LogMethodName);
            Check.IsNotNull<ParametersBase>(parameters, Constants.ParameterNameParameters);

            PawsResponse resultResponse = null;

            try
            {
                string requestVersion = parameters.Version;

                if (requestVersion != this.version)
                {
                    return ErrorHelper.CreateErrorResponse(Constants.TypeAvailableSpectrumResponse, Constants.ErrorMessageDatabaseUnsupported);
                }

                AvailableSpectrumRequestBase spectrumRequest = parameters.GetAvailableSpectrumRequest();
                List<string> errorMessages;

                // Validate Parameters specific to Available Spectrum Query
                if (!this.PawsValidator.ValidateSpectrumQueryRequest(spectrumRequest, out errorMessages))
                {
                    return ErrorHelper.CreateErrorResponse(Constants.TypeAvailableSpectrumResponse, errorMessages.ToArray());
                }

                // Route to GetAvailableSpectrum method in WhiteSpace Driver
                var spectrumSchedules = this.PawsDriver.GetAvailableSpectrum(spectrumRequest);
                List<SpectrumSpec> spectrumSpecArray = new List<SpectrumSpec>();
                RulesetInfo[] ruleSetInfo = this.PawsDriver.GetRuleSetInfo(spectrumRequest.DeviceDescriptor);
                foreach (RulesetInfo rs in ruleSetInfo)
                {
                    float? maxTotalBwKhz = null;
                    float? maxNominalChannelBwKhz = null;
                    if (rs.MaxTotalBwMhz.HasValue)
                    {
                        maxTotalBwKhz = (float)rs.MaxTotalBwMhz;
                    }

                    if (rs.MaxNominalChannelBwMhz.HasValue)
                    {
                        maxNominalChannelBwKhz = (float)rs.MaxNominalChannelBwMhz;
                    }

                    SpectrumSpec spectrumSpec = new SpectrumSpec();
                    spectrumSpec.RulesetInfo = rs;
                    spectrumSpec.SpectrumSchedules = spectrumSchedules;
                    spectrumSpec.NeedsSpectrumReport = true;
                    spectrumSpec.MaxTotalBwHz = maxTotalBwKhz;
                    spectrumSpec.MaxContiguousBwHz = maxNominalChannelBwKhz;
                    spectrumSpecArray.Add(spectrumSpec);
                }

                resultResponse = new PawsResponse
                {
                    JsonRpc = "2.0",
                    Result = new Result
                    {
                        Type = Constants.TypeAvailableSpectrumResponse,
                        Version = this.version,
                        TimeStamp = DateTime.UtcNow.ToString("u"),
                        DeviceDescriptor = spectrumRequest.DeviceDescriptor,
                        SpectrumSpecs = spectrumSpecArray.ToArray()
                    }
                };
            }
            catch (Exception ex)
            {
                // Log failed transaction
                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.PAWSGenericMessage, ex.ToString());
                resultResponse = ErrorHelper.CreateExceptionResponse(Constants.TypeAvailableSpectrumResponse, ex.ToString());
            }

            // End Log transaction
            this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Exit " + LogMethodName);

            // Return Response
            return resultResponse;
        }

        /// <summary>
        /// validates slave 
        /// </summary>
        /// <param name="parameters">parameters coming from master device</param>
        /// <returns>Returns the paws response.</returns>
        public PawsResponse ValidateDevice(Parameters parameters)
        {
            const string LogMethodName = "PawsManager.ValidateDevice(Parameters parameters)";

            try
            {
                // Begin Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Enter " + LogMethodName);
                Check.IsNotNull<ParametersBase>(parameters, Constants.ParameterNameParameters);

                DeviceValidityRequestBase deviceValidRequest = parameters.GetDeviceValidRequest();
                List<string> errorMessages = null;

                if (!this.PawsValidator.ValidateDeviceRequest(deviceValidRequest, out errorMessages))
                {
                    return ErrorHelper.CreateErrorResponse(Constants.TypeDeviceValidationResponse, errorMessages.ToArray());
                }

                if (this.version != parameters.Version)
                {
                    return ErrorHelper.CreateErrorResponse(Constants.TypeDeviceValidationResponse, Constants.ErrorMessageDatabaseUnsupported);
                }

                PawsResponse deviceValidResponse = new PawsResponse();
                deviceValidResponse.JsonRpc = "2.0";
                //// Route to device validation method in WhiteSpace Driver
                for (int deviceIndex = 0; deviceIndex < deviceValidRequest.DevicesDescriptors.Count(); deviceIndex++)
                {
                    if (deviceValidResponse.Result == null)
                    {
                        deviceValidResponse.Result = new Result();
                    }

                    deviceValidResponse.Result.Type = Constants.TypeDeviceValidationResponse;
                    deviceValidResponse.Result.Version = this.version;
                    if (deviceValidResponse.Result.DeviceValidities == null)
                    {
                        deviceValidResponse.Result.DeviceValidities = new DeviceValidity[deviceValidRequest.DevicesDescriptors.Count()];
                    }

                    deviceValidResponse.Result.DeviceValidities[deviceIndex] = new DeviceValidity();
                    deviceValidResponse.Result.DeviceValidities[deviceIndex].DeviceDesciptor = deviceValidRequest.DevicesDescriptors[deviceIndex];
                    deviceValidResponse.Result.DeviceValidities[deviceIndex].IsValid = false;
                    int validDeviceResponse = this.PawsDriver.ValidateDevice(deviceValidRequest, deviceIndex);
                    if (validDeviceResponse == -32000)
                    {
                        deviceValidResponse.Result = null;
                        deviceValidResponse.Error = new Result();
                        deviceValidResponse.Error.Code = "-32000";
                        deviceValidResponse.Error.Message = "The web service is down";
                    }
                    else if (validDeviceResponse == 1)
                    {
                        deviceValidResponse.Result.DeviceValidities[deviceIndex].IsValid = true;
                    }
                    else
                    {
                        deviceValidResponse.Result.DeviceValidities[deviceIndex].Reason = Constants.InvalidDeviceReasonMessage;
                    }
                }

                // End Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Exit " + LogMethodName);

                // Return Response
                return deviceValidResponse;
            }
            catch (Exception ex)
            {
                // Log failed transaction
                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.PAWSGenericMessage, ex.ToString());
                return ErrorHelper.CreateExceptionResponse(Constants.TypeDeviceValidationResponse, ex.ToString());
            }
        }

        /// <summary>
        /// Mater device notify about spectrum usage
        /// </summary>
        /// <param name="parameters">parameters coming from master device</param>
        /// <returns>Returns the paws response.</returns>
        public PawsResponse NotifySpectrumUsage(Parameters parameters)
        {
            try
            {
                const string LogMethodName = "PawsManager.NotifySpectrumUsage(Parameters parameters)";

                // Begin Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Enter " + LogMethodName);
                Check.IsNotNull<ParametersBase>(parameters, Constants.ParameterNameParameters);

                string requestVersion = parameters.Version;

                if (requestVersion != this.version)
                {
                    return ErrorHelper.CreateErrorResponse(Constants.TypeNotifySpectrumUsageResponse, Constants.ErrorMessageDatabaseUnsupported);
                }

                NotifyRequestBase notifyRequest = parameters.GetNotifyRequest();
                List<string> errorMessages;

                if (!this.PawsValidator.ValidateNotifyRequest(notifyRequest, out errorMessages))
                {
                    return ErrorHelper.CreateErrorResponse(Constants.TypeNotifySpectrumUsageResponse, errorMessages.ToArray());
                }

                int resp = 0;
                string errorMessage = string.Empty;
                // Route to Notify Use Spectrum method in WhiteSpace Driver                
                resp = this.PawsDriver.NotifySpectrumUsage(notifyRequest, out errorMessage);
                

                PawsResponse response = null;

                if (resp == 0)
                {
                    response = new PawsResponse
                    {
                        JsonRpc = "2.0",
                        Result = new Result
                        {
                            Type = Constants.TypeNotifySpectrumUsageResponse,
                            Version = this.version,
                        }
                    };
                }
                else if(resp == -202)
                {
                    response = ErrorHelper.CreateErrorResponse(Constants.TypeNotifySpectrumUsageResponse, Constants.ErrorMessageServerError);
                    response.Error.Data = errorMessage;
                }
                else
                {
                    response = ErrorHelper.CreateErrorResponse(Constants.TypeNotifySpectrumUsageResponse, Constants.ErrorMessageServerError);                    
                }

                // End Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Exit " + LogMethodName);

                // Return Response
                return response;
            }
            catch (Exception ex)
            {
                // Log failed transaction
                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.PAWSGenericMessage, ex.ToString());
                PawsResponse resp = ErrorHelper.CreateExceptionResponse(Constants.TypeNotifySpectrumUsageResponse, ex.ToString());
                return resp;
            }
        }

        /// <summary>
        /// Master Device query for batch available spectrum
        /// </summary>
        /// <param name="parameters">parameters coming from master device</param>
        /// <returns>Returns the paws response.</returns>
        public PawsResponse AvailableSpectrumBatch(Parameters parameters)
        {
            const string LogMethodName = "PawsManager.AvailableSpectrumBatch(Parameters parameters)";
            this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Enter " + LogMethodName);
            Check.IsNotNull<ParametersBase>(parameters, Constants.ParameterNameParameters);

            PawsResponse resultResponse = null;

            try
            {
                string requestVersion = parameters.Version;

                if (requestVersion != this.version)
                {
                    return ErrorHelper.CreateErrorResponse(Constants.TypeBatchAvailableSpectrumResponse, Constants.ErrorMessageDatabaseUnsupported);
                }

                BatchAvailableSpectrumRequestBase spectrumRequest = parameters.GetBatchAvailableSpectrumRequest();
                List<string> errorMessages;
                if (!this.PawsValidator.ValidateBatchSpectrumQueryRequest(spectrumRequest, out errorMessages))
                {
                    return ErrorHelper.CreateErrorResponse(Constants.TypeBatchAvailableSpectrumResponse, errorMessages.ToArray());
                }

                var spectrumSchedules = this.PawsDriver.GetAvailableSpectrumBatch(spectrumRequest);

                resultResponse = new PawsResponse
                {
                    JsonRpc = "2.0",
                    Result = new Result
                    {
                        Type = Constants.TypeBatchAvailableSpectrumResponse,
                        Version = this.version,
                        TimeStamp = DateTime.UtcNow.ToString("u"),
                        DeviceDescriptor = spectrumRequest.DeviceDescriptor,
                        GeoSpectrumSpecs = spectrumSchedules
                    }
                };
            }
            catch (Exception ex)
            {
                // Log failed transaction
                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.PAWSGenericMessage, ex.ToString());
                resultResponse = ErrorHelper.CreateExceptionResponse(Constants.TypeBatchAvailableSpectrumResponse, ex.ToString());
            }

            this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Exit " + LogMethodName);

            return resultResponse;
        }

        /// <summary>
        /// Interference Query of Master Device
        /// </summary>
        /// <param name="parameters">parameters coming from master device</param>
        /// <returns>Returns the paws response.</returns>
        public PawsResponse InterferenceQuery(Parameters parameters)
        {
            try
            {
                const string LogMethodName = "PawsManager.InterferenceQuery(Parameters parameters)";

                // Begin Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Enter " + LogMethodName);
                Check.IsNotNull<ParametersBase>(parameters, Constants.ParameterNameParameters);
                string requestVersion = parameters.Version;
                if (requestVersion != this.version)
                {
                    return ErrorHelper.CreateErrorResponse(Constants.TypeInterferenceQueryResponse, Constants.ErrorMessageDatabaseUnsupported);
                }

                InterferenceQueryRequestBase interferenceRequest = parameters.GetInterferenceQueryRequest();
                List<string> errorMessages;
                if (!this.PawsValidator.ValidateInterferenceQueryRequest(interferenceRequest, out errorMessages))
                {
                    return ErrorHelper.CreateErrorResponse(Constants.TypeInterferenceQueryResponse, errorMessages.ToArray());
                }

                // Route to Interference Query method in WhiteSpace Driver
                int resp = this.PawsDriver.InterferenceQuery(interferenceRequest);
                PawsResponse response = null;
                if (resp == 0)
                {
                    response = new PawsResponse
                    {
                        JsonRpc = "2.0",
                        Result = new Result
                        {
                            Type = Constants.TypeInterferenceQueryResponse,
                            Version = this.version,
                        }
                    };
                }
                else if (resp == 200)
                {
                    response = new PawsResponse
                    {
                        JsonRpc = "2.0",
                        Error = new Result
                        {
                            Code = resp.ToString(),
                            Message = Constants.IncumbentNotFound
                        }
                    };
                }
                else
                {
                    response = ErrorHelper.CreateErrorResponse(Constants.TypeInterferenceQueryResponse, Constants.ErrorMessageServerError);
                }

                // End Log transaction
                this.PawsLogger.Log(TraceEventType.Information, LoggingMessageId.PAWSGenericMessage, "Exit " + LogMethodName);

                // Return Response
                return response;
            }
            catch (Exception ex)
            {
                // Log failed transaction
                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.PAWSGenericMessage, ex.ToString());
                PawsResponse resp = ErrorHelper.CreateExceptionResponse(Constants.TypeInterferenceQueryResponse, ex.ToString());
                return resp;
            }
        }
    }
}
