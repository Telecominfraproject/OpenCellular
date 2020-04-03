// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Driver
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.Linq;
    using System.Text;
    using Common.ValueProviders;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents Class RegionManagementDriver.
    /// </summary>
    public class RegionManagementDriver : IDriverRegionManagement
    {
        /// <summary>Gets or sets ILogger Interface</summary>
        [Dependency]
        public ILogger Logger { get; set; }

        /// <summary>Gets or sets the IDALCIncumbent.</summary>
        [Dependency]
        public IDalcIncumbent DalcIncumbent { get; set; }

        /// <summary>Gets or sets the common DALC.</summary>
        /// <value>The common DALC.</value>
        [Dependency]
        public IDalcCommon CommonDalc { get; set; }

        /// <summary>
        /// Gets or sets IRegionCalculation Interface
        /// </summary>
        [Dependency]
        public IRegionCalculation RegionCalculation { get; set; }

        /// <summary>
        /// Gets or sets IRegionManagementValidator Interface
        /// </summary>
        [Dependency]
        public IRegionManagementValidator RegionManagementValidator { get; set; }

        /// <summary>
        /// Gets or sets the value provider.
        /// </summary>
        /// <value>The value provider.</value>
        [Dependency]
        public PawsRegionalValueProvider ValueProvider { get; set; }

        #region IDriverRegionManagement methods

        /// <summary>
        /// Adds the incumbents
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="id">The parameter user id.</param>
        /// <returns>returns string.</returns>
        public RegionManagementResponse RegisterDevice(Parameters parameters, string id)
        {
            const string LogMethodName = "RegionManagementDriver.RegisterDevice(Parameters parameters)";

            RegionManagementResponse response = null;
            try
            {
                List<string> errors;

                // Begin Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + LogMethodName);

                string wsdba = Utils.RegistrationIdOrg;
                DateTime settingsDate = this.CommonDalc.GetDateSequenceFromSettingsTable();
                int sequenceNumber = this.CommonDalc.GetSequenceFromSettingsTable();
                string regSeqNumber = sequenceNumber.ToString("0000000");
                RegistrationDisposition regDisposition = new RegistrationDisposition();
                regDisposition.RegDate = settingsDate.ToString("yyyy'-'MM'-'dd'T'HH':'mm':'ss'Z'", DateTimeFormatInfo.InvariantInfo);
                regDisposition.RegId = string.Format("{0:yyMMdd}", settingsDate) + Utils.RegistrationIdOrg + regSeqNumber;
                regDisposition.Action = 1;
                Check.IsNotNull<ParametersBase>(parameters, Constants.ParameterNameParameters);

                if (parameters.IncumbentType == null)
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeRegisterDeviceResponse, Constants.ErrorMessageIncumbentTypeRequired);
                    return response;
                }

                IncumbentType requestIncumbentType = Conversion.ToIncumbentType(parameters.IncumbentType);

                if (requestIncumbentType == IncumbentType.MVPD)
                {
                    if (this.RegisterMVPDEntity(parameters, regDisposition, id, out errors))
                    {
                        response = new RegionManagementResponse
                        {
                            Result = new Result
                            {
                                Type = Constants.TypeRegisterDeviceResponse,
                                Message = "Device Registered successfully.",
                            }
                        };
                    }
                    else
                    {
                        response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeRegisterDeviceResponse, errors.ToArray());
                    }
                }
                else if (requestIncumbentType == IncumbentType.TBAS)
                {
                    if (this.RegisterTempBASEntity(parameters, regDisposition, id, out errors))
                    {
                        response = new RegionManagementResponse
                        {
                            Result = new Result
                            {
                                Type = Constants.TypeRegisterDeviceResponse,
                                Message = "Device Registered successfully.",
                            }
                        };
                    }
                    else
                    {
                        response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeRegisterDeviceResponse, errors.ToArray());
                    }
                }
                else if (requestIncumbentType == IncumbentType.LPAux)
                {
                    if (this.RegisterLicensedLPAux(parameters, regDisposition, id, out errors))
                    {
                        response = new RegionManagementResponse
                        {
                            Result = new Result
                            {
                                Type = Constants.TypeRegisterDeviceResponse,
                                Message = "Device Registered successfully.",
                            }
                        };
                    }
                    else
                    {
                        response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeRegisterDeviceResponse, errors.ToArray());
                    }
                }
                else if (requestIncumbentType == IncumbentType.UnlicensedLPAux)
                {
                    if (this.RegisterUnLicensedLPAux(parameters, regDisposition, id, out errors))
                    {
                        response = new RegionManagementResponse
                        {
                            Result = new Result
                            {
                                Type = Constants.TypeRegisterDeviceResponse,
                                Message = "Device Registered successfully.",
                            }
                        };
                    }
                    else
                    {
                        response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeRegisterDeviceResponse, errors.ToArray());
                    }
                }
                else
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeRegisterDeviceResponse, Constants.ErrorMessageInvalidIncumbentType);
                    return response;
                }

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + LogMethodName);

                // Return the response
                return response;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Deletes the incumbents
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns integer.</returns>
        public RegionManagementResponse DeleteIncumbentInfo(Parameters parameters)
        {
            string logMethodName = "RegionManagementDriver.DeleteIncumbentInfo(Parameters parameters)";

            // Begin Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

            RegionManagementResponse response = null;
            string registrationTableName = string.Empty;

            try
            {
                if (string.IsNullOrEmpty(parameters.IncumbentType))
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeDeleteIncumbentsResponse, Constants.ErrorMessageIncumbentTypeRequired);
                }
                else
                {
                    List<string> errorMessages;

                    // Validate Parameters
                    if (!this.RegionManagementValidator.ValidateDeleteIncumbentRequest(parameters, out errorMessages))
                    {
                        return ErrorHelper.CreateRegionErrorResponse(Constants.TypeDeleteIncumbentsResponse, errorMessages.ToArray());
                    }

                    if (parameters.IncumbentType.ToLower() == IncumbentType.MVPD.ToString().ToLower())
                    {
                        registrationTableName = Utils.GetRegionalTableName(Constants.MVPDRegistrationTableName);
                    }
                    else if (parameters.IncumbentType.ToLower() == IncumbentType.LPAux.ToString().ToLower() ||
                             parameters.IncumbentType.ToLower() == IncumbentType.UnlicensedLPAux.ToString().ToLower())
                    {
                        registrationTableName = Utils.GetRegionalTableName(Constants.LPAuxRegistrationTable);
                    }
                    else if (parameters.IncumbentType.ToLower() == IncumbentType.TBAS.ToString().ToLower())
                    {
                        registrationTableName = Utils.GetRegionalTableName(Constants.TempBasRegistrationTableName);
                    }
                }

                if (registrationTableName != string.Empty)
                {
                    string resp = this.DalcIncumbent.DeleteIncumbentData(registrationTableName, parameters.RegistrationDisposition.RegId);

                    if ((registrationTableName == Utils.GetRegionalTableName(Constants.LPAuxRegistrationTable)) && resp == "Success")
                    {
                        this.DalcIncumbent.DeleteIncumbentData(Utils.GetRegionalTableName(Constants.LPAuxRegDetails), parameters.RegistrationDisposition.RegId);
                    }

                    if (resp == "Success")
                    {
                        response = new RegionManagementResponse
                        {
                            Result = new Result
                            {
                                Type = Constants.TypeDeleteIncumbentsResponse,
                                Message = "Incumbent deleted successfully.",
                            }
                        };
                    }
                    else if (resp == "No incumbents found")
                    {
                        response = new RegionManagementResponse
                        {
                            Result = new Result
                            {
                                Type = Constants.TypeDeleteIncumbentsResponse,
                                Message = Constants.ErrorMessageNoData,
                            }
                        };
                    }
                    ////else
                    ////{
                    ////    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeDeleteIncumbentsResponse, Constants.ErrorMessageServerError);
                    ////}
                }

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);

                // Return the response
                return response;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Gets the incumbents
        /// </summary>
        /// <param name="incumbentType">The incumbentType.</param>
        /// <param name="id">The parameter user id.</param>
        /// <returns>returns list.</returns>
        public object[] GetIncumbents(string incumbentType, string id)
        {
            try
            {
                string logMethodName = "RegionManagementDriver.GetIncumbents(" + incumbentType + ")";
                object[] response = null;
                string registrationTableName = string.Empty;
                IncumbentType requestType = IncumbentType.None;

                // Begin Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

                if (incumbentType.ToLower() == IncumbentType.MVPD.ToString().ToLower())
                {
                    registrationTableName = Utils.GetRegionalTableName(Constants.MVPDRegistrationTableName);
                    var result = this.DalcIncumbent.GetIncumbentsData<MVPDRegistration>(requestType, registrationTableName, id);
                    foreach (var item in result)
                    {
                        item.DeSerializeAndClearObjectsFromJson();
                    }

                    response = result.ToArray();
                }
                else if (incumbentType.ToLower() == IncumbentType.LPAux.ToString().ToLower())
                {
                    registrationTableName = Utils.GetRegionalTableName(Constants.LPAuxRegistrationTable);
                    var result = this.DalcIncumbent.GetIncumbentsData<LPAuxRegistration>(requestType, registrationTableName, id);
                    foreach (var item in result)
                    {
                        item.DeSerializeAndClearObjectsFromJson();
                    }

                    response = result.ToArray();
                }
                else if (incumbentType.ToLower() == IncumbentType.UnlicensedLPAux.ToString().ToLower())
                {
                    registrationTableName = Utils.GetRegionalTableName(Constants.LPAuxRegistrationTable);
                    var result = this.DalcIncumbent.GetIncumbentsData<LPAuxRegistration>(requestType, registrationTableName, id).Where(obj => obj.Licensed == false).ToList();
                    foreach (var item in result)
                    {
                        item.DeSerializeAndClearObjectsFromJson();
                    }

                    response = result.ToArray();
                }
                else if (incumbentType.ToLower() == IncumbentType.TBAS.ToString().ToLower())
                {
                    registrationTableName = Utils.GetRegionalTableName(Constants.TempBasRegistrationTableName);
                    var result = this.DalcIncumbent.GetIncumbentsData<TempBASRegistration>(requestType, registrationTableName, id);
                    foreach (var item in result)
                    {
                        item.DeSerializeAndClearObjectsFromJson();
                    }

                    response = result.ToArray();
                }

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);

                // Return the response
                return response;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Gets the protected entity.
        /// </summary>
        /// <param name="incumbentType">Type of the incumbent.</param>
        /// <returns>returns List{ProtectedEntityWithEvent}.</returns>
        public List<ProtectedEntityWithEvent> GetProtectedEntityWithEvents(string incumbentType)
        {
            try
            {
                string logMethodName = "RegionManagementDriver.GetProtectedEntityWithEvents(" + incumbentType + ")";
                List<ProtectedEntityWithEvent> response = null;
                string registrationTableName;
                dynamic incumbentData = null;

                // Begin Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

                if (incumbentType.ToLower() == IncumbentType.MVPD.ToString().ToLower())
                {
                    registrationTableName = Utils.GetRegionalTableName(Constants.MVPDRegistrationTableName);
                    incumbentData = this.CommonDalc.FetchEntity<MVPDRegistration>(registrationTableName, null);
                    response = this.ParseMVPDtoProtectedEntity(incumbentData);
                }
                else if (incumbentType.ToLower() == IncumbentType.LPAux.ToString().ToLower())
                {
                    registrationTableName = Utils.GetRegionalTableName(Constants.LPAuxRegistrationTable);
                    incumbentData = this.CommonDalc.FetchEntity<LPAuxRegistration>(registrationTableName, null);
                    response = this.ParseLPAUXtoProtectedEntity(incumbentData);
                }
                else if (incumbentType.ToLower() == IncumbentType.TBAS.ToString().ToLower())
                {
                    registrationTableName = Utils.GetRegionalTableName(Constants.TempBasRegistrationTableName);
                    incumbentData = this.CommonDalc.FetchEntity<TempBASRegistration>(registrationTableName, null);
                    response = this.ParseTempBasstoProtectedEntity(incumbentData);
                }
                else if (incumbentType.ToLower() == IncumbentType.FIXED_TVBD.ToString().ToLower())
                {
                    registrationTableName = Constants.RGN1FixedTVBDRegistrationTable;
                    incumbentData = this.CommonDalc.FetchEntity<FixedTVBDRegistration>(registrationTableName, null);
                    response = this.ParseFixedTVBDToProtectedEntity(incumbentData);
                }
                else
                {
                    return null;
                }

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);

                // Return the response
                return response;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Gets the protected entity.
        /// </summary>
        /// <param name="incumbentType">Type of the incumbent.</param>
        /// <returns>returns List{ProtectedEntity}.</returns>
        public List<ProtectedEntity> GetProtectedEntity(string incumbentType)
        {
            try
            {
                string logMethodName = "RegionManagementDriver.GetProtectedEntity(" + incumbentType + ")";
                List<ProtectedEntity> response = null;
                string registrationTableName;
                dynamic incumbentData = null;

                // Begin Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);
                if (incumbentType.ToLower() == IncumbentType.BAS.ToString().ToLower())
                {
                    registrationTableName = Utils.GetRegionalTableName(Constants.ULSBroadcastAuxiliaryTableName);
                    incumbentData = this.CommonDalc.FetchEntity<ULSRecord>(registrationTableName, null);
                    response = this.ParseULSRecordDatatoProtectedEntity(incumbentData, "BAS", "KEYHOLE");
                }
                else if (incumbentType.ToLower() == IncumbentType.PLCMRS.ToString().ToLower())
                {
                    registrationTableName = Utils.GetRegionalTableName(Constants.ULSPLCMRSTableName);
                    incumbentData = this.CommonDalc.FetchEntity<ULSRecord>(registrationTableName, null);
                    response = this.ParseULSRecordDatatoProtectedEntity(incumbentData, "PLCMRS", "POINT");
                }
                else if (incumbentType.ToLower() == IncumbentType.TV_TRANSLATOR.ToString().ToLower())
                {
                    registrationTableName = Utils.GetRegionalTableName(Constants.CDBSTranslatorDataTableName);
                    incumbentData = this.CommonDalc.FetchEntity<CDBSTvEngData>(registrationTableName, null);
                    List<TableEntity> data = new List<TableEntity>();
                    data.AddRange(incumbentData);
                    response = this.ParseCDBSTvEngDatatoProtectedEntity(data, "TV_TRANSLATOR", "KEYHOLE");
                }
                else if (incumbentType.ToLower() == IncumbentType.TV_US.ToString().ToLower())
                {
                    registrationTableName = Utils.GetRegionalTableName(Constants.CDBSUSMexicoTVEngDataTableName);
                    incumbentData = this.DalcIncumbent.GetProtectedEntityUSMexico(registrationTableName, "TV_US");
                    response = this.ParseCDBSTvEngDatatoProtectedEntity(incumbentData, "TV_US", "POINT");
                }
                else if (incumbentType.ToLower() == IncumbentType.TVCA_MX.ToString().ToLower())
                {
                    registrationTableName = Utils.GetRegionalTableName(Constants.CDBSUSMexicoTVEngDataTableName);
                    incumbentData = this.DalcIncumbent.GetProtectedEntityUSMexico(registrationTableName, "TV_MX");
                    registrationTableName = Utils.GetRegionalTableName(Constants.CDBSCanadaTvEngDataTableName);
                    incumbentData.AddRange(this.DalcIncumbent.GetProtectedEntity(registrationTableName));
                    response = this.ParseCDBSTvEngDatatoProtectedEntity(incumbentData, "TVCA_MX", "POINT");
                }
                else
                {
                    return null;
                }

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);

                // Return the response
                return response;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Gets the authorized devices.
        /// </summary>
        /// <returns>returns List{AuthorizedDeviceRecord}.</returns>
        public List<AuthorizedDeviceRecord> GetAuthorizedDevices()
        {
            try
            {
                string logMethodName = "RegionManagementDriver.GetAuthorizedDevices";

                // Begin Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

                var response = this.CommonDalc.FetchEntity<AuthorizedDeviceRecord>(Utils.GetRegionalTableName(Constants.AuthorizedDeviceModelsTableName), null);
                foreach (var authorizedDeviceRecord in response)
                {
                    authorizedDeviceRecord.PartitionKey = null;
                    authorizedDeviceRecord.RowKey = null;
                    authorizedDeviceRecord.ETag = null;
                    authorizedDeviceRecord.Timestamp = DateTimeOffset.MinValue;
                }

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);

                // Return the response
                return response;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Gets the LPAUX license information.
        /// </summary>
        /// <param name="licenseType">Type of the license.</param>
        /// <returns>returns List{LPAUX LicenseInfo}.</returns>
        public List<LpAuxLicenseInfo> GetLpAuxLicenseInfo(string licenseType = null)
        {
            try
            {
                const string LogMethodName = "RegionManagementDriver.GetLpAuxLicenseInfo";

                // Begin Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + LogMethodName);
                string tableName = string.Empty;

                if (licenseType != null && licenseType == "LICEN")
                {
                    tableName = Utils.GetRegionalTableName(Constants.ULSLicensedLPAuxTableName);
                }
                else
                {
                    tableName = Utils.GetRegionalTableName(Constants.ULSUnLicensedLPAuxTableName);
                }

                var response = this.ParseLpAuxLicenseInfoRecords(this.CommonDalc.FetchEntity<ULSRecord>(tableName, null));

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + LogMethodName);

                // Return the response
                return response;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Gets the TV station call signs.
        /// </summary>
        /// <param name="parameter">The parameter.</param>
        /// <returns>returns stations in search criteria.</returns>
        public RegionManagementResponse SearchMVPDCallSigns(Parameters parameter)
        {
            RegionManagementResponse resp = null;

            if (parameter == null || parameter.MVPDLocation == null)
            {
                resp = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetSearchMVPDCallSignsResponse, Constants.ErrorMessageMVPDLocationRequired);
                return resp;
            }

            SquareArea searchArea = GeoCalculations.BuildSquare(parameter.MVPDLocation, new Distance(Utils.Configuration[Constants.ConfigSettingMVPDTTvStationQueryDistance].ToInt32(), DistanceUnit.KM));
            ServiceCacheRequestParameters requestParams = new ServiceCacheRequestParameters
            {
                SearchArea = searchArea
            };

            var incumbents = DatabaseCache.ServiceCacheHelper.GetServiceCacheObjects(ServiceCacheObjectType.TvEngData, requestParams) as Incumbent[];

            if (incumbents.Length == 0)
            {
                resp = ErrorHelper.CreateRegionErrorResponse(Constants.TypeGetSearchMVPDCallSignsResponse, Constants.ErrorMessageNoData);
                return resp;
            }

            var tvstations = new List<MVPDCallSignsInfo>();
            List<string> errors = new List<string>();
            for (int i = 0; i < incumbents.Length; i++)
            {
                var contour = JsonSerialization.DeserializeString<Contour>(incumbents[i].Contour);
                var isvalidForMVPD = GeoCalculations.IsValidMVPDKeyHoleFromContour(contour.ContourPoints, incumbents[i].Location, parameter.MVPDLocation, out errors);
                if (!isvalidForMVPD)
                {
                    continue;
                }

                var tvstationRecord = new MVPDCallSignsInfo();
                tvstationRecord.ServiceType = incumbents[i].VsdService;
                tvstationRecord.Latitude = incumbents[i].Latitude;
                tvstationRecord.Longitude = incumbents[i].Longitude;
                tvstationRecord.CallSign = incumbents[i].CallSign;
                tvstationRecord.Channel = incumbents[i].Channel;

                tvstations.Add(tvstationRecord);
            }

            resp = new RegionManagementResponse()
                   {
                       Result = new Result()
                                {
                                    SearchMVPDCallSigns = tvstations.ToArray()
                                }
                   };

            return resp;
        }

        /// <summary>
        /// Gets MVPD Callsign information
        /// </summary>
        /// <param name="requestedCallsign">callsign</param>
        /// <returns>returns callsign information</returns>
        public RegionManagementResponse GetMVPDCallSignInfo(string requestedCallsign)
        {
            ServiceCacheRequestParameters serviceCacheRequestParameters = new ServiceCacheRequestParameters()
            {
                CallSign = requestedCallsign
            };

            var incumbents = DatabaseCache.ServiceCacheHelper.SearchCacheObjects(ServiceCacheObjectType.TvEngData, SearchCacheRequestType.ByCallSign, serviceCacheRequestParameters) as List<CacheObjectTvEngdata>;

            if(incumbents != null && incumbents.Count > 0)
            {
                var tvstationRecord = new MVPDCallSignsInfo();                
                tvstationRecord.Latitude = incumbents[0].Latitude;
                tvstationRecord.Longitude = incumbents[0].Longitude;
                tvstationRecord.CallSign = incumbents[0].CallSign;
                tvstationRecord.Channel = incumbents[0].Channel;
                tvstationRecord.Contour = incumbents[0].Contour;

                return new RegionManagementResponse
                {
                    Result = new Result()
                    {
                        Type = Constants.GetCallsignInfoResponse,
                        Message = "CallsignInfo",
                        CallsignInfo = tvstationRecord
                    }
                };
            }
            else
            {
                return new RegionManagementResponse
                {
                    Error = new Result
                    {
                        Type = Constants.GetCallsignInfoResponse,                       
                        Message = Constants.ErrorMessageNoData,
                    }
                };
            }
        }

        /// <summary>
        /// Returns only the free channels at the specified location.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>Returns only the free channels.</returns>
        public RegionManagementResponse GetChannelList(Parameters parameters)
        {
            try
            {
                string logMethodName = "RegionManagementDriver.GetChannelList(parameters)";

                // Begin Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

                Incumbent incumbent = this.ValueProvider.IncumbentInfoForGetChannelListRequest(parameters);

                var freeChannelsResp = this.RegionCalculation.GetFreeChannels(incumbent);

                return new RegionManagementResponse()
                       {
                           Result = new Result()
                                    {
                                        Channels = freeChannelsResp.ChannelsInfo,
                                        UniqueId = incumbent.UniqueId,
                                        StartTime = incumbent.StartTime.ToShortDateString(),
                                        EndTime = string.Empty,
                                        ChannelsInCSV = freeChannelsResp.ChannelsInCSVFormat,
                                        IntermediateResults1 = freeChannelsResp.IntermediateResult1,
                                        MasterOperationParameters = freeChannelsResp.IntermediateResult2
                                    }
                       };
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Gets the incumbents
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns list.</returns>
        public List<ProtectedDevice> GetDeviceList(Parameters parameters)
        {
            try
            {
                const string LogMethodName = "RegionManagementDriver.GetDeviceList()";

                // Begin Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + LogMethodName);

                List<ProtectedDevice> protectedDevices = this.RegionCalculation.GetDeviceList(parameters);

                return protectedDevices;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Adds to the list of excluded Ids
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns RegionManagementResponse.</returns>
        public RegionManagementResponse ExcludeIds(Parameters parameters)
        {
            RegionManagementResponse response = null;
            const string LogMethodName = "RegionManagementDriver.ExcludeIds(Parameters parameters)";
            try
            {
                // Begin Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + LogMethodName);
                List<string> errorMessages;

                // Validate Parameters
                if (!this.RegionManagementValidator.ValidateExcludeIds(parameters, out errorMessages))
                {
                    return ErrorHelper.CreateRegionErrorResponse(Constants.TypeExcludeIdsResponse, errorMessages.ToArray());
                }

                DynamicTableEntity entity = new DynamicTableEntity();
                entity.PartitionKey = Utils.CurrentRegionId.ToString();
                entity.RowKey = System.Guid.NewGuid().ToString();

                // Adding entities
                entity.Properties.Add("DeviceId", EntityProperty.GeneratePropertyForString(parameters.DeviceId));
                entity.Properties.Add("SerialNumber", EntityProperty.GeneratePropertyForString(parameters.SerialNumber));

                string resp = this.DalcIncumbent.ExcludeId(entity);
                if (resp == "Success")
                {
                    response = new RegionManagementResponse
                    {
                        Result = new Result
                        {
                            Type = Constants.TypeExcludeIdsResponse,
                            Message = "Excluded Id inserted successfully",
                        }
                    };
                }
                else
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeExcludeIdsResponse, Constants.ErrorMessageServerError);
                }

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + LogMethodName);

                // Return the response
                return response;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Adds to the list of excluded Regions
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns RegionManagementResponse.</returns>
        public RegionManagementResponse ExcludeChannels(Parameters parameters)
        {
            RegionManagementResponse response = null;
            try
            {
                string logMethodName = "RegionManagementDriver.ExcludeChannels(Parameters parameters)";

                // Begin Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);
                List<string> errorMessages;

                // Validate Parameters
                if (!this.RegionManagementValidator.ValidateExcludeChannels(parameters, out errorMessages))
                {
                    return ErrorHelper.CreateRegionErrorResponse(Constants.TypeExcludeChannelsResponse, errorMessages.ToArray());
                }

                DynamicTableEntity entity = new DynamicTableEntity();
                entity.PartitionKey = Utils.CurrentRegionId.ToString();
                entity.RowKey = System.Guid.NewGuid().ToString();

                // Adding entities
                string spectra = JsonSerialization.SerializeObject(parameters.TvSpectra);
                string locations = JsonSerialization.SerializeObject(parameters.Locations);
                entity.Properties.Add("ChannelList", EntityProperty.GeneratePropertyForString(spectra));
                entity.Properties.Add("Location", EntityProperty.GeneratePropertyForString(locations));

                string resp = this.DalcIncumbent.ExcludeChannel(entity);
                if (resp == "Success")
                {
                    response = new RegionManagementResponse
                    {
                        Result = new Result
                        {
                            Type = Constants.TypeExcludeChannelsResponse,
                            Message = "Channel Excluded Successfully.",
                        }
                    };
                }
                else
                {
                    response = ErrorHelper.CreateRegionErrorResponse(Constants.TypeExcludeChannelsResponse, Constants.ErrorMessageServerError);
                }

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);

                // Return the response
                return response;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Gets the contour data.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns RegionManagementResponse.</returns>
        public List<ContourDetailsInfo> GetContourData(Parameters parameters)
        {
            string logMethodName = "RegionManagementDriver.GetContourData";

            try
            {
                // Begin Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

                var incumbents = this.CommonDalc.FetchEntity<Incumbent>(Utils.GetRegionalTableName(Constants.CDBSUSMexicoTVEngDataTableName), new { CallSign = parameters.ContourRequestCallSign, PartitionKey = "CDBS" });

                if (incumbents == null || incumbents.Count == 0)
                {
                    return null;
                }

                List<ContourDetailsInfo> contours = new List<ContourDetailsInfo>();
                foreach (var incumbent in incumbents)
                {
                    incumbent.BuildContourItems = true;
                    var contourData = this.RegionCalculation.CalculateContour(incumbent);
                    var contourDetails = new ContourDetailsInfo();
                    contourDetails.ContourStationData = incumbent;
                    contourDetails.ContourPointItems = contourData.ContourPointItems;
                    contours.Add(contourDetails);
                }

                // End Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);

                // Return the response
                return contours;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /////// <summary>
        /////// Returns only the free channels at the specified location.
        /////// </summary>
        /////// <param name="parameters">The parameters.</param>
        /////// <returns>Returns only the free channels.</returns>
        ////public RegionManagementResponse CancelRegistration(Parameters parameters)
        ////{
        ////    string logMethodName = "PMSEDriver.CancelRegistration(Parameters parameters)";

        ////    // Begin Log transaction
        ////    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

        ////    RegionManagementResponse response = null;
        ////    Check.IsNotNull<ParametersBase>(parameters, Constants.ParameterNameParameters);
        ////    try
        ////    {
        ////        if (parameters.LPAUXRegId != null)
        ////        {
        ////            this.DalcIncumbent.CancelRegistrations(parameters.LPAUXRegId);
        ////            response = new RegionManagementResponse
        ////            {
        ////                Result = new Result
        ////                {
        ////                    Message = "Successfully Cancelled LpAux Unlicensed Registration"
        ////                }
        ////            };
        ////        }
        ////        else
        ////        {
        ////            response = ErrorHelper.CreateRegionErrorResponse(Constants.LPAuxRegistration, Constants.ErrorMessageRegIdRequired);
        ////        }
        ////    }
        ////    catch (Exception e)
        ////    {
        ////        this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
        ////        response = ErrorHelper.CreateRegionErrorResponse(Constants.LPAuxRegistration, e.ToString());
        ////    }

        ////    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);
        ////    return response;
        ////}

        #endregion IDriverRegionManagement methods

        /// <summary>
        /// Parses the MVPD to protected entity.
        /// </summary>
        /// <param name="mvpdRegistrations">The MVPD registrations.</param>
        /// <returns>returns ProtectedEntityWithEvent.</returns>
        private List<ProtectedEntityWithEvent> ParseMVPDtoProtectedEntity(IEnumerable<MVPDRegistration> mvpdRegistrations)
        {
            List<ProtectedEntityWithEvent> records = new List<ProtectedEntityWithEvent>();

            foreach (var registrationData in mvpdRegistrations)
            {
                registrationData.DeSerializeObjectsFromJson();
                ProtectedEntityWithEvent protectedEntity = new ProtectedEntityWithEvent();
                protectedEntity.Uid = registrationData.RowKey;
                protectedEntity.Entity_Type = "MVPD";
                protectedEntity.Channel = registrationData.Channel == null ? string.Empty : Convert.ToString(registrationData.Channel.Channel);
                protectedEntity.Registrar = registrationData.PartitionKey;
                protectedEntity.Callsign = registrationData.Channel == null ? string.Empty : Convert.ToString(registrationData.Channel.CallSign);
                protectedEntity.Location_Type = "KEYHOLE";
                protectedEntity.Latitude = Convert.ToString(registrationData.Latitude);
                protectedEntity.Longitude = Convert.ToString(registrationData.Longitude);
                protectedEntity.Circle_Radius_Meters = "8000";
                protectedEntity.Azimuth =
                    Convert.ToString(
                        GeoCalculations.CalculateBearing(
                            new Location(registrationData.Latitude, registrationData.Longitude),
                            new Location(registrationData.TxLatitude, registrationData.TxLongitude)));
                protectedEntity.Parent_Callsign = registrationData.Channel == null ? string.Empty : Convert.ToString(registrationData.Channel.CallSign);
                protectedEntity.Parent_Latitude = Convert.ToString(registrationData.TxLatitude);
                protectedEntity.Parent_Longitude = Convert.ToString(registrationData.TxLongitude);
                protectedEntity.Registrant = this.BuildRegistrantInformation(registrationData.Registrant);

                records.Add(protectedEntity);
            }

            return records;
        }

        /// <summary>
        /// Parses the temporary BAS to protected entity.
        /// </summary>
        /// <param name="registrations">The registrations.</param>
        /// <returns>returns List{ProtectedEntityWithEvent}.</returns>
        private List<ProtectedEntityWithEvent> ParseTempBasstoProtectedEntity(IEnumerable<TempBASRegistration> registrations)
        {
            List<ProtectedEntityWithEvent> lstProtectedEntity = new List<ProtectedEntityWithEvent>();

            foreach (var registrationData in registrations)
            {
                registrationData.DeSerializeObjectsFromJson();

                foreach (Microsoft.Whitespace.Entities.Calendar time in registrationData.Event.Times)
                {
                    ProtectedEntityWithEvent protectedEntity = new ProtectedEntityWithEvent();
                    protectedEntity.Uid = registrationData.RowKey;
                    protectedEntity.Entity_Type = "TEMP_BAS";
                    protectedEntity.Channel = registrationData.Channel == null ? string.Empty : Convert.ToString(registrationData.Channel.Channel);
                    protectedEntity.Registrar = registrationData.PartitionKey;
                    protectedEntity.Callsign = registrationData.Channel == null ? string.Empty : Convert.ToString(registrationData.Channel.CallSign);
                    protectedEntity.Location_Type = "KEYHOLE";
                    protectedEntity.Latitude = Convert.ToString(registrationData.Latitude);
                    protectedEntity.Longitude = Convert.ToString(registrationData.Longitude);
                    protectedEntity.Circle_Radius_Meters = "8000";
                    protectedEntity.Azimuth =
                        Convert.ToString(
                            GeoCalculations.CalculateBearing(
                                new Location(registrationData.Latitude, registrationData.Longitude),
                                new Location(registrationData.TxLatitude, registrationData.TxLongitude)));
                    protectedEntity.Parent_Callsign = registrationData.Channel == null ? string.Empty : Convert.ToString(registrationData.Channel.CallSign);
                    protectedEntity.Parent_Latitude = Convert.ToString(registrationData.TxLatitude);
                    protectedEntity.Parent_Longitude = Convert.ToString(registrationData.TxLongitude);
                    protectedEntity.Registrant = this.BuildRegistrantInformation(registrationData.Registrant);
                    protectedEntity.Event_Start = time.Start;
                    protectedEntity.Event_End = time.End;

                    lstProtectedEntity.Add(protectedEntity);
                }
            }

            return lstProtectedEntity;
        }

        /// <summary>
        /// Parses the fixed TVBD to protected entity.
        /// </summary>
        /// <param name="registrations">The registrations.</param>
        /// <returns>returns List{ProtectedEntityWithEvent}.</returns>
        private List<ProtectedEntityWithEvent> ParseFixedTVBDToProtectedEntity(List<FixedTVBDRegistration> registrations)
        {
            List<ProtectedEntityWithEvent> lstProtectedEntity = new List<ProtectedEntityWithEvent>();
            ProtectedEntityWithEvent protectedEntity = new ProtectedEntityWithEvent();
            foreach (var registrationData in registrations)
            {
                registrationData.DeSerializeObjectsFromJson();

                if (registrationData.Disposition != null)
                {
                    protectedEntity.Uid = registrationData.Disposition.RegId;
                }

                if (registrationData.DeviceId != null)
                {
                    protectedEntity.Serial_Num = registrationData.DeviceId.SerialNumber;
                }

                protectedEntity.Entity_Type = "FIXED_TVBD";
                protectedEntity.Fccid = registrationData.RowKey;
                protectedEntity.Registrar = registrationData.WSDBA;
                protectedEntity.Location_Type = "POINT";

                if (registrationData.Loc != null)
                {
                    protectedEntity.Latitude = registrationData.Loc.Latitude.ToString();
                    protectedEntity.Longitude = registrationData.Loc.Longitude.ToString();
                }

                if (registrationData.TVBDDeviceOwner != null)
                {
                    var owner = registrationData.TVBDDeviceOwner.Owner;
                    if (owner != null)
                    {
                        protectedEntity.Registrant = string.Concat(owner.FullName, "|", owner.Organization.Text);

                        var address = owner.Address;
                        if (address != null)
                        {
                            protectedEntity.Registrant = string.Concat(protectedEntity.Registrant, "|", address.Locality, "|", address.Region, "|", address.Code, "|", address.Country);
                        }
                    }
                }

                if (protectedEntity.Registrant == string.Empty)
                {
                    protectedEntity.Registrant = null;
                }
                else
                {
                    protectedEntity.Registrant += "||";
                }

                lstProtectedEntity.Add(protectedEntity);
            }

            return lstProtectedEntity;
        }

        /// <summary>
        /// Parses the LPAUX to protected entity.
        /// </summary>
        /// <param name="registrations">The registrations.</param>
        /// <returns>returns List{ProtectedEntityWithEvent}.</returns>
        private List<ProtectedEntityWithEvent> ParseLPAUXtoProtectedEntity(List<LPAuxRegistration> registrations)
        {
            List<ProtectedEntityWithEvent> lstProtectedEntity = new List<ProtectedEntityWithEvent>();
            foreach (var registrationData in registrations)
            {
                registrationData.DeSerializeObjectsFromJson();

                foreach (Microsoft.Whitespace.Entities.Calendar time in registrationData.Event.Times)
                {
                    ProtectedEntityWithEvent protectedEntity = new ProtectedEntityWithEvent();
                    protectedEntity.Uid = registrationData.RowKey;
                    protectedEntity.Entity_Type = "LP_AUX";
                    protectedEntity.Channel = registrationData.CallSign == null ? string.Empty : Convert.ToString(registrationData.CallSign.Channel);
                    protectedEntity.Registrar = registrationData.PartitionKey;
                    protectedEntity.Callsign = registrationData.CallSign == null ? string.Empty : Convert.ToString(registrationData.CallSign.CallSign);
                    string locatioType = string.Empty;
                    if (registrationData.PointsArea != null && registrationData.PointsArea.Count() == 1)
                    {
                        locatioType = "POINT";
                    }
                    else if (registrationData.PointsArea != null && registrationData.PointsArea.Count() > 1)
                    {
                        locatioType = "MULTI_POINT";
                    }
                    else if (registrationData.QuadrilateralArea != null && registrationData.QuadrilateralArea.Count() == 1)
                    {
                        locatioType = "QUAD_POINT";
                    }
                    else if (registrationData.QuadrilateralArea != null && registrationData.QuadrilateralArea.Count() == 1)
                    {
                        locatioType = " MULTI_QUAD";
                    }

                    protectedEntity.Location_Type = locatioType;
                    protectedEntity.Latitude = Convert.ToString(registrationData.Latitude);
                    protectedEntity.Longitude = Convert.ToString(registrationData.Longitude);
                    protectedEntity.Circle_Radius_Meters = "8000";
                    protectedEntity.Parent_Callsign = registrationData.CallSign == null ? string.Empty : Convert.ToString(registrationData.CallSign.CallSign);
                    protectedEntity.Registrant = this.BuildRegistrantInformation(registrationData.Registrant);
                    protectedEntity.Event_Start = time.Start;
                    protectedEntity.Event_End = time.End;

                    lstProtectedEntity.Add(protectedEntity);
                }
            }

            return lstProtectedEntity;
        }

        /// <summary>
        /// Parses the CDBS TVENG data to protected entity.
        /// </summary>
        /// <param name="cdbsRecords">The CDBS records.</param>
        /// <param name="entityType">Type of the entity.</param>
        /// <param name="locationType">Type of the location.</param>
        /// <returns>returns ProtectedEntity.</returns>
        private List<ProtectedEntity> ParseCDBSTvEngDatatoProtectedEntity(List<TableEntity> cdbsRecords, string entityType, string locationType)
        {
            List<ProtectedEntity> records = new List<ProtectedEntity>();
            foreach (var cdbsRecord in cdbsRecords)
            {
                var cdbsTvEngData = cdbsRecord as CDBSTvEngData;
                ProtectedEntity protectedCdbsTv = new ProtectedEntity();

                protectedCdbsTv.Uid = cdbsTvEngData.CallSign;
                if (entityType != "TV_TRANSLATOR")
                {
                    protectedCdbsTv.Entity_Type = cdbsTvEngData.EntityType;
                }
                else
                {
                    protectedCdbsTv.Entity_Type = entityType;
                }

                protectedCdbsTv.Channel = Convert.ToString(cdbsTvEngData.Channel);
                protectedCdbsTv.Facility_Id = Convert.ToString(cdbsTvEngData.FacilityId);
                protectedCdbsTv.Site_Number = Convert.ToString(cdbsTvEngData.SiteNumber);
                protectedCdbsTv.Application_Id = Convert.ToString(cdbsTvEngData.ApplicationId);
                protectedCdbsTv.Tx_Type = Convert.ToString(cdbsTvEngData.VsdService);
                protectedCdbsTv.Erp_Watts = Convert.ToString(cdbsTvEngData.TxPower * 1000);
                protectedCdbsTv.Antenna_Id = Convert.ToString(cdbsTvEngData.AntennaId);
                protectedCdbsTv.Antenna_Rotation_Degrees = Convert.ToString(cdbsTvEngData.AntRotation);
                protectedCdbsTv.Rcamsl_Meters = Convert.ToString(cdbsTvEngData.Height);
                protectedCdbsTv.Rcagl_Meters = Convert.ToString(cdbsTvEngData.HagRcMtr);
                protectedCdbsTv.Haat_Meters = Convert.ToString(cdbsTvEngData.HaatRcMtr);
                protectedCdbsTv.Location_Type = locationType;
                protectedCdbsTv.Latitude = Convert.ToString(cdbsTvEngData.Latitude);
                protectedCdbsTv.Longitude = Convert.ToString(cdbsTvEngData.Longitude);
                protectedCdbsTv.Parent_Callsign = cdbsTvEngData.ParentCallSign;
                protectedCdbsTv.Parent_Facility_Id = cdbsTvEngData.ParentFacilityId.ToString();
                protectedCdbsTv.Parent_Latitude = cdbsTvEngData.ParentLatitude.ToString();
                protectedCdbsTv.Parent_Longitude = cdbsTvEngData.ParentLongitude.ToString();

                protectedCdbsTv.Data_Source = "CDBS";

                records.Add(protectedCdbsTv);
            }

            return records;
        }

        /// <summary>
        /// Parses the ULS record data to protected entity.
        /// </summary>
        /// <param name="ulsRecords">The ULS records.</param>
        /// <param name="entityType">Type of the entity.</param>
        /// <param name="locationType">Type of the location.</param>
        /// <returns>returns ProtectedEntity.</returns>
        private List<ProtectedEntity> ParseULSRecordDatatoProtectedEntity(IEnumerable<ULSRecord> ulsRecords, string entityType, string locationType)
        {
            List<ProtectedEntity> records = new List<ProtectedEntity>();
            foreach (var ulsRecord in ulsRecords)
            {
                ProtectedEntity protectedULSRec = new ProtectedEntity();
                protectedULSRec.Uid = ulsRecord.CallSign;
                protectedULSRec.Entity_Type = entityType;
                protectedULSRec.Channel = Convert.ToString(ulsRecord.Channel);
                protectedULSRec.Facility_Id = Convert.ToString(ulsRecord.FacilityIDOfParentStation);
                protectedULSRec.Application_Id = Convert.ToString(ulsRecord.UniqueSystemIdentifier);
                protectedULSRec.Tx_Type = Convert.ToString(ulsRecord.RadioServiceCode);
                protectedULSRec.Erp_Watts = Convert.ToString(ulsRecord.TxPower * 1000);
                protectedULSRec.Location_Type = locationType;
                protectedULSRec.Latitude = Convert.ToString(ulsRecord.Latitude);
                protectedULSRec.Longitude = Convert.ToString(ulsRecord.Longitude);
                protectedULSRec.Azimuth =
                    Convert.ToString(
                        GeoCalculations.CalculateBearing(
                            new Location(ulsRecord.Latitude, ulsRecord.Longitude),
                            new Location(ulsRecord.TxLatitude, ulsRecord.TxLongitude)));

                protectedULSRec.Keyhole_Radius_Meters = ulsRecord.KeyHoleRadiusMtrs > 80000 ? "80000" : ulsRecord.KeyHoleRadiusMtrs.ToString();
                protectedULSRec.Parent_Facility_Id = Convert.ToString(ulsRecord.FacilityIDOfParentStation);
                protectedULSRec.Parent_Latitude = Convert.ToString(ulsRecord.TxLatitude);
                protectedULSRec.Parent_Longitude = Convert.ToString(ulsRecord.TxLongitude);
                protectedULSRec.Data_Source = "ULS";

                records.Add(protectedULSRec);
            }

            return records;
        }

        /// <summary>
        /// Parses the LPAUX license information records.
        /// </summary>
        /// <param name="dataRecords">The data records.</param>
        /// <returns>returns LPAUXLicenseInfo.</returns>
        private List<LpAuxLicenseInfo> ParseLpAuxLicenseInfoRecords(List<ULSRecord> dataRecords)
        {
            List<LpAuxLicenseInfo> licenseData = new List<LpAuxLicenseInfo>();
            foreach (var objRecord in dataRecords)
            {
                LpAuxLicenseInfo licenseRecord = new LpAuxLicenseInfo();
                licenseRecord.ContactEntityName = objRecord.ContactEntityName;
                licenseRecord.ContactFirstName = objRecord.ContactFirstName;
                licenseRecord.ContactLastName = objRecord.ContactLastName;
                licenseRecord.ContactPhone = objRecord.ContactPhone;
                licenseRecord.ContactFax = objRecord.ContactFax;
                licenseRecord.ContactEmail = objRecord.ContactEmail;
                licenseRecord.ContactStreetAddress = objRecord.ContactStreetAddress;
                licenseRecord.ContactCity = objRecord.ContactCity;
                licenseRecord.ContactState = objRecord.ContactState;
                licenseRecord.ContactZipCode = objRecord.ContactZipCode;
                licenseRecord.ContactPOBox = objRecord.ContactPOBox;

                licenseRecord.RegEntityName = objRecord.RegEntityName;
                licenseRecord.RegFirstName = objRecord.RegFirstName;
                licenseRecord.RegLastName = objRecord.RegLastName;
                licenseRecord.RegPhone = objRecord.RegPhone;
                licenseRecord.RegFax = objRecord.RegFax;
                licenseRecord.RegEmail = objRecord.RegEmail;
                licenseRecord.RegStreetAddress = objRecord.RegStreetAddress;
                licenseRecord.RegCity = objRecord.RegCity;
                licenseRecord.RegZipCode = objRecord.RegZipCode;
                licenseRecord.RegPOBox = objRecord.RegPOBox;

                licenseRecord.VenueName = objRecord.VenueName;
                licenseRecord.VenueTypeCode = objRecord.VenueTypeCode;
                licenseRecord.ULSFileNumber = objRecord.ULSFileNumber;
                licenseRecord.CallSign = objRecord.CallSign;
                licenseRecord.Latitude = objRecord.Latitude;
                licenseRecord.Longitude = objRecord.Longitude;
                licenseRecord.Channel = objRecord.Channel;
                licenseRecord.GrantDate = objRecord.GrantDate.HasValue ? objRecord.GrantDate.Value : DateTimeOffset.MinValue;
                licenseRecord.ExpireDate = objRecord.ExpireDate.HasValue ? objRecord.ExpireDate.Value : DateTimeOffset.MinValue;
                licenseRecord.Channels = objRecord.Channels;
                licenseData.Add(licenseRecord);
            }

            return licenseData;
        }

        /// <summary>
        /// Registers the MVPD entity.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="registrationDisposition">The registration disposition.</param>
        /// <param name="userId">The user identifier.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        private bool RegisterMVPDEntity(Parameters parameters, RegistrationDisposition registrationDisposition, string userId, out List<string> errorMessages)
        {
            // Validate Parameters
            if (!this.RegionManagementValidator.ValidateAddIncumbentRequestMVPD(parameters, out errorMessages))
            {
                return false;
            }

            errorMessages = new List<string>();

            string registrationTableName = Utils.GetRegionalTableName(Constants.MVPDRegistrationTableName);

            MVPDRegistration mvpdReg = new MVPDRegistration()
            {
                PartitionKey = Utils.RegistrationIdOrg,
                RowKey = registrationDisposition.RegId,
                ChannelNumber = parameters.TvSpectrum.Channel.Value,
                Channel = parameters.TvSpectrum,
                Contact = parameters.Contact,
                Location = parameters.MVPDLocation,
                Disposition = registrationDisposition,
                Registrant = parameters.MVPDRegistrant,
                TransmitLocation = parameters.TransmitLocation,
                UserId = userId,
                WSDBA = Utils.RegistrationIdOrg
            };

            mvpdReg.SerializeObjectsToJston();

            var waiverCallSigns = this.CommonDalc.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.MVPDWaiverCallSignTableName), null);

            // if not in waiver then only check for 80km limit
            if (waiverCallSigns.FirstOrDefault(obj => obj.RowKey == mvpdReg.Channel.CallSign) == null)
            {
                var searchRequestParam = new ServiceCacheRequestParameters()
                                         {
                                             CallSign = mvpdReg.Channel.CallSign
                                         };

                var callSignInformation = DatabaseCache.ServiceCacheHelper.SearchCacheObjects(ServiceCacheObjectType.TvEngData, SearchCacheRequestType.ByCallSign, searchRequestParam) as List<CacheObjectTvEngdata>;

                ////  var callSignInformation = this.CommonDalc.FetchEntity<CacheObjectTvEngdata>(Utils.GetRegionalTableName(Constants.CDBSUSMexicoTVEngDataTableName), new { CallSign = mvpdReg.Channel.CallSign });

                if (callSignInformation == null || !callSignInformation.Any())
                {
                    errorMessages.Add(Constants.ErrorMessageMVPDCallSignDoNotExist);
                    return false;
                }

                foreach (CacheObjectTvEngdata callSign in callSignInformation)
                {
                    if (!string.IsNullOrEmpty(callSign.Contour))
                    {
                        var contour = JsonSerialization.DeserializeString<Contour>(callSign.Contour);
                        if (GeoCalculations.IsValidMVPDKeyHoleFromContour(contour.ContourPoints, mvpdReg.TransmitLocation, mvpdReg.Location, out errorMessages))
                        {
                            string resp = this.DalcIncumbent.InsertIncumbentData(registrationTableName, mvpdReg);

                            if (resp != "Success")
                            {
                                errorMessages.Add(Constants.ErrorMessageServerError);
                                return false;
                            }

                            return true;
                        }
                    }
                }
            }
            
            return false;
        }

        /// <summary>
        /// Registers the temporary BAS entity.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="registrationDisposition">The registration disposition.</param>
        /// <param name="userId">The user identifier.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        private bool RegisterTempBASEntity(Parameters parameters, RegistrationDisposition registrationDisposition, string userId, out List<string> errorMessages)
        {
            // Validate Parameters
            if (!this.RegionManagementValidator.ValidateAddIncumbentRequestTBAS(parameters, out errorMessages))
            {
                return false;
            }

            errorMessages = new List<string>();

            string registrationTableName = Utils.GetRegionalTableName(Constants.TempBasRegistrationTableName);

            TempBASRegistration tbasReg = new TempBASRegistration()
            {
                PartitionKey = Utils.RegistrationIdOrg,
                RowKey = registrationDisposition.RegId,
                Channel = parameters.TvSpectrum,
                Contact = parameters.Contact,
                Disposition = registrationDisposition,
                Event = parameters.Event,
                Registrant = parameters.TempBASRegistrant,
                TransmitLocation = parameters.TransmitLocation,
                RecvLocation = parameters.TempBasLocation,
                UserId = userId,
                WSDBA = Utils.RegistrationIdOrg
            };

            tbasReg.SerializeObjectsToJston();

            // Route to insert incumbent data method in AzureDalc
            string resp = this.DalcIncumbent.InsertIncumbentData(registrationTableName, tbasReg);
            if (resp != "Success")
            {
                errorMessages.Add(Constants.ErrorMessageServerError);
                return false;
            }

            ////this.DalcIncumbent.SaveBroadcastStationDetail(null, null, tbasReg);
            return true;
        }

        /// <summary>
        /// Registers the licensed LPAUX
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="registrationDisposition">The registration disposition.</param>
        /// <param name="userId">The user identifier.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        private bool RegisterLicensedLPAux(Parameters parameters, RegistrationDisposition registrationDisposition, string userId, out List<string> errorMessages)
        {
            // Validate Parameters
            if (!this.RegionManagementValidator.ValidateAddIncumbentRequestLPAux(parameters, out errorMessages))
            {
                return false;
            }

            errorMessages = new List<string>();

            string registrationTableName = Utils.GetRegionalTableName(Constants.LPAuxRegistrationTable);

            LPAuxRegistration lowPowerAuxReg = new LPAuxRegistration()
            {
                PartitionKey = Utils.RegistrationIdOrg,
                RowKey = registrationDisposition.RegId,
                CallSign = parameters.TvSpectrum,
                Contact = parameters.Contact,
                Disposition = registrationDisposition,
                Event = parameters.Event,
                Registrant = parameters.LPAuxRegistrant,
                Licensed = true,
                UserId = userId,
                VenueName = parameters.Venue,
                WSDBA = Utils.RegistrationIdOrg
            };

            if (parameters.PointsArea != null)
            {
                lowPowerAuxReg.PointsArea = parameters.PointsArea.Select(obj => obj.ToPosition()).ToArray();
            }
            else
            {
                lowPowerAuxReg.QuadrilateralArea = parameters.QuadrilateralArea;
            }

            lowPowerAuxReg.SerializeObjectsToJston();

            var excludedRegions = this.CommonDalc.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.ExcludedChannels), null);
            if (excludedRegions.Count > 0)
            {
                foreach (var excludedRegion in excludedRegions)
                {
                    var excludedChannel = JsonSerialization.DeserializeString<TvSpectrum[]>(excludedRegion.Properties["ChannelList"].StringValue);

                    // check if excluded region has any channels matching with lpaux
                    if (excludedChannel.FirstOrDefault(obj => obj.Channel == parameters.TvSpectrum.Channel) != null)
                    {
                        var excludedPoints = JsonSerialization.DeserializeString<Location[]>(excludedRegion.Properties["Location"].StringValue);
                        if (GeoCalculations.IsOverlappingCoordinates(excludedPoints, lowPowerAuxReg.PointsArea, lowPowerAuxReg.QuadrilateralArea))
                        {
                            errorMessages.Add(Constants.ErrorMessageLPAUXRegionNotAvailable);
                            return false;
                        }
                    }
                }
            }

            string[] callSigns = this.DalcIncumbent.GetCallSignArray(Utils.GetRegionalTableName(Constants.ULSLicensedLPAuxTableName));

            if (callSigns.FirstOrDefault(obj => obj.Trim() == lowPowerAuxReg.CallSign.CallSign) == null)
            {
                errorMessages.Add(Constants.ErrorMessageCallSignNotExist);
                return false;
            }

            // Route to insert incumbent data method in AzureDalc
            string resp = this.DalcIncumbent.InsertIncumbentData(registrationTableName, lowPowerAuxReg);

            if (resp != "Success")
            {
                errorMessages.Add(Constants.ErrorMessageServerError);
                return false;
            }

            this.DalcIncumbent.SaveLPAuxRegistrationDetailsTable(lowPowerAuxReg);

            return true;
        }

        /// <summary>
        /// Registers the un licensed LPAUX.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="registrationDisposition">The registration disposition.</param>
        /// <param name="userId">The user identifier.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        private bool RegisterUnLicensedLPAux(Parameters parameters, RegistrationDisposition registrationDisposition, string userId, out List<string> errorMessages)
        {
            // Validate Parameters
            if (!this.RegionManagementValidator.ValidateRegisterDeviceRequestUnlicensedLPAux(parameters, out errorMessages))
            {
                return false;
            }

            string registrationTableName = Utils.GetRegionalTableName(Constants.LPAuxRegistrationTable);

            LPAuxRegistration lowPowerAuxReg = new LPAuxRegistration()
            {
                PartitionKey = Utils.RegistrationIdOrg,
                RowKey = registrationDisposition.RegId,
                CallSign = parameters.TvSpectrum,
                Contact = parameters.Contact,
                Disposition = registrationDisposition,
                Event = parameters.Event,
                Registrant = parameters.LPAuxRegistrant,
                Licensed = false,
                UserId = userId,
                VenueName = parameters.Venue,
                WSDBA = Utils.RegistrationIdOrg,
                ULSFileNumber = parameters.ULSFileNumber
            };

            if (parameters.PointsArea != null)
            {
                lowPowerAuxReg.PointsArea = parameters.PointsArea.Select(obj => obj.ToPosition()).ToArray();
            }
            else
            {
                lowPowerAuxReg.QuadrilateralArea = parameters.QuadrilateralArea;
            }

            lowPowerAuxReg.SerializeObjectsToJston();

            var unlicensedData = this.GetLpAuxLicenseInfo();
            if (unlicensedData.FirstOrDefault(obj => obj.ULSFileNumber == lowPowerAuxReg.ULSFileNumber) == null)
            {
                errorMessages.Add(Constants.ErrorMessageULSFileNumberNotMatch);
                return false;
            }

            if (unlicensedData.FirstOrDefault(obj => obj.VenueName == lowPowerAuxReg.VenueName) == null)
            {
                errorMessages.Add(Constants.ErrorMessageULSVenueNameNotMatch);
                return false;
            }

            var ulsMatchingData = unlicensedData.FirstOrDefault(obj => obj.ULSFileNumber == lowPowerAuxReg.ULSFileNumber);
            for (int k = 0; k < parameters.Event.Times.Length; k++)
            {
                if (parameters.Event.Times[k].Start != null && parameters.Event.Times[k].Start.ToDateTime() >= ulsMatchingData.ExpireDate)
                {
                    errorMessages.Add(Constants.ErrorMessageInvalidStartDate);
                    return false;
                }

                if (parameters.Event.Times[k].End != null && parameters.Event.Times[k].End.ToDateTime() > ulsMatchingData.ExpireDate)
                {
                    errorMessages.Add(Constants.ErrorMessageInvalidEndDate);
                    return false;
                }
            }

            // Route to insert incumbent data method in AzureDalc
            string resp = this.DalcIncumbent.InsertIncumbentData(registrationTableName, lowPowerAuxReg);

            if (resp != "Success")
            {
                errorMessages.Add(Constants.ErrorMessageServerError);
                return false;
            }

            this.DalcIncumbent.SaveLPAuxRegistrationDetailsTable(lowPowerAuxReg);

            return true;
        }

        /// <summary>
        /// Builds the registrant information.
        /// </summary>
        /// <param name="registrantData">The registrant data.</param>
        /// <returns>returns System.String.</returns>
        private string BuildRegistrantInformation(Microsoft.Whitespace.Entities.Versitcard.VCard registrantData)
        {
            string registrantInfo = string.Empty;

            if (registrantData != null)
            {
                StringBuilder builder = new StringBuilder();
                var org = registrantData.Org;
                if (org != null)
                {
                    builder.Append(org.OrganizationName);
                }

                if (registrantData.Name != null)
                {
                    builder.Append("|");
                    builder.Append(registrantData.Name.ContactName);
                }

                if (registrantData.Address != null)
                {
                    builder.Append(" |");
                    builder.Append(registrantData.Address.Locality);
                    builder.Append(" |");
                    builder.Append(registrantData.Address.Region);
                    builder.Append(" |");
                    builder.Append(registrantData.Address.Code);
                    builder.Append(" |");
                    builder.Append(registrantData.Address.Country);
                }

                if (registrantData.Telephone != null)
                {
                    builder.Append(" |");

                    for (int i = 0; i < registrantData.Telephone.Length; i++)
                    {
                        builder.Append(registrantData.Telephone[i].TelephoneNumber);
                        builder.Append(",");
                    }
                }

                if (registrantData.Email != null)
                {
                    builder.Append(" |");

                    for (int i = 0; i < registrantData.Email.Length; i++)
                    {
                        builder.Append(registrantData.Email[i].EmailAddress);
                        builder.Append(",");
                    }
                }

                builder.Append(" |");
                registrantInfo = builder.ToString();
            }

            return registrantInfo;
        }
    }
}
