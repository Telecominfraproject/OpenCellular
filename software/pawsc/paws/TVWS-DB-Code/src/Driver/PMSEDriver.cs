// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Driver
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Xml;
    using System.Xml.Linq;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Validators;
    using Microsoft.Whitespace.Dalc;
    using Microsoft.Whitespace.Entities;

    /// <summary>
    /// Represents Class PMSEDriver.
    /// </summary>
    public class PMSEDriver : IDriverPMSE
    {
        /// <summary>Gets or sets ILogger Interface</summary>
        [Dependency]
        public ILogger Logger { get; set; }

        /// <summary>Gets or sets the PMSE DALC.</summary>
        /// <value>The PMSE DALC.</value>
        [Dependency]
        public IDalcPMSE PMSEDalc { get; set; }

        /// <summary>Gets or sets the IDALCPaws.</summary>
        [Dependency]
        public IDalcPaws PawsDalc { get; set; }

        /// <summary>Gets or sets the common DALC.</summary>
        /// <value>The common DALC.</value>
        [Dependency]
        public IDalcCommon CommonDalc { get; set; }

        /// <summary>
        /// Gets or sets IRegionCalculation Interface
        /// </summary>
        [Dependency]
        public IPMSEValidator PMSEValidator { get; set; }

        #region IDriverPMSE methods

        /// <summary>Registered the specified device.</summary>
        /// <param name="parameters">Registration parameters</param>
        /// <param name="userId">User ID</param>
        /// <returns>PMSE response object</returns>
        public PMSEResponse RegisterProtectedDevice(Parameters parameters, string userId)
        {
            string logMethodName = "PMSEDriver.RegisterProtectedDevice(Parameters parameters)";
            PMSEResponse response = new PMSEResponse();
            try
            {
                // Begin Log transaction
                this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

                string wsdba = Utils.RegistrationIdOrg;
                DateTime settingsDate = this.GetDateSequenceFromSettingsTable();
                int sequenceNumber = this.GetSequenceFromSettingsTable();
                string regSeqNumber = sequenceNumber.ToString("0000000");
                RegistrationDisposition regDisposition = new RegistrationDisposition();
                regDisposition.RegDate = settingsDate.ToString("yyyy'-'MM'-'dd'T'HH':'mm':'ss'Z'", DateTimeFormatInfo.InvariantInfo);
                regDisposition.RegId = string.Format("{0:yyMMdd}", settingsDate) + Utils.RegistrationIdOrg + regSeqNumber;
                regDisposition.Action = 1;
                Check.IsNotNull<ParametersBase>(parameters, Constants.ParameterNameParameters);
                List<string> errorList;

                if (this.PMSEValidator.IsProtectedDeviceValid(parameters, out errorList))
                {
                    string registrationTableName = Utils.GetRegionalTableName(Constants.LPAuxRegistrationTable);

                    LPAuxRegistration lowPowerAuxReg = new LPAuxRegistration()
                    {
                        PartitionKey = wsdba,
                        RowKey = regDisposition.RegId,
                        AuxTvSpectrum = JsonSerialization.SerializeObject(parameters.TvSpectrum),
                        AuxContact = JsonSerialization.SerializeObject(parameters.Contact),
                        AuxRegDisposition = JsonSerialization.SerializeObject(regDisposition),
                        AuxEvent = JsonSerialization.SerializeObject(parameters.Event),
                        AuxRegistrant = JsonSerialization.SerializeObject(parameters.Registrant),
                        Disposition = regDisposition,
                        Licensed = false,
                        UserId = userId,
                        VenueName = parameters.Venue,
                        WSDBA = wsdba
                    };

                    if (parameters.PointsArea != null && parameters.QuadrilateralArea == null)
                    {
                        lowPowerAuxReg.AuxPointsArea = JsonSerialization.SerializeObject(parameters.PointsArea);
                    }
                    else
                    {
                        lowPowerAuxReg.AuxQuadPoints = JsonSerialization.SerializeObject(parameters.QuadrilateralArea);
                    }

                    this.PMSEDalc.RegisterDevice(lowPowerAuxReg);

                    response = new PMSEResponse
                    {
                        Result = new Result
                        {
                            Message = "Lp-Aux Unlicensed Device registration is successful.",
                        }
                    };

                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);
                    return response;
                }
                else
                {
                    StringBuilder builder = new StringBuilder();
                    foreach (string value in errorList)
                    {
                        builder.Append(value);
                        builder.Append(",");
                    }

                    response = ErrorHelper.CreatePMSEErrorResponse(Constants.LPAuxRegistration, Constants.ErrorLPAUXDataMissing);
                    response.Error.Data = response.Error.Data + ":" + builder.ToString();
                    ////response.Result = new Result { Message = builder.ToString(), };
                }
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                response = ErrorHelper.CreatePMSEErrorResponse(Constants.LPAuxRegistration, e.ToString());
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);
            return response;
        }

        /// <summary>Cancels the specified registration.</summary>
        /// <param name="parameters">Id of the registration.</param>
        /// <returns>PMSE response object</returns>
        public PMSEResponse CancelRegistration(Parameters parameters)
        {
            string logMethodName = "PMSEDriver.CancelRegistration(Parameters parameters)";

            // Begin Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

            PMSEResponse response = null;
            Check.IsNotNull<ParametersBase>(parameters, Constants.ParameterNameParameters);
            try
            {
                if (parameters.LPAUXRegId != null)
                {
                    this.PMSEDalc.CancelRegistrations(parameters.LPAUXRegId);
                    response = new PMSEResponse
                               {
                                   Result = new Result
                                            {
                                                Message = "Successfully Cancelled LpAux Unlicensed Registration"
                                            }
                               };
                }
                else
                {
                    response = ErrorHelper.CreatePMSEErrorResponse(Constants.LPAuxRegistration, Constants.ErrorMessageRegIdRequired);
                }
            }
            catch (Exception e)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                response = ErrorHelper.CreatePMSEErrorResponse(Constants.LPAuxRegistration, e.ToString());
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);
            return response;
        }

        /// <summary>Retrieves the specified registration.</summary>
        /// <param name="id">Id of the registration.</param>
        /// <returns>PMSE Response</returns>
        public PMSEResponse GetRegistration(string id)
        {
            string logMethodName = "PMSEDriver.GetRegistration(sring id)";
            PMSEResponse response = new PMSEResponse();

            // Begin Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

            // ToDo: implement
            try
            {
                Result result = new Result();
                response.Result = result;

                response.Result.LPAUXUnlicensedList = this.PMSEDalc.GetLPAuxRegistration(id);
                if (response.Result.LPAUXUnlicensedList == null || response.Result.LPAUXUnlicensedList.Count == 0)
                {
                    response = ErrorHelper.CreatePMSEErrorResponse(Constants.LPAuxRegistration, Constants.ErrorLPAUXDataMissing);
                    response.Error.Data = response.Error.Data + ": Registration ID :" + id + ": doesn't exist.";
                }
                else
                {
                    response.Result.LPAUXUnlicensedList[0].DeSerializeObjectsFromJson();
                }
            }
            catch (Exception e)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                response = ErrorHelper.CreatePMSEErrorResponse(Constants.LPAuxRegistration, e.ToString());
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);
            return response;
        }

        /// <summary>Returns an array of all the PMSE registrations.</summary>
        /// <returns>Array of PMSE response.</returns>
        public PMSEResponse GetRegistration()
        {
            string logMethodName = "PMSEDriver.GetRegistration(sring id)";
            PMSEResponse response = new PMSEResponse();

            // Begin Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

            try
            {
                Result result = new Result();
                response.Result = result;
                response.Result.LPAUXUnlicensedList = this.PMSEDalc.GetLPAuxRegistrations();
            }
            catch (Exception e)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                response = ErrorHelper.CreatePMSEErrorResponse(Constants.LPAuxRegistration, e.ToString());
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);
            return response;
        }

        /// <summary>
        /// Return Settings Date
        /// </summary>
        /// <returns>Settings Date</returns>
        public DateTime GetDateSequenceFromSettingsTable()
        {
            string currentSettingsDate = this.CommonDalc.FetchEntity<Settings>(Utils.GetRegionalTableName(Constants.SettingsTable), new { RowKey = Constants.SettingsDateKey })[0].ConfigValue;

            DateTime settingsDate = DateTime.Parse(currentSettingsDate, CultureInfo.InvariantCulture);
            if (settingsDate.Date != DateTime.Now.Date || settingsDate.Month != DateTime.Now.Month || settingsDate.Year != DateTime.Now.Year)
            {
                Settings settings = new Settings();
                settings.PartitionKey = Constants.SettingsPartitionKey;
                settings.RowKey = Constants.SettingsDateKey;
                settings.ConfigValue = DateTime.Now.Date.ToString(Utils.Configuration[Constants.DateFormat], DateTimeFormatInfo.InvariantInfo);
                this.PawsDalc.UpdateSettingsDateSequence(settings);
                Settings settingsSequence = new Settings();
                settingsSequence.PartitionKey = Constants.SettingsPartitionKey;
                settingsSequence.RowKey = Constants.SettingsSequenceKey;
                settingsSequence.ConfigValue = "0";
                this.PawsDalc.UpdateSettingsDateSequence(settingsSequence);
            }

            settingsDate = DateTime.Now;
            return settingsDate;
        }

        /// <summary>
        /// Return Settings Date
        /// </summary>
        /// <returns>Settings Date</returns>
        public int GetSequenceFromSettingsTable()
        {
            int sequenceNumber = Convert.ToInt32(this.CommonDalc.FetchEntity<Settings>(Utils.CurrentRegionPrefix + Constants.SettingsTable, new { RowKey = Constants.SettingsSequenceKey })[0].ConfigValue) + 1;
            Settings seqSettings = new Settings();
            seqSettings.PartitionKey = Constants.SettingsPartitionKey;
            seqSettings.RowKey = Constants.SettingsSequenceKey;
            seqSettings.ConfigValue = sequenceNumber.ToString();
            this.PawsDalc.UpdateSettingsDateSequence(seqSettings);
            return sequenceNumber;
        }

        #endregion IDriverPMSE methods
    }
}
