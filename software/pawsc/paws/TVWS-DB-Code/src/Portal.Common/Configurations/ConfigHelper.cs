// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Text;
    using System.Threading.Tasks;
    using System.Web;
    using Microsoft.Practices.Unity;
    using Microsoft.Practices.Unity.Configuration;
    using Microsoft.Whitespace.Common;
    using Microsoft.WindowsAzure.ServiceRuntime;    

    /// <summary>
    /// Holds all configured values both in web.config and corresponding authority config file
    /// </summary>
    public static class ConfigHelper
    {
        /// <summary>
        /// constant key for  Storage Connection
        /// </summary>
        private const string StorageConnectionStringKey = "AzureStorageAccountConnectionString";

        /// <summary>
        /// constant key for live client id
        /// </summary>
        private const string LiveClientIdKey = "LiveClientId";

        /// <summary>
        /// constant key for live secret id
        /// </summary>
        private const string LiveSecretClientIdKey = "LiveSecretClientId";

        private const string RequestScopesKey = "RequestScopes";

        private const string AuthorityKey = "Authority";

        private const string ContainerNamekey = "UnityContainerNames";

        private const string AuthorizeSchemaKey = "AuthorizationSchema";

        private const string BingCredentialKey = "BingCredential";

        private const string PawsApiKey = "PawsApi";

        static ConfigHelper()
        {
            CurrentContainer = Utils.Configuration.CurrentContainer;
        }

        /// <summary>
        /// Gets <see cref="StorageConnectionString"/> from configuration
        /// </summary>
        public static string StorageConnectionString
        {
            get
            {
                return RoleEnvironment.IsAvailable
                        ? RoleEnvironment.GetConfigurationSettingValue(StorageConnectionStringKey)
                        : ConfigurationManager.ConnectionStrings[StorageConnectionStringKey].ConnectionString;
            }
        }

        public static string PawsApi
        {
            get
            {
                return RoleEnvironment.IsAvailable
                        ? RoleEnvironment.GetConfigurationSettingValue(PawsApiKey)
                        : ConfigurationManager.AppSettings[PawsApiKey];
            }
        }

        /// <summary>
        /// Gets <see cref="LiveClientId"/> from configuration
        /// </summary>
        public static string LiveClientId
        {
            get
            {
                return RoleEnvironment.IsAvailable
                        ? RoleEnvironment.GetConfigurationSettingValue(LiveClientIdKey)
                        : ConfigurationManager.AppSettings[LiveClientIdKey];
            }
        }

        /// <summary>
        /// Gets <see cref="LiveSecretClientId"/> from configuration
        /// </summary>
        public static string LiveSecretClientId
        {
            get
            {
                return RoleEnvironment.IsAvailable
                        ? RoleEnvironment.GetConfigurationSettingValue(LiveSecretClientIdKey)
                        : ConfigurationManager.AppSettings[LiveSecretClientIdKey];
            }
        }

        /// <summary>
        /// Gets <see cref="RequestScopes"/> from configuration
        /// </summary>
        public static string RequestScopes
        {
            get
            {
                return RoleEnvironment.IsAvailable
                        ? RoleEnvironment.GetConfigurationSettingValue(RequestScopesKey)
                        : ConfigurationManager.AppSettings[RequestScopesKey];
            }
        }

        /// <summary>
        /// Gets <see cref="LiveSecretClientId"/> from configuration
        /// </summary>
        public static string Authority
        {
            get
            {
                return RoleEnvironment.IsAvailable
                        ? RoleEnvironment.GetConfigurationSettingValue(AuthorityKey)
                        : ConfigurationManager.AppSettings[AuthorityKey];
            }
        }

        /// <summary>
        /// Gets <see cref="LiveSecretClientId"/> from configuration
        /// </summary>
        public static string UnityContainerNames
        {
            get
            {
                return RoleEnvironment.IsAvailable
                        ? RoleEnvironment.GetConfigurationSettingValue(ContainerNamekey)
                        : ConfigurationManager.AppSettings[ContainerNamekey];
            }
        }

        /// <summary>
        /// Gets AuthorizationSchema from configuration
        /// </summary>
        public static string AuthorizationSchema
        {
            get
            {
                return RoleEnvironment.IsAvailable
                        ? RoleEnvironment.GetConfigurationSettingValue(AuthorizeSchemaKey)
                        : ConfigurationManager.AppSettings[AuthorizeSchemaKey];
            }
        }

        /// <summary>
        /// Gets bing credential key from configuration
        /// </summary>
        public static string BingCredential
        {
            get
            {
                return RoleEnvironment.IsAvailable
                        ? RoleEnvironment.GetConfigurationSettingValue(BingCredentialKey)
                        : ConfigurationManager.AppSettings[BingCredentialKey];
            }
        }

        /// <summary>
        /// Gets the configuration values that are loaded through authority config file
        /// </summary>
        public static IUnityContainer CurrentContainer { get; private set; }

        public static string GetApiAddressByKey(string apiKey)
        {
            return RoleEnvironment.IsAvailable
                        ? RoleEnvironment.GetConfigurationSettingValue(apiKey)
                        : ConfigurationManager.AppSettings[apiKey];
        }
    }
}
