// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.Validators
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;

    /// <summary>
    /// implements the IPMSEValidator
    /// </summary>
    public class FCCPMSEValidator : IPMSEValidator
    {
        /// <summary>
        /// validates the protected device registration info
        /// </summary>
        /// <param name="parameters"> parameters of PMSE Request</param>
        /// <param name="errList"> List of Error</param>
        /// <returns>Boolean value</returns>
        public bool IsProtectedDeviceValid(Parameters parameters, out List<string> errList)
        {
            errList = new List<string>();

            if (parameters.TvSpectrum == null)
            {
                errList.Add(Constants.LPAuxRegistration + " - " + Constants.ErrorMessageCallSignAndChannelRequired + " Missing");
            }
            
            if (parameters.PointsArea != null && parameters.QuadrilateralArea != null)
            {
                errList.Add(Constants.LPAuxRegistration + " - " + Constants.ErrorMessagePointAreaQuadAreaMutuallyExclusive);
            }
            
            if (parameters.PointsArea == null && parameters.QuadrilateralArea == null)
            {
                errList.Add(Constants.LPAuxRegistration + " - " + Constants.ErrorMessagePointAreaOrQuadAreaRequired + " Missing");
            }
            
            if (parameters.Event != null && parameters.Event.Channels == null)
            {
                errList.Add(Constants.LPAuxRegistration + " - " + Constants.ErrorMessageChannelRequired + " Missing");
            }
            
            if (parameters.Event.Channels != null && parameters.Event.Channels.Length == 0)
            {
                errList.Add(Constants.LPAuxRegistration + " - " + Constants.ErrorMessageChannelsRequired + " Missing");
            }

            if (parameters.Event != null && parameters.Event.Times == null)
            {
                errList.Add(Constants.LPAuxRegistration + " - " + Constants.ErrorMessageChannelsRequired + " Missing");
            }
            
            if (parameters.Registrant != null && parameters.Registrant.Organization != null && parameters.Registrant.Organization.Text == null)
            {
                errList.Add(Constants.LPAuxRegistration + " - " + Constants.ErrorMessageOrgNameMissing + " Missing");
            }
            
            if (parameters.Registrant == null)
            {
                errList.Add(Constants.LPAuxRegistration + " - " + Constants.ErrorMessageRegistrantMissing + " Missing");
            }

            if (parameters.Event != null && parameters.Event.Times != null && parameters.Event.Times.Any())
            {
                for (int k = 0; k < parameters.Event.Times.Length; k++)
                {
                    if (parameters.Event.Times[k].Stamp == null)
                    {
                        errList.Add(Constants.LPAuxRegistration + " - " + Constants.ErrorMessageEventTimesStampRequired + "Missing");
                    }
                    else if (parameters.Event.Times[k].Start == null)
                    {
                        errList.Add(Constants.LPAuxRegistration + " - " + Constants.ErrorMessageEventTimesStartRequired + "Missing");
                    }
                    else if (parameters.Event.Times[k].End == null)
                    {
                        errList.Add(Constants.LPAuxRegistration + " - " + Constants.ErrorMessageEventTimesEndRequired + "Missing");
                    }
                }
            }

            if (errList.Count == 0)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}
