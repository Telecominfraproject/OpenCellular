// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.DataAccessManager
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.AzureTableAccess;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Enums;
    using Microsoft.WindowsAzure.Storage.Table;

    public class RegionDataAccess : IRegionDataAccess
    {
        private IAzureTableOperation azureTableOperations;

        [Microsoft.Practices.Unity.InjectionConstructor]
        public RegionDataAccess()
            : this(new AzureTableOperation())
        { 
        }

        public RegionDataAccess(IAzureTableOperation azureTableOperations)
        {
            this.azureTableOperations = azureTableOperations;
        }

        public IEnumerable<MVPDRegistration> GetMvpdRegistrations(string userId)
        {
            List<MVPDRegistration> registrations = new List<MVPDRegistration>();
            object lockobject = new object();

            var query = new TableQuery<DynamicTableEntity>();
            List<string> columns = new List<string>();
            columns.Add("MVPDRegDisposition");
            columns.Add("MVPDRegistrant");
            columns.Add("MVPDContact");
            columns.Add("MVPDLocation");
            columns.Add("MVPDChannel");
            columns.Add("MVPDTransmitLocation");
            columns.Add("UserId");
            query.Select(columns);

            string registrationTableName = Microsoft.Whitespace.Common.Utils.GetRegionalTableName(Microsoft.Whitespace.Entities.Constants.MVPDRegistrationTableName);

            var results = this.azureTableOperations.GetTableEntityProjection(registrationTableName, query);
            if (results != null && results.Count() > 0)
            {
                if (!string.IsNullOrEmpty(userId))
                {
                    results = results.Where(x => x.Properties["UserId"].StringValue == userId);
                }

                Parallel.ForEach(
                    results, 
                    result =>
                    {
                        var mvpdRegistration = new MVPDRegistration
                        {
                            MVPDRegDisposition = result.Properties["MVPDRegDisposition"].StringValue,
                            MVPDRegistrant = result.Properties["MVPDRegistrant"].StringValue,
                            MVPDContact = result.Properties["MVPDContact"].StringValue,
                            MVPDLocation = result.Properties["MVPDLocation"].StringValue,
                            MVPDChannel = result.Properties["MVPDChannel"].StringValue,
                            MVPDTransmitLocation = result.Properties["MVPDTransmitLocation"].StringValue,
                            UserId = result.Properties["UserId"].StringValue,
                            PartitionKey = result.PartitionKey,
                            RowKey = result.RowKey,
                            ETag = result.ETag
                        };

                        lock (lockobject)
                        {
                            registrations.Add(mvpdRegistration);
                        }
                    });
            }

            return registrations;
        }

        public IEnumerable<LPAuxRegistration> GetLpAuxRegistrations(string userId)
        {
            List<LPAuxRegistration> registrations = new List<LPAuxRegistration>();
            object lockobject = new object();

            var query = new TableQuery<DynamicTableEntity>();
            List<string> columns = new List<string>();
            columns.Add("AuxRegDisposition");
            columns.Add("AuxRegistrant");
            columns.Add("AuxTvSpectrum");
            columns.Add("ULSFileNumber");
            columns.Add("RegId");
            columns.Add("VenueName");
            columns.Add("Licensed");
            columns.Add("UserId");
            columns.Add("Latitude");
            columns.Add("Longitude");
            columns.Add("AuxEvent");
            columns.Add("AuxContact");
            columns.Add("AuxPointsArea");

            query.Select(columns);

            string registrationTableName = Microsoft.Whitespace.Common.Utils.GetRegionalTableName(Microsoft.Whitespace.Entities.Constants.LPAuxRegistrationTable);
            var results = this.azureTableOperations.GetTableEntityProjection(registrationTableName, query);

            if (results != null && results.Count() > 0)
            {
                if (!string.IsNullOrEmpty(userId))
                {
                    results = results.Where(x => x.Properties["UserId"].StringValue == userId);
                }

                Parallel.ForEach(
                    results, 
                    result =>
                    {
                        var lpAuxRegistration = new LPAuxRegistration
                        {
                            AuxRegDisposition = result.Properties["AuxRegDisposition"].StringValue,
                            AuxRegistrant = result.Properties["AuxRegistrant"].StringValue,
                            AuxContact = result.Properties["AuxContact"].StringValue,
                            AuxTvSpectrum = result.Properties["AuxTvSpectrum"].StringValue,
                            AuxPointsArea = result.Properties["AuxPointsArea"].StringValue,
                            AuxEvent = result.Properties["AuxEvent"].StringValue,
                            ULSFileNumber = result.Properties["ULSFileNumber"].StringValue,
                            RegId = result.Properties["RegId"].StringValue,
                            VenueName = result.Properties["VenueName"].StringValue,
                            UserId = result.Properties["UserId"].StringValue,
                            Licensed = Convert.ToBoolean(result.Properties["Licensed"].BooleanValue),
                            Latitude = Convert.ToDouble(result.Properties["Latitude"].DoubleValue),
                            Longitude = Convert.ToDouble(result.Properties["Longitude"].DoubleValue),
                            PartitionKey = result.PartitionKey,
                            RowKey = result.RowKey,
                            ETag = result.ETag
                        };

                        lock (lockobject)
                        {
                            registrations.Add(lpAuxRegistration);
                        }
                    });
            }

            return registrations;
        }

        public IEnumerable<TempBASRegistration> GetTempBasRegistrations(string userId)
        {
            List<TempBASRegistration> registrations = new List<TempBASRegistration>();
            object lockobject = new object();

            var query = new TableQuery<DynamicTableEntity>();
            List<string> columns = new List<string>();
            columns.Add("TempBasRegDisposition");
            columns.Add("TempBasRegistrant");
            columns.Add("TempBasContact");
            columns.Add("TempBasLocation");
            columns.Add("TempBasChannel");
            columns.Add("TempBasTransmitLocation");
            columns.Add("TempBasEvent");
            columns.Add("UserId");
            query.Select(columns);

            string registrationTableName = Microsoft.Whitespace.Common.Utils.GetRegionalTableName(Microsoft.Whitespace.Entities.Constants.TempBasRegistrationTableName);

            var results = this.azureTableOperations.GetTableEntityProjection(registrationTableName, query);
            if (results != null && results.Count() > 0)
            {
                if (!string.IsNullOrEmpty(userId))
                {
                    results = results.Where(x => x.Properties["UserId"].StringValue == userId);
                }

                Parallel.ForEach(
                    results, 
                    result =>
                    {
                        var tempBasRegistration = new TempBASRegistration
                        {
                            TempBasRegDisposition = result.Properties["TempBasRegDisposition"].StringValue,
                            TempBasRegistrant = result.Properties["TempBasRegistrant"].StringValue,
                            TempBasContact = result.Properties["TempBasContact"].StringValue,
                            TempBasLocation = result.Properties["TempBasLocation"].StringValue,
                            TempBasChannel = result.Properties["TempBasChannel"].StringValue,
                            TempBasTransmitLocation = result.Properties["TempBasTransmitLocation"].StringValue,
                            TempBasEvent = result.Properties["TempBasEvent"].StringValue,
                            UserId = result.Properties["UserId"].StringValue,
                            PartitionKey = result.PartitionKey,
                            RowKey = result.RowKey,
                            ETag = result.ETag
                        };

                        lock (lockobject)
                        {
                            registrations.Add(tempBasRegistration);
                        }
                    });
            }

            return registrations;
        }

        public void DeleteRegistration(string partitionKey, string rowKey, string etag, RegistrationType registrationType)
        {
            string tableName = string.Empty;
            TableEntity entity = null;

            switch (registrationType.ToString())
            {
                case "LpAux": tableName = Utils.GetRegionalTableName(Microsoft.Whitespace.Entities.Constants.LPAuxRegistrationTable);
                    entity = this.azureTableOperations.FetchEntity<LPAuxRegistration>(partitionKey, rowKey, tableName);
                    break;

                case "TBas": tableName = Utils.GetRegionalTableName(Microsoft.Whitespace.Entities.Constants.TempBasRegistrationTableName);
                    entity = this.azureTableOperations.FetchEntity<TempBASRegistration>(partitionKey, rowKey, tableName);
                    break;

                case "Mvpd": tableName = Utils.GetRegionalTableName(Microsoft.Whitespace.Entities.Constants.MVPDRegistrationTableName);
                    entity = this.azureTableOperations.FetchEntity<MVPDRegistration>(partitionKey, rowKey, tableName);
                    break;
            }

            if (entity != null)
            {
                this.azureTableOperations.DeleteEntity(tableName, entity);
            }
        }

        public bool SaveFeedback(FeedbackInfo info)
        {
            Feedbacks feedback = new Feedbacks
            {
                RowKey = info.FeedbackTime.ToString(),
                FirstName = info.FirstName,
                LastName = info.LastName,
                Email = info.Email,
                Phone = info.Phone,
                Subject = info.Subject,
                Message = info.Message
            };

            this.azureTableOperations.InsertEntity(feedback);

            return true;
        }

        public IEnumerable<ExcludedDevice> GetExcludedIdsByRegionCode(string regionCode)
        {
            List<ExcludedDevice> excludedIds = new List<ExcludedDevice>();
            object lockobject = new object();

            var query = new TableQuery<DynamicTableEntity>();
            List<string> columns = new List<string>();
            columns.Add("DeviceId");
            columns.Add("SerialNumber");
            query.Select(columns);

            string tableName = regionCode + Microsoft.WhiteSpaces.Common.Constants.ExcludedIdsTableName;

            var results = this.azureTableOperations.GetTableEntityProjection(tableName, query);

            if (results.Count() > 0)
            {
                Parallel.ForEach(
                    results, 
                    result =>
                    {
                        ExcludedDevice excludeId = new ExcludedDevice
                        {
                            DeviceId = result.Properties["DeviceId"].StringValue,
                            SerialNumber = result.Properties["SerialNumber"].StringValue,
                            RecordPartitionKey = result.PartitionKey,
                            RecordRowKey = result.RowKey
                        };

                        lock (lockobject)
                        {
                            excludedIds.Add(excludeId);
                        }
                    });
            }

            return excludedIds;
        }

        public void DeleteExcludedId(string regionCode, string partionKey, string rowKey)
        {
            string tableName = regionCode + Microsoft.WhiteSpaces.Common.Constants.ExcludedIdsTableName;

            var entity = this.azureTableOperations.GetTableEntityProjection(tableName, new TableQuery<DynamicTableEntity>()).FirstOrDefault();
            if (entity != null)
            {
                TableEntity tableEntity = new TableEntity
                {
                    PartitionKey = entity.PartitionKey,
                    RowKey = entity.RowKey,
                    ETag = entity.ETag
                };

                this.azureTableOperations.DeleteEntity(tableName, tableEntity);
            }
        }

        public IEnumerable<ExcludedChannels> GetExcludedChannelsByRegionCode(string regionCode)
        {           
            string tableName = regionCode + Microsoft.WhiteSpaces.Common.Constants.ExcludedChannelsTableName;

            return this.azureTableOperations.GetAllEntities<ExcludedChannels>(tableName);
        }

        public void DeleteExcludedChannels(string regionCode, string partitionKey, string rowKey)
        {
            string tableName = regionCode + Microsoft.WhiteSpaces.Common.Constants.ExcludedChannelsTableName;

            var entity = this.azureTableOperations.FetchEntity<ExcludedChannels>(partitionKey, rowKey, tableName);

            if (entity != null)
            {
                this.azureTableOperations.DeleteEntity(tableName, entity);
            }
        }

        public string[] UploadTestResults(Dictionary<string, System.IO.Stream> blobStreams)
        {            
            BlobOperations blobOperations = new BlobOperations();
            return blobOperations.UploadFilesandGetUrls(blobStreams);
        }

        public IEnumerable<PmseAssignment> GetAllPmseAssignementEntities(string tableName)
        {
            return this.azureTableOperations.GetAllEntities<PmseAssignment>(tableName);
        }

        public IEnumerable<DTTDataAvailability> GetDTTDataAvailability(string tableName)
        {
            return this.azureTableOperations.GetAllEntities<DTTDataAvailability>(tableName);
        }

        public void DeletePmseEntities(IEnumerable<PmseAssignment> entities, string tableName)
        {
            this.azureTableOperations.DeleteBatchEntities<PmseAssignment>(tableName, entities);
        }

        public void DeleteDTTDataAvailability(IEnumerable<DTTDataAvailability> entities, string tableName)
        {
            this.azureTableOperations.DeleteBatchEntities<DTTDataAvailability>(tableName, entities);
        }

        public void InsertPmseEntities(IEnumerable<PmseAssignment> entities, string tableName)
        {            
            this.azureTableOperations.InsertBatchEntities<PmseAssignment>(tableName, entities);
        }

        public void InsertDTTDataAvailability(IEnumerable<DTTDataAvailability> entities, string tableName)
        {
            this.azureTableOperations.InsertBatchEntities<DTTDataAvailability>(tableName, entities);
        }
    }
}
