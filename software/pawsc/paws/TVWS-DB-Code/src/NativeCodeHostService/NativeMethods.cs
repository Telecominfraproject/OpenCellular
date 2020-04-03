// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace NativeCodeHostService
{
    using Microsoft.Whitespace.InterpolationLibrary;

    public class NativeMethods : INativeMethodService
    {
        #region Implementation of INativeMethodService

        /// <summary>
        /// Gets the distance.
        /// </summary>
        /// <param name="erp">The erp.</param>
        /// <param name="haat">The haat.</param>
        /// <param name="fieldInDb">The field in database.</param>
        /// <param name="channel">The channel.</param>
        /// <param name="isDigital">if set to <c>true</c> [is digital].</param>
        /// <returns>returns System.Single.</returns>
        public float GetDistance(float erp, float haat, float fieldInDb, int channel, bool isDigital)
        {
            CurveCalculator curveCalculator = new CurveCalculator();
            float distance = curveCalculator.CalculateCurveValue(erp, haat, channel, fieldInDb, 0.0f, 2, isDigital);
            return distance;
        }

        #endregion
    }
}
