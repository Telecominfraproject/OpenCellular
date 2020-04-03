// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents the data of a whitespace database administrator (WSDBA).
    /// </summary>
    public class DBAdminInfo : TableEntity
    {
        /// <summary>
        /// Gets or sets the whitespace DB admin name (i.e. SPBR, TELC, MSFT, ...).
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the "fast" polling web service URL.
        /// </summary>
        public string WebServiceUrl { get; set; }

        /// <summary>
        /// Gets or sets the ftp file server file downloading daily files.
        /// </summary>
        public string FtpServerUrl { get; set; }

        /// <summary>
        /// Gets or sets the ftp server user id.
        /// </summary>
        public string FtpServerUserId { get; set; }

        /// <summary>
        /// Gets or sets the ftp server user id.
        /// </summary>
        public string FtpServerPwd { get; set; }

        /// <summary>
        /// Gets or sets the DB Admin's public key used for validating the 
        /// authenticity of the XML files retrieved from either the "fast" 
        /// polling web service or the ftp server.
        /// </summary>
        public string PublicKey { get; set; }

        /// <summary>
        /// Gets or sets the DB Admin's private key used for validating the 
        /// authenticity of the XML files retrieved from either the "fast" 
        /// polling web service or the ftp server.
        /// </summary>
        public string PrivateKey { get; set; }
        
        /// <summary>
        /// Gets or sets NextTransactionID for polling
        /// </summary>
        public string NextTransactionID { get; set; }

        /// <summary>
        /// Gets or sets SFTP Path
        /// </summary>
        public string SFTPPath { get; set; }
    }
}
