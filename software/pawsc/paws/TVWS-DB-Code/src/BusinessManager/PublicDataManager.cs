// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.BusinessManager
{
    using System;
    using System.Data;
    using System.Diagnostics;
    using System.Text;
    using System.Xml;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.DataAccessManager;
    using Newtonsoft.Json;

    public class PublicDataManager : IPublicDataManager
    {
        private readonly IPublicDataAccess dataAccess;

        private readonly IWhitespacesDataClient whitespaceDataClient;

        private readonly string defaultResultString;

        public PublicDataManager(IPublicDataAccess dataAccess, IWhitespacesDataClient whitespaceDataClient)
        {
            this.dataAccess = dataAccess;

            this.whitespaceDataClient = whitespaceDataClient;

            this.defaultResultString = Constants.NoDataAvailable;
        }

        [Dependency]
        public IAuditor DataDownloadAuditor { get; set; }

        [Dependency]
        public ILogger DataDownloadLogger { get; set; }

        public string GetPublicDataWithEvents(string entityType, string regionName = "United States")
        {
            Check.IsNotEmptyOrWhiteSpace(entityType, "entityType");

            var jsonString = this.dataAccess.GetPublicDataWithEvents(entityType, regionName);

            if (!string.IsNullOrEmpty(jsonString) && !string.Equals(jsonString, Microsoft.Whitespace.Entities.Constants.ErrorMessageNoData, StringComparison.OrdinalIgnoreCase))
            {
                this.DataDownloadLogger.Log(TraceEventType.Information, LoggingMessageId.PortalDataDownload, "Data of entity type " + entityType + " has been downloaded for region " + regionName);

                return this.GetCSVOutofJson(jsonString);
            }

            var region = CommonUtility.GetRegionByName(regionName);
            this.DataDownloadAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
            this.DataDownloadAuditor.TransactionId = this.DataDownloadLogger.TransactionId;
            this.DataDownloadAuditor.Audit(AuditId.PublicDataDownload, AuditStatus.Failure, default(int), "No data found for entity type" + entityType + " for region " + regionName);
            this.DataDownloadLogger.Log(TraceEventType.Error, LoggingMessageId.PortalDataDownload, "No data found for entity type" + entityType + " for region " + regionName);

            return this.defaultResultString;
        }

        public string GetPublicData(string entityType, string regionName = "United States")
        {
            Check.IsNotEmptyOrWhiteSpace(entityType, "entityType");

            var jsonString = this.dataAccess.GetPublicData(entityType, regionName);

            if (!string.IsNullOrEmpty(jsonString) && !string.Equals(jsonString, Microsoft.Whitespace.Entities.Constants.ErrorMessageNoData, StringComparison.OrdinalIgnoreCase))
            {
                this.DataDownloadLogger.Log(TraceEventType.Information, LoggingMessageId.PortalDataDownload, "Data of entity type " + entityType + " has been downloaded for region " + regionName);
                return this.GetCSVOutofJson(jsonString);
            }

            var region = CommonUtility.GetRegionByName(regionName);
            this.DataDownloadAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
            this.DataDownloadAuditor.TransactionId = this.DataDownloadLogger.TransactionId;
            this.DataDownloadAuditor.Audit(AuditId.PublicDataDownload, AuditStatus.Failure, default(int), "No data found for entity type" + entityType + " for region " + regionName);
            this.DataDownloadLogger.Log(TraceEventType.Error, LoggingMessageId.PortalDataDownload, "No data found for entity type" + entityType + " for region " + regionName);

            return this.defaultResultString;
        }

        public string GetAuthorizedDeviceModels(string regionName = "United States")
        {
            var jsonString = this.dataAccess.GetAuthorizedDeviceModels(regionName);

            if (!string.IsNullOrEmpty(jsonString) && !string.Equals(jsonString, Microsoft.Whitespace.Entities.Constants.ErrorMessageNoData, StringComparison.OrdinalIgnoreCase))
            {
                this.DataDownloadLogger.Log(TraceEventType.Information, LoggingMessageId.PortalDataDownload, "Authorized Device Models data has been downloaded for region " + regionName);
                return this.GetCSVOutofJson(jsonString);
            }

            var region = CommonUtility.GetRegionByName(regionName);
            this.DataDownloadAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
            this.DataDownloadAuditor.TransactionId = this.DataDownloadLogger.TransactionId;
            this.DataDownloadAuditor.Audit(AuditId.PublicDataDownload, AuditStatus.Failure, default(int), "No data found for authorized device models for region " + regionName);
            this.DataDownloadLogger.Log(TraceEventType.Error, LoggingMessageId.PortalDataDownload, "No data found for authorized device models for region " + regionName);

            return this.defaultResultString;
        }

        public string GetUlsCallSigns(string regionName = "United States")
        {
            var lpAuxLicenses = this.whitespaceDataClient.GetAllUlsCallSigns(regionName).LpAuxLicenses;

            if (lpAuxLicenses != null)
            {
                var jsonString = JsonConvert.SerializeObject(lpAuxLicenses);

                if (!string.IsNullOrEmpty(jsonString) && !string.Equals(jsonString, Microsoft.Whitespace.Entities.Constants.ErrorMessageNoData, StringComparison.OrdinalIgnoreCase))
                {
                    this.DataDownloadLogger.Log(TraceEventType.Information, LoggingMessageId.PortalDataDownload, "Call signs data has been downloaded for region " + regionName);

                    return this.GetCSVOutofJson(jsonString);
                }
            }

            var region = CommonUtility.GetRegionByName(regionName);
            this.DataDownloadAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
            this.DataDownloadAuditor.TransactionId = this.DataDownloadLogger.TransactionId;
            this.DataDownloadAuditor.Audit(AuditId.PublicDataDownload, AuditStatus.Failure, default(int), "No data found for call signs for region " + regionName);
            this.DataDownloadLogger.Log(TraceEventType.Error, LoggingMessageId.PortalDataDownload, "No data found for call signs for region " + regionName);

            return this.defaultResultString;
        }

        public string GetUlsFileNumbers(string regionName = "United States")
        {
            var lpAuxLicenses = this.whitespaceDataClient.GetAllUlsFileNumbers(regionName).LpAuxLicenses;

            if (lpAuxLicenses != null)
            {
                var jsonString = JsonConvert.SerializeObject(lpAuxLicenses);

                if (!string.IsNullOrEmpty(jsonString) && !string.Equals(jsonString, Microsoft.Whitespace.Entities.Constants.ErrorMessageNoData, StringComparison.OrdinalIgnoreCase))
                {
                    this.DataDownloadLogger.Log(TraceEventType.Information, LoggingMessageId.PortalDataDownload, "uls file number data has been downloaded for region " + regionName);

                    return this.GetCSVOutofJson(jsonString);
                }
            }

            var region = CommonUtility.GetRegionByName(regionName);
            this.DataDownloadAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
            this.DataDownloadAuditor.TransactionId = this.DataDownloadLogger.TransactionId;
            this.DataDownloadAuditor.Audit(AuditId.PublicDataDownload, AuditStatus.Failure, default(int), "No data found for uls file numbers for region " + regionName);
            this.DataDownloadLogger.Log(TraceEventType.Error, LoggingMessageId.PortalDataDownload, "No data found for call signs for region " + regionName);

            return this.defaultResultString;
        }

        private string GetCSVOutofJson(string jsonString)
        {
            string delimator = ",";
            var result = new StringBuilder();

            XmlNode xml = JsonConvert.DeserializeXmlNode("{records:{record:" + jsonString + "}}");

            XmlDocument xmldoc = new XmlDocument();
            
            // Create XmlDoc Object
            xmldoc.LoadXml(xml.InnerXml);
            
            // Create XML Steam 
            var xmlReader = new XmlNodeReader(xmldoc);
            DataSet dataSet = new DataSet();
            
            // Load Dataset with Xml
            dataSet.ReadXml(xmlReader);

            var table = dataSet.Tables[0];

            for (int i = 0; i < table.Columns.Count; i++)
            {
                result.Append(table.Columns[i].ColumnName);
                result.Append(i == table.Columns.Count - 1 ? "\n" : delimator);
            }

            foreach (DataRow row in table.Rows)
            {
                for (int i = 0; i < table.Columns.Count; i++)
                {
                    result.Append(row[i].ToString().Contains(delimator) ? "\"" + row[i].ToString() + "\"" : row[i].ToString());
                    result.Append(i == table.Columns.Count - 1 ? "\n" : delimator);
                }
            }

            return result.ToString().TrimEnd(new char[] { '\r', '\n' });
        }
    }
}
