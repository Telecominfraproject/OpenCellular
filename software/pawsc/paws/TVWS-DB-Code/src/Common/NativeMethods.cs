// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using Entities;

    /// <summary>
    /// Represents Class NativeMethods.
    /// </summary>
    public static class NativeMethods
    {
        /// <summary>
        /// native service client
        /// </summary>
        private static NativeMethodServiceClient ServiceClient;

        /// <summary>
        /// Gets the distance.
        /// </summary>
        /// <param name="erp">The ERP.</param>
        /// <param name="haat">The HAAT.</param>
        /// <param name="ichannel">The channel.</param>
        /// <param name="field">The field.</param>
        /// <param name="isDigital">if set to <c>true</c> [is digital].</param>
        /// <returns>returns System.Double.</returns>
        public static float GetDistance(float erp, float haat, int ichannel, float field, bool isDigital)
        {
            if (ServiceClient == null)
            {
                ServiceClient = Utils.GetNativeServiceClient();
            }

            var distance = ServiceClient.GetDistance(erp, haat, field, ichannel, isDigital);
            return distance;
        }
    }
}
