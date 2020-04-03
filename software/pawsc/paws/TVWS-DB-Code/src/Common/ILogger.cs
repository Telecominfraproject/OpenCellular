// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents the logging level.
    /// </summary>
    public enum LoggingLevel
    {
        /// <summary>
        /// Verbose logging level.
        /// </summary>
        Verbose = 1,

        /// <summary>
        /// Informational logging level.
        /// </summary>
        Informational = 2,

        /// <summary>
        /// Warning logging level.
        /// </summary>
        Warning = 3,

        /// <summary>
        /// Error logging level.
        /// </summary>
        Error = 4
    }

    /// <summary>
    /// Enumerations of all the different message Ids.
    /// </summary>
    /// <remarks>
    /// <list type="bullet">
    /// <description>Each major component within the whitespace DB has a range of message Ids which is
    /// defined as:</description>
    /// <item><description>1 to 99 range reserved generic log messages that are not specific to a 
    /// particular component.</description></item>
    /// <item><description>100 to 199 range reserved for the DALC.</description></item>
    /// <item><description>200 to 299 range reserved for the Terrain Cache.</description></item>
    /// <item><description>300 to 399 range reserved for the White Space Driver.</description></item>
    /// <item><description>400 to 499 range reserved for the PAWS Manager and Service.</description></item>
    /// <item><description>500 to 699 reserved for the PMSE web service.</description></item>
    /// <item><description>600 to 699 reserved for the Region Management Web Service.</description></item>
    /// <item><description>700 to 799 reserved for the User Manager component.</description></item>
    /// <item><description>800 to 899 reserved for the DB Admin Web Service.</description></item>
    /// <item><description>900 to 999 reserved for the DB Sync Poller Worker Service.</description></item>
    /// <item><description>1000 to 1099 reserved for the DB Sync Manager.</description></item>
    /// <item><description>1100 to 1199 reserved for the DB Sync File Worker.</description></item>
    /// <item><description>1200 to 1299 reserved for the Protected Region Sync Worker.</description></item>
    /// <item><description>2000 to 2099 reserved for the Portal.</description></item>
    /// </list>
    /// </remarks>
    public enum LoggingMessageId
    {
        // 1 to 99 contain generic messages that are not tied to a particular component.

        /// <summary>
        /// A Generic log message that is not tied to any component.
        /// </summary>
        GenericMessage = 1,

        // DB Operators messages are from 100 to 199.

        /// <summary>
        /// Generic Data Access Layer Component log message.
        /// </summary>
        DALCGenericMessage = 100,

        // Terrain Cache messages are from 200 to 299.

        /// <summary>
        /// A generic Terrain Cache log message.
        /// </summary>
        TerrainCacheGenericMessage = 200,

        /// <summary>
        /// Indicates that a file has been loaded from blob storage.
        /// </summary>
        TerrainCacheTileLoaded = 201,

        /// <summary>
        /// Indicates that a cached file has been unloaded.
        /// </summary>
        TerrainCacheTileUnLoaded = 202,

        // White Space Driver messages are from 300 to 399.

        /// <summary>
        /// A generic Driver log message.
        /// </summary>
        DriverGenericMessage = 300,

        // PAWS Manager and PAWS Web Service messages are from 400 to 499.

        /// <summary>
        /// A generic Paws log message.
        /// </summary>
        PAWSGenericMessage = 400,

        // PMSE Web Service messages are from 500 to 599.

        /// <summary>
        /// A generic PMSE log message.
        /// </summary>
        PMSEGenericMessage = 500,

        // Region Management Web Service from 600 to 699.

        /// <summary>
        /// A generic Region Management log message.
        /// </summary>
        RegionManagementGenericMessage = 600,

        // User Manager messages are from 700 to 799.

        /// <summary>
        /// A generic User Manager log message.
        /// </summary>
        UserManagerGenericMessage = 700,

        // DB Admin Web Service messages are from 800 to 899.

        /// <summary>
        /// A generic Admin Web Service log message.
        /// </summary>
        DBAdminWebServiceGenericMessage = 800,

        // DB Sync Poller messages from 900 to 999.

        /// <summary>
        /// A generic DB Sync Poller worker log message.
        /// </summary>
        DBSyncPollerGenericMessage = 900,

        // DB Sync Manager messages are from 1000 to 1099.

        /// <summary>
        ///  A generic Registration Sync Manager log message.
        /// </summary>
        RegistrationSyncManagerGenericMessage = 1000,

        /// <summary>
        ///  A generic Registration Sync Manager log message used while reading cdbs files.
        /// </summary>
        RegistrationSyncManagerCdbsFileReadingGenericMessage = 1001,


        // DB Sync File Worker messages are from 1100 to 1199.

        /// <summary>
        /// A generic File Sync Worker message.
        /// </summary>
        DBSyncFileWorkerGenericMessage = 1100,

        // Protected Region Sync Worker messages are from 1200 to 1299.

        /// <summary>
        /// A generic protected region log message.
        /// </summary>
        ProtectedRegionGenericMessage = 1200,

        // ElevationData Ids are from 300 to 399

        /// <summary>The elevation message </summary>
        ElevationMessage = 300,

        /// <summary>The elevation glob directory path </summary>
        ElevationGlobDirectoryPath = 301,

        /// <summary>The elevation glob unsupported tile identifier </summary>
        ElevationGlobUnsupportedTileId = 302,

        // Propagation Ids are from 400 to 499

        /// <summary>The propagation message </summary>
        PropagationMessage = 400,

        /// <summary>The propagation tile not found </summary>
        PropagationTileNotFound = 401,

        /// <summary>The propagation exception </summary>
        PropagationException = 402,

        /// <summary>The propagation calculate attenuation </summary>
        PropagationCalculateAttenuation = 403,

        // Terrain Server Ids are from 600 to 699

        /// <summary>The terrain server message </summary>
        TerrainServerMessage = 600,

        /// <summary>The terrain server started </summary>
        TerrainServerStarted = 601,

        /// <summary>The terrain server listen </summary>
        TerrainServerListen = 602,

        /// <summary>The terrain server cancel </summary>
        TerrainServerCancel = 603,

        /// <summary>
        /// The PMSE synchronize generic message
        /// </summary>
        PmseSyncGenericMessage = 1300,

        /// <summary>
        /// The DTT synchronize generic message
        /// </summary>
        DttSyncGenericMessage = 1400,

        /// <summary>The database cache message</summary>
        DatabaseCacheMessage = 1500,

        /// <summary>The region calculation generic message</summary>
        RegionCalculationGenericMessage = 1600,

        /// <summary>
        /// A Generic log message that is not tied to any event in Portal.
        /// </summary>
        PortalGenericMessage = 2000,

        /// <summary>
        /// User logged in to the portal message
        /// </summary>
        PortalUserLoggedIn = 2002,

        /// <summary>
        /// user session time out message
        /// </summary>
        PortalUserSessionTimedOut = 2003,

        /// <summary>
        /// user logged out from the portal
        /// </summary>
        PortalUserLoggedOut = 2004,

        /// <summary>
        /// new user registered to the portal
        /// </summary>
        PortalUserRegistration = 2005,

        /// <summary>
        /// user updated his profile
        /// </summary>
        PortalUserProfileUpdate = 2010,

        /// <summary>
        /// user requested for access elevation
        /// </summary>
        PortalUserAccessLevelElevationRequest = 2006,

        /// <summary>
        /// user's access level elevation request is approved
        /// </summary>
        PortalUserAccessLevelElevationRequestApproved = 2007,

        /// <summary>
        /// user's access level elevation request is rejected
        /// </summary>
        PortalUserAccessLevelElevationRequestRejected = 2008,

        /// <summary>
        /// user's access level is upgraded by super admin
        /// </summary>
        PortalUserAccessLevelUpgraded = 2009,

        /// <summary>
        /// user's access level is downgraded by super admin
        /// </summary>
        PortalUserAccessLevelDowngraded = 2010,

        /// <summary>
        /// user's access level is changed by super admin
        /// </summary>
        PoratlUserAccessLevelChange = 2011,

        /// <summary>
        /// user excluded device
        /// </summary>
        PortalExcludeId = 2020,

        /// <summary>
        /// user excluded channel
        /// </summary>
        PortalExcludeChannel = 2021,

        /// <summary>
        /// public data download
        /// </summary>
        PortalDataDownload = 2030,

        /// <summary>
        /// Multichannel Video Programming Distributor Registration
        /// </summary>
        PortalMvpdRegistration = 2040,

        /// <summary>
        /// Licensed Low Power Auxiliary Registration
        /// </summary>
        PortalLicensedLpAuxRegistration = 2041,

        /// <summary>
        /// Unlicensed Low Power Auxiliary Registration
        /// </summary>
        PortalUnlicensedLpAuxRegistration = 2042,

        /// <summary>
        /// TempBas Registration
        /// </summary>
        PortalTempBasRegistration = 2043,

        /// <summary>
        /// Exception in Portal
        /// </summary>
        PortalException = 2050,

        /// <summary>
        /// Monitor Get Channel List from Notification Service
        /// </summary>
        MonitorGetChannelList = 2060,

        /// <summary>
        /// Ofcom Evaluation
        /// </summary>
        PortalOfcomEvaluation = 2070,

        /// <summary>
        /// WSD Spectrum Usage Information
        /// </summary>
        PortalWSDInfo = 2080
    }

    /// <summary>
    /// Interface used to log messages.
    /// </summary>
    public interface ILogger
    {
        /// <summary>
        /// Gets or sets the current region code.
        /// </summary>
        int RegionCode { get; set; }

        /// <summary>
        /// Gets or sets the user Id associated with the log message (can be a web user, remote DB Admin, or a worker thread).
        /// </summary>
        string UserId { get; set; }

        /// <summary>
        /// Gets or sets the transaction associated with the log message.
        /// </summary>
        Guid TransactionId { get; set; }

        /// <summary>
        /// Writes the specified message to the log file.
        /// </summary>
        /// <param name="severity">The log message level.</param>
        /// <param name="messageId">The message id.</param>
        /// <param name="message">The message being logged.</param>
        void Log(TraceEventType severity, LoggingMessageId messageId, string message);

        /// <summary>
        /// Logs the specified message if the current configured logging level is less than the passed in log level.
        /// </summary>
        /// <param name="level">The logging level of the new entry (used to determine if message is to be logged).</param>
        /// <param name="userName">The registered user who made the call.</param>
        /// <param name="msgId">The message Id</param>
        /// <param name="logMessage">The message that is to be logged.</param>
        /// <param name="transactionId">The TransactionId that is to be logged.</param>
        void Log(LoggingLevel level, string userName, LoggingMessageId msgId, string logMessage, Guid transactionId);
    }
}
