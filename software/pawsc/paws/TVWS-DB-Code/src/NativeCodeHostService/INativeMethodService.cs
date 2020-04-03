// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace NativeCodeHostService
{
    using System.ServiceModel;

    /// <summary>
    /// Interface INativeMethodService
    /// </summary>
    [ServiceContract]
    internal interface INativeMethodService
    {
        /// <summary>
        /// Gets the distance.
        /// </summary>
        /// <param name="erp">The erp.</param>
        /// <param name="haat">The haat.</param>
        /// <param name="fieldInDb">The field in database.</param>
        /// <param name="channel">The channel.</param>
        /// <param name="isDigital">if set to <c>true</c> [is digital].</param>
        /// <returns>returns System.Single.</returns>
        [OperationContract]
        float GetDistance(float erp, float haat, float fieldInDb, int channel, bool isDigital);
    }
}
