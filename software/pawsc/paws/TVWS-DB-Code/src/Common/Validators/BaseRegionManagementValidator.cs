// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.Validators
{
    using System.Collections.Generic;
    using System.Linq;
    using System.Text.RegularExpressions;
    using Entities;
    using Practices.EnterpriseLibrary.Validation;
    using Practices.EnterpriseLibrary.Validation.Validators;
    using Practices.Unity;

    /// <summary>
    /// Class BaseRegionManagementValidator
    /// </summary>
    public class BaseRegionManagementValidator : IRegionManagementValidator
    {
        /// <summary>
        /// Gets or sets ILogger Interface
        /// </summary>
        [Dependency]
        public IDalcIncumbent IncumbentDalc { get; set; }

        /// <summary>
        /// Gets or sets the common DALC.
        /// </summary>
        /// <value>The common DALC.</value>
        [Dependency]
        public IDalcCommon CommonDalc { get; set; }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public virtual bool ValidateAddIncumbentRequestMVPD(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public virtual bool ValidateAddIncumbentRequestTBAS(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public virtual bool ValidateAddIncumbentRequestLPAux(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public virtual bool ValidateDeleteIncumbentRequest(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public virtual bool ValidateRequiredIncumbentType(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the get incumbents request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public virtual bool ValidateGetIncumbentsRequest(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public virtual bool ValidateExcludeChannels(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public virtual bool ValidateExcludeIds(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public virtual bool ValidateGetDeviceList(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public virtual bool ValidateGetChannelListRequest(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the add incumbent request unlicensed LPAUX.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public virtual bool ValidateRegisterDeviceRequestUnlicensedLPAux(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the get contour data request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public virtual bool ValidateGetContourDataRequest(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Determines whether the specified object is valid.
        /// </summary>
        /// <typeparam name="T">Generic Type</typeparam>
        /// <param name="obj">The object.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if the specified object is valid; otherwise, <c>false</c>.</returns>
        public bool IsValid<T>(T obj, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            Validator cusValidator = new ObjectValidator();
            ValidationResults valResults = cusValidator.Validate(obj);
            if (!valResults.IsValid)
            {
                errorMessages.AddRange(valResults.Select(objVal => objVal.Message));
                return false;
            }

            return true;
        }
    }
}
