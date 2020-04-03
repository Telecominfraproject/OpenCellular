// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Entities;

    /// <summary>
    /// Represents a interface used for performing contour calculations.
    /// </summary>
    public interface IRegionCalculation
    {
        /// <summary>
        /// Calculates the contour of the specified incumbent.
        /// </summary>
        /// <param name="incumbent">Incumbent that is to be calculated.</param>
        /// <returns>Returns the contour of the calculations.</returns>
        Contour CalculateContour(Incumbent incumbent);

        /// <summary>
        /// Calculates the Hat of the incumbent.
        /// </summary>
        /// <param name="incumbent">The incumbent that is to be calculated.</param>
        /// <returns>Returns the Hat</returns>
        RadialHAT CalculateRadialHAT(Incumbent incumbent);

        /// <summary>
        /// Calculates the station HAAT.
        /// </summary>
        /// <param name="sourceLocation">The source location.</param>
        /// <returns>returns RadialHAT.</returns>
        RadialHAT CalculateStationHAAT(Location sourceLocation);

        /// <summary>
        /// Determines the channel availability for each channel at the specified position.
        /// </summary>
        /// <param name="position">Location that is determine channel availability.</param>
        /// <returns>Returns all of the channels and their availability status.</returns>
        ChannelInfo[] GetChannelAvailabilty(Position position);

        /// <summary>
        /// Gets the free channels.
        /// </summary>
        /// <param name="wsdInfo">The WSD information.</param>
        /// <returns>returns GetChannelsResponse.</returns>
        GetChannelsResponse GetFreeChannels(Incumbent wsdInfo);

        /// <summary>
        /// Returns the devices at the specified location.
        /// </summary>
        /// <param name="parameters">parameters for get device list.</param>
        /// <returns>Returns the device list.</returns>
        List<ProtectedDevice> GetDeviceList(Parameters parameters);
    }
}
