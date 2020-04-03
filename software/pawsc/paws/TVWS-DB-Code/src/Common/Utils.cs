// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.ServiceModel;
    using System.Text;
    using System.Web.Configuration;
    using System.Xml;
    using System.Xml.Linq;
    using System.Xml.Serialization;
    using Microsoft.Practices.EnterpriseLibrary.Common.Configuration;
    using Microsoft.Practices.EnterpriseLibrary.Logging;
    using Microsoft.Practices.EnterpriseLibrary.Logging.Configuration;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.Diagnostics;
    using Microsoft.WindowsAzure.ServiceRuntime;
    using Practices.Unity.Configuration.ConfigurationHelpers;

    /// <summary>
    /// Static helper class used to access the configuration store.
    /// </summary>
    public static class Utils
    {
        /// <summary>
        /// The web.config/app.config app settings field name that contains the fully qualified class name of the IConfiguration component.
        /// </summary>
        private const string DefaultConfigFieldName = "DefaultConfig";

        /// <summary>
        /// Contains a reference to the instantiated IConfiguration component.
        /// </summary>
        private static IConfiguration defaultConfig = null;

        /// <summary>
        /// Initializes static members of the Utils class.
        /// </summary>
        static Utils()
        {
            string configClassName = ConfigurationManager.AppSettings[DefaultConfigFieldName];

            if (!string.IsNullOrEmpty(configClassName))
            {
                string assemblyName = configClassName.Substring(0, configClassName.LastIndexOf('.'));
                defaultConfig = Activator.CreateInstance(AppDomain.CurrentDomain, assemblyName, configClassName).Unwrap() as IConfiguration;
            }
            else
            {
                // No config specified, so just go with AzureConfig.
                defaultConfig = new AzureConfig();
            }

            Initialize();
        }

        /// <summary>
        /// Gets the terrain database connection string.
        /// </summary>
        /// <value>The terrain database connection string.</value>
        public static string TerrainDbConnectionString { get; private set; }

        /// <summary>
        /// Gets the default Configuration store instance.
        /// </summary>
        public static IConfiguration Configuration
        {
            get
            {
                return Utils.defaultConfig;
            }
        }

        /// <summary>
        /// Gets the user id.
        /// </summary>
        /// <value>The user id.</value>
        public static string UserId
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the current region id.
        /// </summary>
        /// <value>The current region id.</value>
        public static int CurrentRegionId
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the authority.
        /// </summary>
        /// <value>The authority.</value>
        public static string Authority
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the registration id org.
        /// </summary>
        /// <value>The registration id org.</value>
        public static string RegistrationIdOrg
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the name of the current region.
        /// </summary>
        /// <value>The name of the current region.</value>
        public static string CurrentRegionName
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the region prefix.
        /// </summary>
        /// <value>The region prefix.</value>
        public static string CurrentRegionPrefix
        {
            get
            {
                return CurrentRegionName + CurrentRegionId;
            }
        }

        /// <summary>
        /// Gets or sets the unity containers.
        /// </summary>
        /// <value>The unity containers.</value>
        public static string[] UnityContainers
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the Bing Key.
        /// </summary>
        /// <value>The Bing Key.</value>
        public static string BingKey
        {
            get;
            private set;
        }


        /// <summary>
        /// Initializes this instance.
        /// </summary>
        public static void Initialize()
        {
            CurrentRegionId = defaultConfig.CurrentRegionId;
            UserId = defaultConfig["UserId"];
            RegistrationIdOrg = defaultConfig["RegistrationIdOrg"];
            Authority = defaultConfig["Authority"];
            CurrentRegionName = defaultConfig.CurrentRegionName;
            TerrainDbConnectionString = defaultConfig["DBConnectionStringTerrain"];
            BingKey = defaultConfig["BingCredential"];

            if (RoleEnvironment.IsAvailable)
            {
                UnityContainers = string.Format(RoleEnvironment.GetConfigurationSettingValue("UnityContainerNames"), CurrentRegionPrefix).Split(',');
            }
            else
            {
                UnityContainers = string.Format(defaultConfig["UnityContainerNames"], Utils.CurrentRegionPrefix).Split(',');
            }
        }

        /// <summary>
        /// Gets the name of the regional table.
        /// </summary>
        /// <param name="tableName">Name of the table.</param>
        /// <returns>returns System.String.</returns>
        public static string GetRegionalTableName(string tableName)
        {
            return CurrentRegionPrefix + tableName;
        }

        /// <summary>
        /// DeserializeXMLToObject DeSerializes the xml to Generic object.
        /// </summary>
        /// <typeparam name="T">generic object</typeparam>
        /// <param name="clsObject"> Generic object to be serialized</param>
        /// <param name="xml">xml to be deserialized</param>
        public static void DeserializeXMLToObject<T>(ref T clsObject, string xml)
        {
            StringReader reader = new StringReader(xml);
            XmlSerializer serializer = new XmlSerializer(typeof(T));
            clsObject = (T)serializer.Deserialize(reader);
        }

        /// <summary>
        /// converts date to string with the z format.
        /// </summary>
        /// <param name="date"> date time</param>
        /// <returns>Converted Date</returns>
        public static string ConvertDateZ(DateTime date)
        {
            string month = date.Month.ToString();
            string day = date.Day.ToString();
            string hour = date.Hour.ToString();
            string min = date.Minute.ToString();
            string second = date.Second.ToString();

            if (month.Length == 1)
            {
                month = "0" + month;
            }

            if (day.Length == 1)
            {
                day = "0" + day;
            }

            if (hour.Length == 1)
            {
                hour = "0" + hour;
            }

            if (min.Length == 1)
            {
                min = "0" + min;
            }

            if (second.Length == 1)
            {
                second = "0" + second;
            }

            string datetimez = date.Year.ToString() + "-" + month + "-" + day + "T" + hour + ":" + min + ":" + second + "." + date.Millisecond + "Z";
            return datetimez;
        }

        /// <summary>
        /// Gets the file path.
        /// </summary>
        /// <returns>returns System.String.</returns>
        public static string GetValidAssemblyPath()
        {
            string directoryPath = string.Empty;
            if (RoleEnvironment.IsAvailable)
            {
                directoryPath = Environment.GetEnvironmentVariable("RoleRoot") + "\\approot\\bin\\";
                if (!Directory.Exists(directoryPath))
                {
                    directoryPath = Environment.GetEnvironmentVariable("RoleRoot") + "\\approot\\";
                }
            }
            else
            {
                directoryPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory + "\\bin\\");
            }

            return directoryPath;
        }

        /// <summary>
        /// Gets the file path.
        /// </summary>
        /// <param name="configKeyName">Name of the configuration key.</param>
        /// <param name="fileNameSuffix">The file name suffix.</param>
        /// <returns>returns System.String.</returns>
        public static string GetFilePathForConfigKey(string configKeyName, string fileNameSuffix = null)
        {
            string fileName = Configuration[configKeyName];
            if (fileNameSuffix != null)
            {
                fileName += fileNameSuffix;
            }

            string filePath = string.Empty;
            if (RoleEnvironment.IsAvailable)
            {
                filePath = Environment.GetEnvironmentVariable("RoleRoot") + "\\approot\\bin\\" + fileName;
                if (!File.Exists(filePath))
                {
                    filePath = Environment.GetEnvironmentVariable("RoleRoot") + "\\approot\\" + fileName;
                }
            }
            else
            {
                filePath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, fileName);
            }

            return filePath;
        }

        /// <summary>
        /// Gets the output directory path for configuration key.
        /// </summary>
        /// <param name="configKeyName">Name of the configuration key.</param>
        /// <param name="fileNameSuffix">The file name suffix.</param>
        /// <returns>returns System.String.</returns>
        public static string GetOutputDirPathForConfigKey(string configKeyName, string fileNameSuffix = null)
        {
            string fileName = Configuration[configKeyName];
            if (fileNameSuffix != null)
            {
                fileName += fileNameSuffix;
            }

            string filePath = string.Empty;
            if (RoleEnvironment.IsAvailable)
            {
                filePath = Environment.GetEnvironmentVariable("RoleRoot") + "\\" + fileName;
            }
            else
            {
                filePath = AppDomain.CurrentDomain.BaseDirectory + fileName;
            }

            return filePath;
        }

        /// <summary>
        /// Configures the logging settings according to RoleEnvironment.
        /// </summary>
        /// <returns>returns LoggingSettings.</returns>
        public static LoggingSettings GetLoggingSettings()
        {
            var loggingStream = File.OpenRead(Path.Combine(GetValidAssemblyPath(), "Logging.config"));
            XmlReaderSettings readerSettings = new XmlReaderSettings();
            readerSettings.IgnoreWhitespace = true;
            var xmlReader = XmlReader.Create(loggingStream, readerSettings);

            XDocument loggingSettingsDoc = XDocument.Load(xmlReader);

            if (RoleEnvironment.IsAvailable)
            {
                var fileListeners = loggingSettingsDoc.Root.Descendants("listeners").Elements("add").Where(obj => obj.Attribute("type") != null && obj.Attribute("type").Value.Contains("Microsoft.Practices.EnterpriseLibrary.Logging.TraceListeners.FlatFileTraceListener"));
                foreach (var fileListener in fileListeners)
                {
                    var listenerName = fileListener.Attribute("name").Value;
                    var listeners = loggingSettingsDoc.Root.Descendants("listeners").Elements("add").Where(obj => obj.Attribute("type") == null && obj.Attribute("name").Value == listenerName);
                    listeners.Remove();
                }

                fileListeners.Remove();
            }
            else
            {
                var azureListeners = loggingSettingsDoc.Root.Descendants("listeners").Elements("add").Where(obj => obj.Attribute("type") != null && obj.Attribute("type").Value.Contains("Microsoft.WindowsAzure"));
                foreach (var azureListener in azureListeners)
                {
                    var listenerName = azureListener.Attribute("name").Value;
                    var listeners = loggingSettingsDoc.Root.Descendants("listeners").Elements("add").Where(obj => obj.Attribute("type") == null && obj.Attribute("name").Value == listenerName);
                    listeners.Remove();
                }

                azureListeners.Remove();
            }

            StringBuilder settingsBuilder = new StringBuilder();
            using (XmlWriter finalWriter = XmlWriter.Create(settingsBuilder))
            {
                loggingSettingsDoc.WriteTo(finalWriter);
                finalWriter.Flush();
            }

            LoggingSettings settings = new LoggingSettings();
            settings.ReadXml(XmlReader.Create(new StringReader(settingsBuilder.ToString())));

            return settings;
        }

        /// <summary>
        /// Starts the diagnostics.
        /// </summary>
        public static void InitDiagnostics()
        {
            // Get default initial configuration.
            var config = DiagnosticMonitor.GetDefaultInitialConfiguration();
            var logLevelTransferTime = RoleEnvironment.GetConfigurationSettingValue("LogLevelTransferTime").ToInt32();
            if (logLevelTransferTime == 0)
            {
                logLevelTransferTime = 5;
            }

            config.Logs.ScheduledTransferPeriod = TimeSpan.FromMinutes(logLevelTransferTime);

            // Start the diagnostic monitor with the modified configuration.
            DiagnosticMonitor.Start("AzureStorageAccountConnectionString", config);
        }

        /// <summary>
        /// Gets the native service client.
        /// </summary>
        /// <returns>returns NativeMethodServiceClient.</returns>
        public static NativeMethodServiceClient GetNativeServiceClient()
        {
            EndpointAddress address = new EndpointAddress("net.pipe://localhost/NativeCodeService");
            NetNamedPipeBinding binding = new NetNamedPipeBinding();
            binding.ReceiveTimeout = TimeSpan.MaxValue;
            binding.SendTimeout = TimeSpan.MaxValue;
            ////binding.MaxReceivedMessageSize = 40000000;
            ////binding.ReaderQuotas.MaxArrayLength = int.MaxValue;
            ////binding.ReaderQuotas.MaxStringContentLength = int.MaxValue;
            var serviceClient = new NativeMethodServiceClient(binding, address);
            return serviceClient;
        }

        /// <summary>
        /// Gets the valid file path.
        /// </summary>
        /// <returns>returns System.String.</returns>
        public static string GetLocalStorePath()
        {
            string filePath = string.Empty;
            filePath = RoleEnvironment.GetLocalResource("LocalCacheStore").RootPath;
            return filePath;
        }

        /// <summary>
        /// Gets the local store path.
        /// </summary>
        /// <param name="fileNameWithoutExtension">The file name without extension.</param>
        /// <returns>returns System.String.</returns>
        public static string GetLocalStorCacheFileName(string fileNameWithoutExtension)
        {
            return GetLocalStorePath() + fileNameWithoutExtension + ".cache";
        }

        /// <summary>
        /// Checks the file exists.
        /// </summary>
        /// <param name="fileName">Name of the file.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public static bool CheckFileExists(string fileName)
        {
            string filePath = Utils.GetLocalStorCacheFileName(fileName);
            if (File.Exists(filePath))
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Dumps the cache file.
        /// </summary>
        /// <typeparam name="T">object type</typeparam>
        /// <param name="table">The table.</param>
        /// <param name="fileName">Name of the file.</param>
        /// <exception cref="System.Exception">Incorrect Path :  + filePath</exception>
        public static void WriteCacheFile<T>(List<T> table, string fileName)
        {
            if (table.Count == 0)
            {
                return;
            }

            List<string> fileData = new List<string>();

            for (int i = 0; i < table.Count; i++)
            {
                fileData.Add(JsonSerialization.SerializeObject(table[i]));
            }

            string filePath = Utils.GetLocalStorCacheFileName(fileName);

            File.WriteAllLines(filePath, fileData);
        }

        /// <summary>
        /// Reads the dumped file.
        /// </summary>
        /// <typeparam name="T">type of object</typeparam>
        /// <param name="fileName">Name of the file.</param>
        /// <returns>returns System.Collections.Generic.List&lt;T&gt;.</returns>
        public static List<T> ReadDumpedFile<T>(string fileName)
        {
            List<T> dumpedData = new List<T>();
            try
            {
                string filePath = Utils.GetLocalStorCacheFileName(fileName);

                if (!File.Exists(filePath))
                {
                    throw new Exception("Incorrect Path : " + filePath);
                }

                var fileData = File.ReadAllLines(filePath);
                if (fileData == null)
                {
                    throw new Exception("File Not Exist");
                }

                for (int i = 0; i < fileData.Length; i++)
                {
                    dumpedData.Add(JsonSerialization.DeserializeString<T>(fileData[i]));
                }
            }
            catch (Exception ex)
            {
                throw new Exception(ex.ToString());
            }

            return dumpedData;
        }
    }
}
