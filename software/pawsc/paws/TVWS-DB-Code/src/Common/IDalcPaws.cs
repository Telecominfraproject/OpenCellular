// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System.Collections.Generic;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.Storage.Blob;
    using WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents the Paws interface into the data access layer component.
    /// </summary>
    public interface IDalcPaws
    {
        /// <summary>
        /// Saves the specified spectrum usage.
        /// </summary>
        /// <param name="spectrumUsageTable">Register Request</param>
        /// <param name="regEntity">Register Request Entity</param>
        /// <returns>response of the saved Spectrum usage</returns>
        int NotifySpectrumUsage(string spectrumUsageTable, object regEntity);

        /// <summary>
        /// Returns Device Information of the specified Id.
        /// </summary>
        /// <param name="id">The device Id.</param>
        /// <returns>Device Descriptor information.</returns>
        DeviceDescriptor GetDeviceInfo(string id);

        /// <summary>
        /// Returns all of the device descriptors.
        /// </summary>
        /// <returns>All of the device descriptors.</returns>
        DeviceDescriptor[] GetDevices();

        /// <summary>
        /// Register method 
        /// </summary>
        /// <param name="registrationTable">Register Request</param>
        /// <param name="regEntity">Register Request Entity</param>
        /// <returns>response of the registration</returns>
        int RegisterDevice(string registrationTable, object regEntity);

        /// <summary>
        /// Initialize method of Paws DALC
        /// </summary>
        /// <param name="initializationTable">Initialize Request</param>
        /// <param name="regEntity">Initialize Request Entity</param>
        /// <returns>response of the initialization</returns>
        int Initialize(string initializationTable, object regEntity);

        /// <summary>
        /// Device Validation method of the IPawsDALC.
        /// </summary>
        /// <param name="validationTable">Device Validate Request</param>
        /// <param name="valuePartitionKey">Partition Key Entity</param>
        /// <param name="valueRowKey">Row Key Entity</param>
        /// <returns>response of the Device Validation</returns>
        int ValidateDevice(string validationTable, string valuePartitionKey, string valueRowKey);

        /// <summary>
        /// Fetch method of the IPawsDALC.
        /// </summary>
        /// <param name="request">Fetch Registration Type</param>
        /// <returns>response of the fetched device registration type</returns>
        string FetchDeviceRegistrationType(IRegisterRequest request);

        /// <summary>
        /// Fetches the entity.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="finalQuery">The query.</param>
        /// <returns>returns list.</returns>
        List<UsedSpectrum> GetUsedSpectrumDevices(string tableName, TableQuery<UsedSpectrum> finalQuery);

        /// <summary>
        /// Determines whether [is fixed TVDB registered] [the specified FCC id].
        /// </summary>
        /// <typeparam name="T">type of table to query</typeparam>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="deviceId">The device id.</param>
        /// <returns><c>true</c> if [is fixed TVDB registered] [the specified FCC id]; otherwise, <c>false</c>.</returns>
        bool IsDeviceRegistered<T>(string tableName, string deviceId) where T : ITableEntity, new();

        /// <summary>
        /// Updates the validated device.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <param name="entity">The entity.</param>
        void UpdateValidatedDevice(string tableName, TableEntity entity);

        /// <summary>
        /// Retrieves an entity.
        /// </summary>
        /// <param name="table">Table that is to be fetched.</param>
        /// <param name="keyValue">Value that is to be fetched.</param>
        /// <param name="value">Second value that is to be fetched.</param>
        /// <returns>Entity Name.</returns>
        object RetrieveEntity(string table, string keyValue, string value);
        
        /// <summary>
        /// Update Method.
        /// </summary>
        /// <param name="settings">Update Date or Sequence</param>
        void UpdateSettingsDateSequence(object settings);

         /// <summary>
        /// Fetches the Date or Sequence.
        /// </summary>
        /// <param name="tableName">Table name from where the rule set information is to be fetched.</param>
        /// <returns>returns RuleSetInfoEntity.</returns>
        List<RuleSetInfoEntity> GetRulesetInfo(string tableName);

        /// <summary>
        /// Retrieve the cloud blob data container
        /// </summary>
        /// <param name="dataContainer">The blob storage folder that contains all of the elevation files.</param>
        /// <returns>returns Cloud Blob Container</returns>
        CloudBlobContainer GetBlobContainer(string dataContainer);

        /// <summary>
        /// Retrieve the incumbents
        /// </summary>
        /// <param name="squareArea">The square Area.</param>
        /// <returns>returns Incumbents</returns>
        Incumbent[] GetIncumbents(SquareArea squareArea);
    }
}
