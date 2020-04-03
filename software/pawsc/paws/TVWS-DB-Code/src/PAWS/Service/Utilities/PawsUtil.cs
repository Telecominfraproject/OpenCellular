// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.PAWS.Service.Utilities
{
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;

    public class PawsUtil
    {
        /// <summary>
        /// variable to have method name for auditor
        /// </summary>
        private const string PawsRegistrationAuditMethodName = "Paws Registration Audit";

        /// <summary>
        /// variable to have method name for auditor
        /// </summary>
        private const string PawsInitializationAuditMethodName = "Paws Initialization Audit";

        /// <summary>
        /// variable to have method name for auditor
        /// </summary>
        private const string PawsDeviceValidationAuditMethodName = "Paws Device Validation Audit";

        /// <summary>
        /// variable to have method name for auditor
        /// </summary>
        private const string PawsSpectrumUsageNotifyAuditMethodName = "Paws Spectrum Usage Notify Audit";

        /// <summary>
        /// variable to have method name for auditor
        /// </summary>
        private const string PawsAvailableSpectrumQueryAuditMethodName = "Paws Available Spectrum Audit";

        /// <summary>
        /// variable to have method name for auditor
        /// </summary>
        private const string PawsAvailableSpectrumQueryBatchAuditMethodName = "Paws Available Spectrum Batch Audit";

        /// <summary>
        /// variable to have method name for auditor
        /// </summary>
        private const string PawsInterferenceQueryAuditMethodName = "Paws Interference Query Audit";

        /// <summary>
        /// Gets corresponding PAWS AuditId and audit method name for the given requested method name.
        /// </summary>
        /// <param name="requestMethodName">PAWS request method name.</param>
        /// <param name="auditMethodName">PAWS audit method name.</param>
        /// <returns>PWAS AuditId</returns>
        public static AuditId GetAuditId(string requestMethodName, out string auditMethodName)
        {
            AuditId auditId = AuditId.PAWSInvalidMethod;
            auditMethodName = requestMethodName;

            switch (requestMethodName)
            {
                case Constants.MethodNameRegister:
                    auditId = AuditId.PAWSRegistrationReq;
                    auditMethodName = PawsRegistrationAuditMethodName;
                    break;

                case Constants.MethodNameInit:
                    auditId = AuditId.PAWSInitReq;
                    auditMethodName = PawsInitializationAuditMethodName;
                    break;

                case Constants.MethodNameValidateDevice:
                    auditId = AuditId.PAWSValidReq;
                    auditMethodName = PawsDeviceValidationAuditMethodName;
                    break;

                case Constants.MethodNameNotify:
                    auditId = AuditId.PAWSSpectrumUseNotify;
                    auditMethodName = PawsSpectrumUsageNotifyAuditMethodName;
                    break;

                case Constants.MethodNameAvailableSpectrum:
                    auditId = AuditId.PAWSAvailableSpectrumReq;
                    auditMethodName = PawsAvailableSpectrumQueryAuditMethodName;
                    break;

                case Constants.MethodNameAvailableSpectrumBatch:
                    auditId = AuditId.PAWSAvailableSpectrumBatchReq;
                    auditMethodName = PawsAvailableSpectrumQueryBatchAuditMethodName;
                    break;

                case Constants.MethodNameInterferenceQuery:
                    auditId = AuditId.PAWSInterferenceQueryReq;
                    auditMethodName = PawsInterferenceQueryAuditMethodName;
                    break;
            }

            return auditId;
        }
    }
}
