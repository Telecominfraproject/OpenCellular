// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.ServiceModel;
    using System.ServiceModel.Channels;
    using Microsoft.Whitespace.Entities;

    /// <summary>
    /// Represents Class NativeMethodServiceClient.
    /// </summary>
    public class NativeMethodServiceClient : ClientBase<INativeMethodService>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="NativeMethodServiceClient"/> class.
        /// </summary>
        public NativeMethodServiceClient()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="NativeMethodServiceClient"/> class.
        /// </summary>
        /// <param name="binding">The binding.</param>
        /// <param name="remoteAddress">The remote address.</param>
        public NativeMethodServiceClient(Binding binding, EndpointAddress remoteAddress)
            : base(binding, remoteAddress)
        {
        }

        /// <summary>
        /// Gets the distance.
        /// </summary>
        /// <param name="erp">The ERP.</param>
        /// <param name="haat">The HAAT.</param>
        /// <param name="fieldInDb">The field in database.</param>
        /// <param name="channel">The channel.</param>
        /// <param name="isDigital">if set to <c>true</c> [is digital].</param>
        /// <returns>returns System.Double.</returns>
        public float GetDistance(float erp, float haat, float fieldInDb, int channel, bool isDigital)
        {
            return Channel.GetDistance(erp, haat, fieldInDb, channel, isDigital);
        }
    }
}
