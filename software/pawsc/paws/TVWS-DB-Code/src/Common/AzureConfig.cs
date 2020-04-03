// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.Configuration;
    using System.IO;
    using System.Linq;
    using System.Timers;
    using System.Web;
    using System.Xml;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.ServiceRuntime;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;
    using Practices.EnterpriseLibrary.Common.Configuration;
    using Practices.Unity.Configuration;
    using Practices.Unity.Configuration.ConfigurationHelpers;

    /// <summary>
    /// Represents an azure config instance.
    /// </summary>
    public sealed class AzureConfig : IConfiguration, IDisposable
    {
        /// <summary>
        /// The internal settings
        /// </summary>
        private NameValueCollection internalSettings = new NameValueCollection();

        /// <summary>
        /// Gets or sets the synchronize timer.
        /// </summary>
        /// <value>The synchronize timer.</value>
        private Timer syncTimer;

        /// <summary>
        /// current container
        /// </summary>
        private IUnityContainer currentContainer;

        /// <summary>
        /// Initializes a new instance of the AzureConfig class.
        /// </summary>
        public AzureConfig()
        {
            if (RoleEnvironment.IsAvailable)
            {
                this.DbConnectionString = RoleEnvironment.GetConfigurationSettingValue("DBConnectionString");
            }
            else
            {
                this.DbConnectionString = ConfigurationManager.AppSettings["DBConnectionString"];
            }

            this.InitConfigurationSettings(this.ReadRegionIdAndRegionNameFromAppConfig);
        }

        public AzureConfig(int regionId, string regionName)
        {
            if (string.IsNullOrWhiteSpace(regionName))
            {
                throw new ArgumentException("regionName");
            }

            if (RoleEnvironment.IsAvailable)
            {
                this.DbConnectionString = RoleEnvironment.GetConfigurationSettingValue("DBConnectionString");
            }
            else
            {
                this.DbConnectionString = ConfigurationManager.AppSettings["DBConnectionString"];
            }

            var table = CloudStorageAccount.Parse(this.DbConnectionString).CreateCloudTableClient().GetTableReference(regionName + regionId + Constants.ConfigTableName);

            if (!table.Exists())
            {
                throw new ArgumentException(string.Format("Invalid region Id {0},Table: {1}{2} doesn't exist", regionId, regionId, regionName));
            }

            this.CurrentRegionId = regionId;
            this.CurrentRegionName = regionName;
            this.InitConfigurationSettings();
        }

        /// <summary>
        /// Gets the database connection string.
        /// </summary>
        /// <value>The database connection string.</value>
        public string DbConnectionString { get; private set; }

        /// <summary>
        /// Gets the region code.
        /// </summary>
        /// <value>The region code.</value>
        public int CurrentRegionId
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the name of the current region.
        /// </summary>
        /// <value>The name of the current region.</value>
        public string CurrentRegionName
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets current Unity Container.
        /// </summary>
        /// <value>The current container.</value>
        /// <returns>A newly initialized and configured Unity container.</returns>
        public IUnityContainer CurrentContainer
        {
            get
            {
                if (this.currentContainer == null)
                {
                    this.CreateUnityContainer();
                }

                return this.currentContainer;
            }
        }

        /// <summary>
        /// Gets or sets the current configuration value.
        /// </summary>
        /// <param name="name">Name of the configuration field.</param>
        /// <returns>The configuration value.</returns>
        public string this[string name]
        {
            get
            {
                return this.internalSettings[name];
            }
        }

        /// <summary>
        /// Determines whether the specified configuration settings name has key.
        /// </summary>
        /// <param name="configSettingsName">Name of the configuration settings.</param>
        /// <returns><c>true</c> if the specified configuration settings name has key; otherwise, <c>false</c>.</returns>
        public bool HasSetting(string configSettingsName)
        {
            return this.internalSettings.AllKeys.FirstOrDefault(obj => obj.ToLower() == configSettingsName.ToLower()) != null;
        }

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
        /// </summary>
        public void Dispose()
        {
            this.syncTimer.Dispose();
        }

        /// <summary>
        /// Gets the regional settings.
        /// </summary>
        /// <param name="readRegionIdAndName">An action delegate to load settings from AppConfig.</param>
        private void InitConfigurationSettings(Action readRegionIdAndName = null)
        {
            foreach (var configKey in ConfigurationManager.AppSettings.AllKeys)
            {
                if (this.internalSettings.AllKeys.FirstOrDefault(obj => obj == configKey) == null)
                {
                    this.internalSettings.Add(configKey, ConfigurationManager.AppSettings[configKey]);
                }
                else
                {
                    this.internalSettings[configKey] = ConfigurationManager.AppSettings[configKey];
                }
            }

            if (readRegionIdAndName != null)
            {
                readRegionIdAndName();
            }

            var table = CloudStorageAccount.Parse(this.DbConnectionString).CreateCloudTableClient().GetTableReference(this.CurrentRegionName + this.CurrentRegionId + Constants.ConfigTableName);
            var settings = table.ExecuteQuery(new TableQuery<DynamicTableEntity>());

            foreach (var dynamicTableEntity in settings)
            {
                if (this.internalSettings.AllKeys.FirstOrDefault(obj => obj == dynamicTableEntity.RowKey) == null)
                {
                    this.internalSettings.Add(dynamicTableEntity.RowKey, dynamicTableEntity["ConfigValue"].StringValue);
                }
                else
                {
                    this.internalSettings[dynamicTableEntity.RowKey] = dynamicTableEntity["ConfigValue"].StringValue;
                }
            }

            if (this.syncTimer == null)
            {
                this.syncTimer = new Timer(this.internalSettings["SyncTimer_MilliSec"].ToInt32());
                this.syncTimer.AutoReset = false;

                if (readRegionIdAndName != null)
                {
                    this.syncTimer.Elapsed += this.SyncTimer_Elapsed;
                }
                else
                {
                    this.syncTimer.Elapsed += (sender, args) =>
                        {
                            this.InitConfigurationSettings();
                        };
                }

                this.syncTimer.Start();
            }
            else
            {
                this.syncTimer.Interval = this.internalSettings["SyncTimer_MilliSec"].ToInt32();
            }
        }

        private void ReadRegionIdAndRegionNameFromAppConfig()
        {
            if (RoleEnvironment.IsAvailable)
            {
                // Read the value from the Azure cscfg file
                this.CurrentRegionId = RoleEnvironment.GetConfigurationSettingValue("RegionId").ToInt32();
                this.CurrentRegionName = RoleEnvironment.GetConfigurationSettingValue("RegionName");
            }
            else
            {
                this.CurrentRegionId = this.internalSettings["RegionId"].ToInt32();
                this.CurrentRegionName = this.internalSettings["RegionName"];
            }
        }

        /// <summary>
        /// Handles the Elapsed event of the SyncTimer control.
        /// </summary>
        /// <param name="sender">The source of the event.</param>
        /// <param name="e">The <see cref="ElapsedEventArgs"/> instance containing the event data.</param>
        private void SyncTimer_Elapsed(object sender, ElapsedEventArgs e)
        {
            this.InitConfigurationSettings(this.ReadRegionIdAndRegionNameFromAppConfig);
        }

        /// <summary>
        /// Instantiates a new Unity Container and initializes it with the config 
        /// value stored in the config table under the name "UnityConfig".
        /// </summary>
        private void CreateUnityContainer()
        {
            IUnityContainer container = new UnityContainer();
            string filePath = string.Empty;

            if (RoleEnvironment.IsAvailable)
            {
                filePath = Environment.GetEnvironmentVariable("RoleRoot") + "\\approot\\bin\\Unity.config";
                if (!File.Exists(filePath))
                {
                    filePath = Environment.GetEnvironmentVariable("RoleRoot") + "\\approot\\Unity.config";
                }
            }
            else if (HttpContext.Current != null && HttpContext.Current.Server != null)
            {
                filePath = HttpContext.Current.Server.MapPath(@"~\bin") + "\\Unity.config";
            }
            else
            {
                filePath = "Unity.config";
            }

            XmlReader reader = new XmlTextReader(filePath);
            XmlUnityConfigurationSection streamConfig = new XmlUnityConfigurationSection();
            streamConfig.DeserializeFromXml(reader);

            foreach (string containerName in Utils.UnityContainers)
            {
                streamConfig.Configure(container, containerName.Trim());
            }

            this.currentContainer = container;
        }
    }
}
