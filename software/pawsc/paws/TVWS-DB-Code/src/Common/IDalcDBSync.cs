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
    /// Represents the database sync interface into the data access layer component.
    /// </summary>
    public interface IDalcDBSync
    {
        /// <summary>
        /// Returns all of the fixed TV DB Registrations.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="transactionId">Specifies the TransactionId that is being used for 
        /// filtering the query.</param>
        /// <returns>Returns an array of Fixed TVBD Registrations.</returns>
        FixedTVBDRegistration[] GetFixedTVBDRegistration(string adminName = null, string transactionId = null);

        /// <summary>
        /// Returns all of the fixed TV DB Registrations for the given period.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="fromDate"> from date</param>
        /// <param name="toDate"> To Date</param>
        /// <returns>Returns an array of Fixed TVBD Registrations.</returns>
        FixedTVBDRegistration[] GetFixedTVBDRegistration(string adminName, DateTime fromDate, DateTime toDate);

        /// <summary>
        /// Returns all of the LP Aux Registrations for given time
        /// </summary>
        /// <param name="adminName">the DB admin name of the requestor.  If the
        /// value is null, then there is no specific requestor.</param>
        /// <param name="fromDate">from date</param>
        /// <param name="toDate">to date</param>
        /// <returns>LP Aux registrations Array</returns>
        LPAuxRegistration[] GetLPAuxRegistration(string adminName, DateTime fromDate, DateTime toDate);
        
        /// <summary>
        /// Returns all of the LP Aux Registrations.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="transactionId">Specifies the TransactionId that is being used for 
        /// filtering the query.</param>
        /// <returns>Returns an array of LP Aux Registrations.</returns>
        LPAuxRegistration[] GetLPAuxRegistration(string adminName = null, string transactionId = null);

        /// <summary>
        /// Returns all of the fixed MVPD Registrations.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="transactionId">Specifies the TransactionId that is being used for 
        /// filtering the query.</param>
        /// <returns>Returns an array of MVPD Registrations.</returns>
        MVPDRegistration[] GetMVPDRegistration(string adminName = null, string transactionId = null);

        /// <summary>
        /// Returns the fixed MVPD Registrations for given time
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="fromDate">from date</param>
        /// <param name="toDate">to date</param>
        /// <returns>MVPD Registrations Array</returns>
        MVPDRegistration[] GetMVPDRegistration(string adminName, DateTime fromDate, DateTime toDate);
        
        /// <summary>
        /// Returns all of the TV Receive Site Registrations.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="transactionId">Specifies the TransactionId that is being used for 
        /// filtering the query.</param>
        /// <returns>Returns an array of TV Receive Site Registrations.</returns>
        TVReceiveSiteRegistration[] GetTVReceiveSiteRegistration(string adminName = null, string transactionId = null);

        /// <summary>
        /// Returns TV Receive Site Registrations for the given time.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="fromDate">from date</param>
        /// <param name="toDate">to date</param>
        /// <returns>TV Receive Site Registrations Array</returns>
        TVReceiveSiteRegistration[] GetTVReceiveSiteRegistration(string adminName, DateTime fromDate, DateTime toDate);

        /// <summary>
        /// Returns Temp BAS Registrations for the given time
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="transactionId">Specifies the TransactionId that is being used for 
        /// filtering the query.</param>
        /// <returns>Returns an array of Temp BAS Registrations.</returns>
        TempBASRegistration[] GetTempBASRegistration(string adminName = null, string transactionId = null);

        /// <summary>
        /// Returns all of the Temp BAS Registrations.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.  If the 
        /// value is null, then there is no specific requestor.</param>
        /// <param name="fromDate">from date</param>
        /// <param name="toDate">to date</param>
        /// <returns>Temp BAS Registrations Array</returns>
        TempBASRegistration[] GetTempBASRegistration(string adminName, DateTime fromDate, DateTime toDate);

        /// <summary>
        /// Updates all of the registrations in the DB.
        /// </summary>
        /// <param name="adminName">Specifies the DB admin name of the requestor.</param>
        /// <param name="fixedRegistrations">Updated Fixed TVBD Registrations.</param>
        /// <param name="licensedPointAuxRegistrations">Updated LP Aux Registrations.</param>
        /// <param name="mvpdRegistrations">Updated MVPD Registrations.</param>
        /// <param name="televisionReceiveSiteRegistrations">Updated Receive Site Registrations.</param>
        /// <param name="tempBASregistrations">Updated Temp BAS Registrations.</param>
        /// <param name="transactionId">The transaction Id of this update.</param>
        void Update(
            string adminName,
            FixedTVBDRegistration[] fixedRegistrations,
            LPAuxRegistration[] licensedPointAuxRegistrations, 
            MVPDRegistration[] mvpdRegistrations,
            TVReceiveSiteRegistration[] televisionReceiveSiteRegistrations,
            TempBASRegistration[] tempBASregistrations,
            string transactionId = null);

        /// <summary>
        /// Returns the last transaction Id used for the specified Database Admin.
        /// </summary>
        /// <param name="adminName">Database Admin name (SPBR, TELC, MSFT, ...)</param>
        /// <param name="scope"> Scope of the Update Database Sync Scope (ALL/INCR)</param>
        /// <returns>Returns the last transaction Id.</returns>
        NextTransactionID GetRequestorTransactionId(string adminName, DbSyncScope scope);

        /// <summary>
        /// Returns the last transaction Id used for the specified DB Admin.
        /// </summary>
        /// <param name="requestedTransactionId">Requested Transaction ID</param>
        /// <returns>Returns the last transaction Id.</returns>
        NextTransactionID GetRequestorTransactionId(string requestedTransactionId);

        /// <summary>
        /// Returns transaction info for the Sync Poller worker.
        /// </summary>
        /// <param name="adminName">DB Admin name (SPBR, TELC, MSFT, ...)</param>
        /// <returns>Returns the transaction info of the whitespace DBA.</returns>
        TransactionInfo GetPollerTransactionInfo(string adminName);

        /// <summary>
        /// Returns all of the known DB Admins.
        /// </summary>
        /// <returns>All of the registered DB Admins</returns>
        DBAdminInfo[] GetDBAdmins();

        /// <summary>
        /// Updates DBAdmin Info after poll request.
        /// </summary>
        /// <param name="dbadminInfo">DB Admin info</param>
        void UpdateDBAdminInfo(DBAdminInfo dbadminInfo);

        /// <summary>
        /// Updates next transaction id Info.
        /// </summary>
        /// <param name="nextTransactionId"> next transaction id table entity</param>
        void SaveNextTransactionIdInfo(NextTransactionID nextTransactionId);

        /// <summary>
        /// Uploads the All Registrations file to the cloud storage
        /// </summary>
        /// <param name="filename"> file to be uploaded</param>
        /// <param name="containerName"> container name to which file to be uploaded</param>
        /// <param name="blobName">name of the Blob</param>
        void UploadAllRegistrationsFile(string filename, string containerName, string blobName);

        /// <summary>
        /// Uploads the All Registrations file to the cloud storage
        /// </summary>
        /// <param name="filename"> file to be uploaded</param>
        /// <param name="containerName"> container name to which file to be uploaded</param>
        /// <param name="blobName">name of the blob</param>
        void UploadINCRegistrationsFile(string filename, string containerName, string blobName);
    }
}
