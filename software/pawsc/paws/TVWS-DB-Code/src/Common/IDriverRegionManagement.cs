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
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents the driver's interface used by the Region Management web service.
    /// </summary>
    public interface IDriverRegionManagement
    {
        /// <summary>
        /// Adds the incumbents
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="id">The parameter user id.</param>
        /// <returns>returns RegionManagementResponse.</returns>
        RegionManagementResponse RegisterDevice(Parameters parameters, string id);

        /// <summary>
        /// Deletes the incumbents
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns RegionManagementResponse.</returns>
        RegionManagementResponse DeleteIncumbentInfo(Parameters parameters);

        /// <summary>
        /// Gets the incumbents
        /// </summary>
        /// <param name="incumbentType">The incumbentType.</param>
        /// <param name="id">The parameter user id.</param>
        /// <returns>returns list.</returns>
        object[] GetIncumbents(string incumbentType, string id);

        /// <summary>
        /// Gets the incumbents
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns list.</returns>
        List<ProtectedDevice> GetDeviceList(Parameters parameters);

        /// <summary>
        /// Adds to the list of excluded Ids
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns RegionManagementResponse.</returns>
        RegionManagementResponse ExcludeIds(Parameters parameters);

        /// <summary>
        /// Adds to the list of excluded Regions
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns RegionManagementResponse.</returns>
        RegionManagementResponse ExcludeChannels(Parameters parameters);

        /// <summary>
        /// Returns only the free channels at the specified location.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>Returns only the free channels.</returns>
        RegionManagementResponse GetChannelList(Parameters parameters);

        /// <summary>
        /// Gets the protected entity.
        /// </summary>
        /// <param name="incumbentType">Type of the incumbent.</param>
        /// <returns>returns List{ProtectedEntityWithEvent}.</returns>
        List<ProtectedEntityWithEvent> GetProtectedEntityWithEvents(string incumbentType);

        /// <summary>
        /// Gets the protected entity.
        /// </summary>
        /// <param name="incumbentType">Type of the incumbent.</param>
        /// <returns>returns List{ProtectedEntity}.</returns>
        List<ProtectedEntity> GetProtectedEntity(string incumbentType);

        /// <summary>
        /// Gets the authorized devices.
        /// </summary>
        /// <returns>returns List{AuthorizedDeviceRecord}.</returns>
        List<AuthorizedDeviceRecord> GetAuthorizedDevices();

        /// <summary>
        /// Gets the LPAUX license information.
        /// </summary>
        /// <param name="licenseType">Type of the license.</param>
        /// <returns>returns List{LPAUX LicenseInfo}.</returns>
        List<LpAuxLicenseInfo> GetLpAuxLicenseInfo(string licenseType = null);

        /// <summary>
        /// Gets the TV stations.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns stations.</returns>
        RegionManagementResponse SearchMVPDCallSigns(Parameters parameters);

        /// <summary>
        /// Gets MVPD Callsign information
        /// </summary>
        /// <param name="requestedCallsign">callsign</param>
        /// <returns>returns callsign information</returns>
        RegionManagementResponse GetMVPDCallSignInfo(string requestedCallsign);

        /// <summary>
        /// Gets the contour data.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns Contours.</returns>
        List<ContourDetailsInfo> GetContourData(Parameters parameters);
    }
}
