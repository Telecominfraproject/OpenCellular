// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.ValueProviders
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Reflection;
    using Entities;
    using Microsoft.Practices.Unity;

    /// <summary>
    ///     Represents PawsRegionalValueProvider
    /// </summary>
    public abstract class PawsRegionalValueProvider
    {
        /// <summary>
        /// Gets or sets the IDALCPaws.
        /// </summary>
        [Dependency]
        public IDalcPaws PawsDalc
        {
            get;
            set;
        }

        /// <summary>
        ///     Gets the device descriptor.
        /// </summary>
        /// <param name="objData">The object data.</param>
        /// <returns>returns DeviceDescriptor.</returns>
        public DeviceDescriptor GetDeviceDescriptor(dynamic objData)
        {
            if (this.CheckForProperty<DeviceDescriptor>(objData))
            {
                return objData.DeviceDescriptor;
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        ///     Checks for property.
        /// </summary>
        /// <typeparam name="T">type of object to check</typeparam>
        /// <param name="objData">The object data.</param>
        /// <returns><c>true</c> if exists, <c>false</c> otherwise</returns>
        public bool CheckForProperty<T>(dynamic objData)
        {
            Type returnType = objData.GetType();
            return returnType.GetRuntimeProperties().FirstOrDefault(prop => prop.Name == typeof(T).Name) != null;
        }

        /// <summary>
        ///     Gets the device id.
        /// </summary>
        /// <param name="objData">The object data.</param>
        /// <returns>returns deviceId.</returns>
        public abstract string GetDeviceId(dynamic objData);

        /// <summary>
        ///     Gets the device id.
        /// </summary>
        /// <param name="deviceDescriptor">The Device Descriptor.</param>
        /// <returns>returns rule set information.</returns>
        public abstract List<RulesetInfo> GetRuleSetInfo(DeviceDescriptor deviceDescriptor);        

        /// <summary>
        /// Gets the WSD information.
        /// </summary>
        /// <typeparam name="T">type of request</typeparam>
        /// <param name="request">The request.</param>
        /// <returns>returns Incumbent.</returns>
        public abstract Incumbent[] GetWSDInfo<T>(T request);

        /// <summary>
        /// Gets the incumbent information for get channel list.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns Microsoft.Whitespace.Entities.Incumbent[].</returns>
        public abstract Incumbent IncumbentInfoForGetChannelListRequest(Parameters parameters);
    }
}
