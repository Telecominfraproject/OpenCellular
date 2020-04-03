// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.DTTSync
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.IO.Compression;
    using System.Linq;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.Storage.Blob;
    using Practices.ObjectBuilder2;
    using WindowsAzure.Storage.Table;

    /// <summary>
    /// Class DTTDataAvailabilitySync.
    /// </summary>
    public class DttDataAvailabilitySync : IDTTAvailabilitySync
    {
        /// <summary>The batches</summary>
        private static Dictionary<string, TableBatchOperation> batches = new Dictionary<string, TableBatchOperation>();

        /// <summary>Gets or sets IUserManager Interface</summary>
        [Dependency]
        public IDalcDTTAvailabilitySync DalcDttAvailabilitySync { get; set; }

        /// <summary>
        /// Gets or sets IAuditor Interface
        /// </summary>
        [Dependency]
        public IAuditor Auditor { get; set; }

        /// <summary>
        /// Gets or sets ILogger Interface
        /// </summary>
        [Dependency]
        public ILogger Logger { get; set; }

        /// <summary>
        /// Gets or sets the DALC common.
        /// </summary>
        /// <value>The DALC common.</value>
        [Dependency]
        public IDalcCommon DalcCommon { get; set; }

        /// <summary>
        /// Synchronizes the database.
        /// </summary>
        public void SyncDB()
        {
            const string LogMethodName = "DttSyncDB";
            Stopwatch parentProcessWatch = new Stopwatch();
            parentProcessWatch.Start();

            var processedFiles = this.DalcCommon.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.ConfigSettingDTTSyncStatusTable), null);

            Action<IListBlobItem> blobAction = (blobItem) =>
            {
                // Gets new transaction id for each file
                string fileName = Path.GetFileName(blobItem.Uri.ToString());

                //// check for already processed file
                if (processedFiles.FirstOrDefault(obj => obj.RowKey.ToLower() == fileName.ToLower()) != null)
                {
                    return;
                }

                try
                {
                    // Begin elapsed time calculation
                    Stopwatch stopWatch = new Stopwatch();
                    stopWatch.Start();

                    ////this.DalcDttAvailabilitySync.LoadDttSyncFile(fileName);
                    string[] fileNameSegments = fileName.Split('_');

                    var dttTablenameSuffix = Utils.Configuration[Constants.ConfigSettingDTTSyncTableSuffix];

                    if (string.IsNullOrEmpty(dttTablenameSuffix))
                    {
                        dttTablenameSuffix = Constants.ConfigSettingDefaultDTTSyncTableSuffix;
                    }

                    string tableName = Utils.CurrentRegionName + Utils.Configuration.CurrentRegionId + "p" + fileNameSegments[2] + "h" + fileNameSegments[3] + dttTablenameSuffix;

                    Stream dataStream = this.DalcCommon.StreamBlobFile(fileName, Utils.Configuration[Constants.ConfigSettingDttSyncSourceContainer], TimeSpan.FromDays(1));
                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DttSyncGenericMessage, string.Format("Start Processing File name : {0}", fileName));

                    this.ProcessDttSyncFile(dataStream, fileName, tableName);

                    stopWatch.Stop();

                    DynamicTableEntity tableEntity = new DynamicTableEntity();
                    tableEntity.PartitionKey = "DttSync";
                    tableEntity.RowKey = fileName;

                    this.DalcCommon.InsertEntity(Utils.GetRegionalTableName("DttSyncStatus"), tableEntity);

                    this.Logger.Log(TraceEventType.Information, LoggingMessageId.DttSyncGenericMessage, string.Format("Completed File:{0}, TimeInSec:{1}", fileName, stopWatch.Elapsed.TotalSeconds));
                }
                catch (Exception e)
                {
                    this.Logger.Log(TraceEventType.Error, LoggingMessageId.DttSyncGenericMessage, "Exception Occured while processing file :- " + fileName + " Exception " + e.ToString());
                    this.Auditor.Audit(AuditId.DttSync, AuditStatus.Failure, 0, string.Format("Exception Occured in Method: {0} while processing file :- {1}, Exceptiion :{2}", LogMethodName, fileName, e.ToString()));
                }
            };

            this.DalcDttAvailabilitySync.GetDttSyncFileListFromBlob().AsParallel().WithDegreeOfParallelism(1).ForAll(blobAction);
            
            this.Auditor.Audit(AuditId.DttSync, AuditStatus.Success, parentProcessWatch.ElapsedMilliseconds, string.Format("Completed DTTSyncProcess, TimeInMin:{0}", parentProcessWatch.Elapsed.TotalMinutes));
        }

        /// <summary>
        /// Loads the DTT synchronize file.
        /// </summary>
        /// <param name="dataStream">The data stream.</param>
        /// <param name="fileName">Name of the file.</param>
        /// <param name="tableName">Name of the table.</param>
        public void ProcessDttSyncFile(Stream dataStream, string fileName, string tableName)
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DttSyncGenericMessage, string.Format("Enter Method Name: {0}", "ProcessDttSyncFile"));

            try
            {
                int count = 0;

                Dictionary<string, DTTSquare> squares = new Dictionary<string, DTTSquare>();
                using (StreamReader sr = new StreamReader(dataStream))
                {
                    // First line should be header
                    string line = sr.ReadLine();

                    // Make sure that the header has "easting"
                    if (!line.ToLower().Contains("easting"))
                    {
                        throw new Exception("First line does not contain the word 'easting'");
                    }

                    while (sr.Peek() >= 0)
                    {
                        count++;

                        line = sr.ReadLine();

                        ////string[] numbers = line.Split(',');
                        int[] dttValues = line.Split(new[] { ',' }).Select(obj => obj.ToInt32()).ToArray();

                        int squareStartEasting = (dttValues[0] / DTTSquare.NumberOfEastings) * DTTSquare.NumberOfEastings;
                        int squareStartNorthing = (dttValues[1] / DTTSquare.NumberOfNorthings) * DTTSquare.NumberOfNorthings;
                        string key = DTTSquare.FormatEastingNorthingName(squareStartEasting, squareStartNorthing);

                        DTTSquare square;
                        if (!squares.Keys.Contains(key))
                        {
                            square = new DTTSquare(squareStartEasting, squareStartNorthing);
                            squares.Add(key, square);
                        }
                        else
                        {
                            square = squares[key];
                        }

                        square.SetDTT(dttValues[0], dttValues[1], dttValues);

                        if (square.AllDTTFieldsSet())
                        {
                            // Write square out to be completed
                            this.SaveDTTSquareToBatch(tableName, square);
                            squares.Remove(key);
                        }
                    }

                    if (squares.Count() > 0)
                    {
                        throw new Exception("Didn't write out all squares.");
                    }

                    // Flush any remaining batches
                    foreach (string paritionKey in batches.Keys)
                    {
                        TableBatchOperation batchOperation = batches[paritionKey];
                        this.FlushBatch(tableName, batchOperation);
                    }
                }
            }
            catch (Exception e)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DttSyncGenericMessage, string.Format("Exception Occured in file : - {0}, Exception :{1}", fileName, e.ToString()));
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DttSyncGenericMessage, string.Format("Exit Method Name: {0}", "ProcessDttSyncFile"));
        }

        /// <summary>
        /// Save the DTT square to batch.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="square">The square.</param>
        private void SaveDTTSquareToBatch(string tableName, DTTSquare square)
        {
            try
            {
                byte[] block = new byte[square.DttValues.Length * sizeof(int)];
                Buffer.BlockCopy(square.DttValues, 0, block, 0, block.Length);

                var stream = new MemoryStream();
                using (Stream ds = new GZipStream(stream, CompressionMode.Compress))
                {
                    ds.Write(block, 0, square.DttValues.Count() * sizeof(int));
                }

                byte[] compressed = stream.ToArray();

                DynamicTableEntity entity = new DynamicTableEntity();
                entity.Properties.Add("DataRecord", EntityProperty.GeneratePropertyForByteArray(compressed));
                entity.Properties.Add("Easting", EntityProperty.GeneratePropertyForInt(square.Easting));
                entity.Properties.Add("Northing", EntityProperty.GeneratePropertyForInt(square.Northing));
                entity.RowKey = square.Northing.ToString().PadLeft(7, '0');
                entity.PartitionKey = square.Easting.ToString().PadLeft(7, '0');

                if (!batches.ContainsKey(entity.PartitionKey))
                {
                    batches[entity.PartitionKey] = new TableBatchOperation();
                }

                TableBatchOperation batchOperations = batches[entity.PartitionKey];
                batchOperations.Add(TableOperation.Insert(entity));

                if (batchOperations.Count >= 50)
                {
                    this.FlushBatch(tableName, batchOperations);
                    batchOperations.Clear();
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DttSyncGenericMessage, string.Format("Exception Occured in Table: {0}, Exception :{1}", tableName, ex.ToString()));
            }
        }

        /// <summary>
        /// Flushes the batch operations to the specified table.
        /// </summary>
        /// <param name="tableName">Name of table to perform batch inserts.</param>
        /// <param name="batchOperations">All insert commands.</param>
        private void FlushBatch(string tableName, TableBatchOperation batchOperations)
        {
            if (batchOperations == null || batchOperations.Count == 0)
            {
                // Nothing in batch to flush.
                return;
            }

            var success = this.DalcDttAvailabilitySync.InsertDTTFileData(tableName, batchOperations);
            if (!success)
            {
                throw new Exception("Error in Processing Batch for table");
            }
        }
    }
}
