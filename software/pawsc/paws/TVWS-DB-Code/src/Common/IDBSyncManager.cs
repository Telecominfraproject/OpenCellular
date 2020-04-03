// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Xml;
    using Microsoft.Whitespace.Entities;

    /// <summary>
    /// Represents the DB Sync manager interface used by DB Sync Poller worker, DB Sync Filer Worker, and DB Admin web service.
    /// </summary>
    public interface IDBSyncManager
    {
        // TransactionId is valid
        // Validate command is wsdPoll
        // Embeds next Transaction Id
        // Makes sure less than 72 hours for TransactionId

        /// <summary>
        /// Parses all poll request from external DB Admins. 
        /// </summary>
        /// <param name="pollRequest">The poll request data.</param>
        /// <returns>Returns a response stream that is to be sent back to the DB Admins.</returns>
        ParsePollRequestResult ParsePollRequest(string pollRequest);

        /// <summary>
        /// Generates RealTimePollRequest for the specified DB Administrator.
        /// </summary>
        /// <param name="stream">A DB Admin name such as SPBR, TELC, MSFT, KEYB, ...</param>
        /// <param name="nextTransactionId"> next transaction Id</param>
        /// <returns>Returns an XML stream of the RealTimePollRequest.</returns>
        Stream GeneratePollRequest(Stream stream,  string nextTransactionId);

        /// <summary>
        /// Parses the response XML from the fast poll request and updates the local DB.
        /// </summary>
        /// <param name="adminName">The whitespace DB who generated the poll response.</param>
        /// <param name="pollResponseXml">The response from the remote DB Admin that is to be parsed.</param>
        /// <param name="publicKey">Public key of Administrator</param>
        void ParsePollResponse(string adminName, string pollResponseXml, string publicKey);

        /// <summary>
        /// Generates an XML file (either generates the full XML or an incremental file).
        /// </summary>
        /// <param name="nextTransactionId">Next transaction ID to generate RecordEnsemble.</param>
        /// <returns>Returns an XML stream of the file content.</returns>
        ParsePollRequestResult GenerateRecordEnsemble(string nextTransactionId);

        /// <summary>
        /// Generates an XML file (either generates the full XML file).
        /// </summary>
        /// <param name="scope">DB Sync Scope</param>
        /// <returns>Returns an XML string of the file content.</returns>
        string GenerateRecordEnsemble(DbSyncScope scope);

        /// <summary>
        /// Gets DB Admin info object array
        /// </summary>
        /// <returns>DBAdminInfo Array.</returns>
        DBAdminInfo[] GetDBAdmininfo();

        /// <summary>
        /// Update DB Admin Info
        /// </summary>
        /// <param name="admin">admin info</param>
        void UpdateDBAdminInfo(DBAdminInfo admin);
    }
}
