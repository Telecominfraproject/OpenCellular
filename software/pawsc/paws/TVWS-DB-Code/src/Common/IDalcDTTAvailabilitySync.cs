// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System.Collections.Generic;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.Storage.Blob;
    using WindowsAzure.Storage.Table;

    /// <summary>Interface IDALCDTTAvailabilitySync</summary>
    public interface IDalcDTTAvailabilitySync
    {
        /// <summary>Gets the DTT synchronize file list from BLOB.</summary>
        /// <returns>returns IListBlobItem[][].</returns>
        IListBlobItem[] GetDttSyncFileListFromBlob();

        /// <summary>Loads the DTT synchronize file.</summary>
        /// <param name="fileName">Name of the file.</param>
        /// <returns>returns List{DTTDataAvailability}.</returns>
        List<DTTDataAvailability> LoadDttSyncFile(string fileName);

        /// <summary>
        /// Inserts the DTT file data.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="batchOperation">The batch operation.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        bool InsertDTTFileData(string tableName, TableBatchOperation batchOperation);

        /// <summary>
        /// Moves the blobs DTT synchronize.
        /// </summary>
        /// <param name="blobName">Name of the BLOB.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        bool MoveBlobsDttSync(string blobName);
    }
}
