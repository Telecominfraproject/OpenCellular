// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents Poll Status
    /// </summary>
    public enum PollStatus
    {
        /// <summary>
        /// The Success.
        /// </summary>
        Success = 0,

        /// <summary>
        /// The transaction id stale.
        /// </summary>
        TransactionIDStale = 1,

        /// <summary>
        /// The bad request.
        /// </summary>
        BadRequest = 2,

        /// <summary>
        /// The requested version not supported.
        /// </summary>
        RequestedVersionnotsupported = 3,

        /// <summary>
        /// No new records.
        /// </summary>
        NoNewRecords = 4,

        /// <summary>
        /// The server error.
        /// </summary>
        ServerError = 5
    }

    /// <summary>
    /// Represents the Other DBA Poll Request Response
    /// </summary>
    public class ParsePollRequestResult
    {
        /// <summary>
        /// Gets or sets poll response code
        /// </summary>
        public PollStatus RTResponseCode { get; set; }

        /// <summary>
        /// Gets or sets Records Ensemble
        /// </summary>
        public string ResponseData { get; set; }

        /// <summary>
        /// Gets or sets next transaction id
        /// </summary>
        public string NextTransactionId { get; set; }
    }
 }
