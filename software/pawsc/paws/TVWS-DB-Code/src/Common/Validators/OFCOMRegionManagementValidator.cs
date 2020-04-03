// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.Validators
{
    using System;
    using System.Collections.Generic;
    using Entities;
    using Utilities;

    /// <summary>
    ///     Represents Class OFCOMRegionManagementValidator.
    /// </summary>
    public class OFCOMRegionManagementValidator : BaseRegionManagementValidator
    {
        /// <summary>
        ///     Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public override bool ValidateRequiredIncumbentType(Parameters parameters, out List<string> errorMessages)
        {
            var valResult = base.ValidateRequiredIncumbentType(parameters, out errorMessages);
            if (valResult)
            {
                if (!this.IncumbentType(parameters, out errorMessages))
                {
                    return false;
                }
            }

            return valResult;
        }

        /// <summary>
        ///     Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public override bool ValidateGetChannelListRequest(Parameters parameters, out List<string> errorMessages)
        {
            if (!this.Location(parameters, out errorMessages))
            {
                return false;
            }

            errorMessages = new List<string>();

            ////if (parameters == null || parameters.DeviceId == null || parameters.DeviceId == string.Empty)
            ////{
            ////    errorMessages.Add(Constants.ErrorMessageDeviceIdRequired);
            ////    return false;
            ////}

            if (string.IsNullOrWhiteSpace(parameters.RequestType))
            {
                errorMessages.Add(Constants.ErrorMessageParametersRequestTypeRequired);
                return false;
            }

            ////if (string.IsNullOrWhiteSpace(parameters.UniqueId))
            ////{
            ////    errorMessages.Add(Constants.ErrorMessageParametersUniqueIdRequired);
            ////    return false;
            ////}

            ////if (parameters.DeviceDescriptor == null)
            ////{
            ////    errorMessages.Add(Constants.ErrorMessageParametersDeviceDescriptorRequired);
            ////    return false;
            ////}

            ////if (string.IsNullOrWhiteSpace(parameters.DeviceDescriptor.EtsiDeviceCategory))
            ////{
            ////    errorMessages.Add(Constants.ErrorMessageParametersDeviceCategoryRequired);
            ////    return false;
            ////}

            ////if (string.IsNullOrWhiteSpace(parameters.DeviceDescriptor.EtsiEnDeviceType))
            ////{
            ////    errorMessages.Add(Constants.ErrorMessageParametersEtsiDeviceTypeRequired);
            ////    return false;
            ////}

            if (parameters.Antenna != null && parameters.Antenna.HeightType == HeightType.None)
            {
                errorMessages.Add(Constants.ErrorMessageParametersHeightTypeRequired);
                return false;
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        ///     Incumbents the type.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool IncumbentType(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.IncumbentType == null || parameters.IncumbentType == string.Empty)
            {
                errorMessages.Add(Constants.ErrorMessageIncumbentTypeRequired);
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        ///     Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool Location(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.Location == null)
            {
                errorMessages.Add(Constants.ErrorMessageLocationRequired);
            }
            else if (parameters.Location.Point != null && parameters.Location.Point.Center != null)
            {
                if ((parameters.Location.Point.Center.Latitude == null) || (parameters.Location.Point.Center.Latitude == string.Empty))
                {
                    errorMessages.Add(Constants.ErrorMessageLatitudeMissing);
                }
                else if ((parameters.Location.Point.Center.Longitude == null) || (parameters.Location.Point.Center.Longitude == string.Empty))
                {
                    errorMessages.Add(Constants.ErrorMessageLongitudeMissing);
                }
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the get incumbents request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public override bool ValidateGetIncumbentsRequest(Parameters parameters, out List<string> errorMessages)
        {
            if (!this.ValidateRequiredIncumbentType(parameters, out errorMessages))
            {
                return false;
            }

            errorMessages = new List<string>();

            Entities.IncumbentType incumbentType = Conversion.ToIncumbentType(parameters.IncumbentType);
            if (incumbentType == Entities.IncumbentType.None)
            {
                errorMessages.Add(Constants.ErrorMessageInvalidIncumbentType);
                return false;
            }

            if (incumbentType == Entities.IncumbentType.Fixed || incumbentType == Entities.IncumbentType.Mode_1 || incumbentType == Entities.IncumbentType.Mode_2)
            {
                return errorMessages.Count == 0;                
            }
            else
            {
                errorMessages.Add(Constants.ErrorMessageInvalidIncumbentType);
                return false;
            }            
        }
    }
}
