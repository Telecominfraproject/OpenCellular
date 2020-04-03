// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    /// <summary>
    /// Utility class for storing constants
    /// </summary>
    public class Constants
    {
        /// <summary>
        /// Constant value of the string User Profile Table
        /// </summary>
        public const string UserProfileTable = "UserProfile";

        /// <summary>
        ///  Constant value for <see cref="WebpagesOauthMembershipTable"/> value
        /// </summary>
        public const string WebpagesOauthMembershipTable = "WebpagesOauthMembership";

        /// <summary>
        /// Constant value for <see cref="ApplicationName"/> value
        /// </summary>
        public const string ApplicationName = "TV WhiteSpaces";

        public const string AuthorityAccessTable = "AuthorityAccess";

        public const string AuthorityTable = "Authority";

        public const string AccessLevelTable = "AccessLevel";

        public const string EmailScope = "wl.emails";

        public const string AddressScope = "wl.postal_addresses";

        public const string MicrosoftProvider = "microsoft";

        public const string LiveRequestScopes = "wl.emails,wl.postal_addresses";

        public const string MethodAddIncumbent = "RegisterDevice";

        public const string MethodGetChannelList = "GetChannelList";

        public const string MethodGetIncumbents = "GetIncumbents";

        public const string MethodDeleteIncumbent = "DeleteIncumbent";

        public const string MethodGetFreeChannels = "GetFreeChannels";

        public const string MethodExcludeChannel = "ExcludeChannel";

        public const string MethodExcludeIds = "ExcludeIds";

        public const string MethodGetDeviceList = "GetDeviceList";

        public const string MethodGetPublicData = "GetPublicData";

        public const string MethodGetPublicDataWithEvents = "GetPublicDataWithEvents";

        public const string MethodGetAuthorizedDeviceModels = "GetAuthorizedDeviceModels";

        public const string MethodSearchTvStations = "SearchMvpdCallSigns";

        public const string MethodGetMVPDCallSignInfo = "GetMVPDCallSignInfo";

        public const string MethodGetULSFileNumbers = "GetULSFileNumbers";

        public const string MethodGetULSCallSigns = "GetULSCallSigns";

        public const string DefaultRequestApi = "RegionApi";

        public const string SuccessfullDeviceRegistration = "Device Registered successfully.";

        public const string ExcludedIdSuccessfully = "Excluded Id inserted successfully";

        public const string ExcludedChannelSuccessfully = "Channel Excluded Successfully.";

        public const string ExcludedIdsTableName = "ExcludedIds";

        public const string ExcludedChannelsTableName = "ExcludedChannels";

        public const string Paws = "paws";

        public const string NoDataAvailable = "No Entries";
    }
}
