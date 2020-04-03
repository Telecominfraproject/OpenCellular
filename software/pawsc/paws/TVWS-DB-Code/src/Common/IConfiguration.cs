// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System.Security.Cryptography.X509Certificates;
    using Microsoft.Practices.Unity;

    /// <summary>Defines all of the whitespace configuration methods.</summary>
    public interface IConfiguration
    {
        /// <summary>Gets the current region id.</summary>
        /// <value>The current region id.</value>
        int CurrentRegionId { get; }

        /// <summary>
        /// Gets the name of the current region.
        /// </summary>
        /// <value>The name of the current region.</value>
        string CurrentRegionName { get; }

        /// <summary>Gets the database connection string.</summary>
        /// <value>The database connection string.</value>
        string DbConnectionString { get; }

        /// <summary>Gets current Unity Container.</summary>
        /// <value>The current container.</value>
        /// <returns>A newly initialized and configured Unity container.</returns>
        IUnityContainer CurrentContainer { get; }

        /// <summary>Gets or sets the current configuration value.</summary>
        /// <param name="name">Name of the configuration field.</param>
        /// <returns>The configuration value.</returns>
        string this[string name] { get; }

        /// <summary>
        /// Determines whether the specified configuration settings name has key.
        /// </summary>
        /// <param name="configSettingsName">Name of the configuration settings.</param>
        /// <returns><c>true</c> if the specified configuration settings name has key; otherwise, <c>false</c>.</returns>
        bool HasSetting(string configSettingsName);
    }
}
