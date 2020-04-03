// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.ServiceModel;
    using Entities;

    /// <summary>
    ///     Interface INativeMethodService
    /// </summary>
    [ServiceContract]
    public interface INativeMethodService
    {
        /// <summary>
        /// Gets the distance.
        /// </summary>
        /// <param name="erp">The ERP.</param>
        /// <param name="haat">The HAAT.</param>
        /// <param name="fieldInDb">The field in database.</param>
        /// <param name="channel">The channel.</param>
        /// <param name="isDigital">if set to <c>true</c> [is digital].</param>
        /// <returns>returns System.Double.</returns>
        [OperationContract]
        float GetDistance(float erp, float haat, float fieldInDb, int channel, bool isDigital);
    }
}
