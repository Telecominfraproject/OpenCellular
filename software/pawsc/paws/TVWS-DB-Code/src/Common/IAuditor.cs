// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Defines the status of an audit operation.
    /// </summary>
    public enum AuditStatus
    {
        /// <summary>
        /// Indicates that an auditable operation failed.
        /// </summary>
        Failure = 0,

        /// <summary>
        /// Indicates that an auditable operation completed successfully.
        /// </summary>
        Success = 1
    }

    /// <summary>
    /// Defines all of the whitespace audit types.
    /// </summary>
    public enum AuditId
    {
        /// <summary>
        /// Indicates an Invalid PAWS request method.
        /// </summary>
        PAWSInvalidMethod,

        /// <summary>
        /// Indicates that a PAWS Initialize Request has completed.  The UserId field 
        /// will be populated with the requestor's Id (device Id or serial #) and the 
        /// message field will be populated with all of the parameters passed into the request.
        /// </summary>
        PAWSInitReq,

        /// <summary>
        /// Indicates that a PAWS Registration Request has completed.  The UserId field 
        /// will be populated with the requestor's Device Id and the message field 
        /// will be populated with all of the parameters passed into the request.
        /// </summary>
        PAWSRegistrationReq,

        /// <summary>
        /// Indicates that a PAWS Registration Request has completed.  The UserId field 
        /// will be populated with the requestor's Device Id and the message field 
        /// will be populated with all of the parameters passed into the request.
        /// </summary>
        PAWSAvailableSpectrumReq,

        /// <summary>
        /// Indicates that a PAWS Batch Registration Request has completed.  The UserId field 
        /// will be populated with the requestor's Device Id and the message field 
        /// will be populated with all of the parameters passed into the request.
        /// </summary>
        PAWSAvailableSpectrumBatchReq,

        /// <summary>
        /// Indicates that a Spectrum Use Request has completed.  The UserId field 
        /// will be populated with the requestor's Device Id and the message field 
        /// will be populated with all of the parameters passed into the request.
        /// </summary>
        PAWSSpectrumUseNotify,

        /// <summary>
        /// Indicates that an Interference Query Request has completed.  The UserId field 
        /// will be populated with the requestor's Device Id and the message field 
        /// will be populated with all of the parameters passed into the request.
        /// </summary>
        PAWSInterferenceQueryReq,

        /// <summary>
        /// Indicates that a PAWS Validate Request has completed.  The UserId field 
        /// will be populated with the requestor's Device Id and the message field 
        /// will be populated with all of the parameters passed into the request.
        /// </summary>
        PAWSValidReq,

        /// <summary>
        /// Indicates that a PMSE Register Request has completed.
        /// </summary>
        PMSERegisterRequest,

        /// <summary>
        /// Indicates that a PMSE Cancel Registration Request has completed.
        /// </summary>
        PMSECancelRegistrationRequest,

        /// <summary>
        /// Indicates that a PMSE Get Registration Request has completed.
        /// </summary>
        PMSEGetRegistrationRequest,

        /// <summary>
        /// Indicates that a region sync operation has completed.  The UserId field 
        /// will be populated with the name of the specific DB that has just been
        /// synchronized (for instance, FCC CDBS, FCC ULS, or FCC EAS).
        /// </summary>
        RegionSync,

        /// <summary>
        /// Indicates that our whitespace DB has just completed a PollRequest from an external 
        /// whitespace DB Admin (the web service call has completed and the DB has been 
        /// synchronized).  The UserId field will be populated with the name of the 3rd party
        /// DB Admin (such as SPBR, TELC, MSFT, ...).
        /// </summary>
        DBSyncPollRequest,

        /// <summary>
        /// Indicates that a Poll Request has just been processed from another whitespace
        /// DB Admin.  The UserId field will be populated with the name of the 3rd party
        /// DB Admin (such as SPBR, TELC, MSFT, ...).
        /// </summary>
        DBSyncPollResponse,

        /// <summary>
        /// Indicates that a new DB Sync file has been generated and placed on the server.  The
        /// UserId field will indicate the type of file generated ("All" or "Incremental") and the message
        /// field will contain the generated Transaction Id stored within the file.
        /// </summary>
        DBSyncFileGenerated,

        /// <summary>
        /// Indicates that a list of Ids were added to the exclusion list.  The UserId will contain
        /// the Id of the caller and the message will contain the list of Ids added to the exclusion list.
        /// </summary>
        ManagementExcludeIds,

        /// <summary>
        /// Indicates that a list of serial numbers were added to the exclusion list.  The UserId will
        /// contain the Id of the caller and the message will contain the list of all serial numbers 
        /// added to the exclusion list.
        /// </summary>
        ManagementExcludeDevice,

        /// <summary>
        /// Indicates that a region has been added to the reservation exclusion list.  The UserId will
        /// contain the Id of the caller and the message will contain all of the points identifying the
        /// excluded region.
        /// </summary>
        ManagementExcludeChannel,

        /// <summary>
        /// Indicates that a new user has registered with the DB.  The UserId will contain the Id of the caller
        /// and the message will all the parameters passed into the web method.
        /// </summary>
        ManagementRegisterUser,

        /// <summary>
        /// Indicates that a user request has just been completed.  The UserId will contain the Id of the caller
        /// and the message will all the parameters passed into the web method.
        /// </summary>
        ManagementGetUserInformation,

        /// <summary>
        /// Indicates that a delete user request has just been completed.  The UserId will contain the Id of the caller
        /// and the message will all the parameters passed into the web method.
        /// </summary>
        ManagementDeleteUser,

        /// <summary>
        /// Indicates that a update user request has just been completed.  The UserId will contain the Id of the caller
        /// and the message will all the parameters passed into the web method.
        /// </summary>
        ManagementUpdateUser,

        /// <summary>
        /// Indicates that a user has requested elevated access to the DB.  The UserId will contain the Id of the caller
        /// and the message will all the parameters passed into the web method.
        /// </summary>
        ManagementRequestElevatedAccess,

        /// <summary>
        /// Indicates that a Add Incumbent request has just been completed.  The UserId will contain the Id of the caller
        /// and the message will all the parameters passed into the web method.
        /// </summary>
        ManagementRegisterDevice,

        /// <summary>
        /// Indicates that a Get Incumbent request has just been completed.  The UserId will contain the Id of the caller
        /// and the message will all the parameters passed into the web method.
        /// </summary>
        ManagementGetIncumbentData,

        /// <summary>
        /// Indicates that a Delete Incumbent request has just been completed.  The UserId will contain the Id of the caller
        /// and the message will all the parameters passed into the web method.
        /// </summary>
        ManagementDeleteIncumbentData,

        /// <summary>
        /// Indicates that a Get Channel List request has just been completed.  The UserId will contain the Id of the caller
        /// and the message will all the parameters passed into the web method.
        /// </summary>
        ManagementGetChannelList,

        /// <summary>
        /// Indicates that a Get Device List request has just been completed.  The UserId will contain the Id of the caller
        /// and the message will all the parameters passed into the web method.
        /// </summary>
        ManagementGetDeviceList,

        /// <summary>
        /// Indicates that a Generic AuditID for DTTSync.  The UserId will contain the Id of the caller
        /// and the message will all the parameters passed into the web method.
        /// </summary>
        DttSync,

        /// <summary>
        /// Indicates that a Generic AuditID for PMSESync.  The UserId will contain the Id of the caller
        /// and the message will all the parameters passed into the web method.
        /// </summary>
        PmseSync,

        /// <summary>
        /// Indicates that a Generic AuditID for PublicData.  The UserId will contain the Id of the caller
        /// and the message will all the parameters passed into the web method.
        /// </summary>
        PublicData,

        /// <summary>
        /// Indicates that a Generic AuditID for GetPublicDataWithEvents.  The UserId will contain the Id of the caller
        /// and the message will all the parameters passed into the web method.
        /// </summary>
        RegionManagementGetPublicDataWithEvents,

        /// <summary>The get licensed LPAUX information</summary>
        RegionManagementGetULSCallSigns,

        /// <summary>The get unlicensed LPAUX information</summary>
        RegionManagementGetULSFileNumbers,

        /// <summary>The get authorized devices information</summary>
        RegionManagementGetAuthorizedDevicesInfo,

        /// <summary>The region management search TV stations</summary>
        RegionManagementSearchMVPDCallSigns,

        /// <summary>The region management Get Callsign Info</summary>
        RegionMangamentGetCallsignInfo,

        /// <summary>The region management get contour data</summary>
        RegionManagementGetContourData,

        /// <summary>register user</summary>
        RegisterUser,

        /// <summary>update user profile</summary>
        UpdateUserProfile,

        /// <summary>request access elevation</summary>        
        RequestAccessElevation,

        /// <summary>update access level</summary>
        UpdateAccessLevel,

        /// <summary>block channel</summary>
        BlockChannel,

        /// <summary>block device</summary>
        BlockDevice,

        /// <summary>device registration</summary>
        DeviceRegistration,

        /// <summary>call sign information</summary>
        CallSignInfo,

        /// <summary>Universal Licensing System file information</summary>
        ULSFileInfo,

        /// <summary>channel list</summary>
        ChannelList,

        /// <summary>public data download</summary>
        PublicDataDownload,

        /// <summary>Connection Error between portal and backend services</summary>
        ConnectionError,

        /// <summary>Exception Occurrence</summary>
        Exception,

        /// <summary>Delete Registrations</summary>
        DeleteRegistration,

        /// <summary>Whitespace finder </summary>
        WhitespaceFinder,

        /// <summary>feedback event</summary>
        Feedback,

        /// <summary>Blocked Id</summary>
        BlockedId,

        /// <summary>Blocked Channel</summary>
        BlockedChannel,

        /// <summary>Monitor Get Channel List</summary>
        MonitorGetChannelList,

        /// <summary>Ofcom evaluation stage testing</summary>
        OfcomEvaluationStageTesting,

        /// <summary>Ofcom evaluation custom file testing</summary>
        OfcomEvaluationCustomFileTesting,

        /// <summary>Ofcom evaluation Operational parameters testing</summary>
        OfcomEvaluationOperationalParameters,

        /// <summary>WSD Spectrum usage Information</summary>
        WSDInfoSystem
    }

    /// <summary>
    /// Identifies all of the audit operations.
    /// </summary>
    public interface IAuditor
    {
        /// <summary>
        /// Gets or sets the region identifier code.
        /// </summary>
        int RegionCode { get; set; }

        /// <summary>
        /// Gets or sets the current user Id associated with the audit (if no user 
        /// associated with audit, then the value can be null).
        /// </summary>
        string UserId { get; set; }

        /// <summary>
        /// Gets or sets the transaction id associated with the audit.
        /// </summary>
        Guid TransactionId { get; set; }

        /// <summary>
        /// Saves the specified audit message.
        /// </summary>
        /// <param name="id">Type of audit being logged.</param>
        /// <param name="status">Indicates if the audit was a success or failure.</param>
        /// <param name="elapsedTime">Amount of time in milliseconds to complete the operation.</param>
        /// <param name="message">Message associated with the audit.</param>
        void Audit(AuditId id, AuditStatus status, long elapsedTime, string message);
    }
}
