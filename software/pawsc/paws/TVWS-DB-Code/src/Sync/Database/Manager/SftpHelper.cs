// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Sync.Database.Manager
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.IO.Compression;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Common;
    using Renci.SshNet;
    using Renci.SshNet.Common;
    using Renci.SshNet.Sftp;
    
    /// <summary>
    /// SFTP helper class used to connect to SFTP sites.
    /// </summary>
    public class SFTPHelper
    {
        /// <summary>
        /// holds the instance of logger.
        /// </summary>
        private ILogger syncLogger = null;

        /// <summary>
        /// Initializes a new instance of the SFTPHelper class.
        /// </summary>
        /// <param name="logger"> Logger Object</param>
        public SFTPHelper(ILogger logger)
        {
            this.syncLogger = logger;
        }

        /// <summary>
        /// Gets All Registrations file from SFTP
        /// </summary>
        /// <param name="sftpServerUrl">SFTP Server URL</param>
        /// <param name="sftpPath">SFTP's Path</param>
        /// <param name="userID">SFTP's User Id</param>
        /// <param name="password">The Password</param>
        /// <param name="targetPath"> Target path to download the file.</param>
        /// <returns> file path </returns>
        public string GetAllXMLfileFromSFTP(string sftpServerUrl, string sftpPath, string userID, string password, string targetPath)
        {
            string fileName = null;
            SftpClient sftpclient;
            FileStream mem = null;

            this.syncLogger.Log(System.Diagnostics.TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin SftpHelper.GetAllXMLfileFromSFTP");
            using (sftpclient = new SftpClient(sftpServerUrl, userID, password))
            {
                // connecting to sftp
                sftpclient.Connect();
                foreach (SftpFile file in sftpclient.ListDirectory(sftpPath))
                { 
                    fileName = file.Name;
                    if (fileName != null && fileName.IndexOf("ALL") > 0)
                    {
                        break;
                    }
                }

                targetPath = targetPath + "F" + DateTime.Now.ToFileTime();

                if (!Directory.Exists(targetPath))
                {
                    Directory.CreateDirectory(targetPath);
                }
                
                // downloading the file
                using (mem = File.Create(targetPath + "\\" + fileName))
                {
                    var asynch = sftpclient.BeginDownloadFile(sftpPath + fileName, mem, null, null);
                    while (!asynch.IsCompleted)
                    {
                        Thread.Sleep(100);
                    }

                    sftpclient.EndDownloadFile(asynch);
                }
                
                // unzip and save the file
                this.UnzipFile(targetPath + "\\" + fileName, targetPath);
                sftpclient.Disconnect();
            }

            using (ZipArchive zip = ZipFile.Open(targetPath + "\\" + fileName, ZipArchiveMode.Read))
            {
                foreach (ZipArchiveEntry entry in zip.Entries)
                {
                    if (entry.Name != null && entry.Name.IndexOf("ALL") > 0)
                    {
                        fileName = entry.Name;
                    }
                }
            }

            this.syncLogger.Log(System.Diagnostics.TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End SftpHelper.GetAllXMLfileFromSFTP");
            return targetPath + "\\" + fileName.Substring(0, fileName.LastIndexOf(".")) + ".xml";
        }

        /// <summary>
        /// unzip the compressed file
        /// </summary>
        /// <param name="zipFilePath">compressed file path</param>
        /// <param name="targetPath">target path</param>
        private void UnzipFile(string zipFilePath, string targetPath)
        {
            ZipFile.ExtractToDirectory(zipFilePath, targetPath);
        }
    }
}
