// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Linq;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    /// <summary>
    ///     Json converter for paws device owner information.
    /// </summary>
    public class RulesetInfoConverter : BaseJsonConverter
    {
        /// <summary>
        ///     Logic called while de-serializing json string to object
        /// </summary>
        /// <param name="reader">The serialized json reader data.</param>
        /// <param name="objectType">The type of object being de-serialized.</param>
        /// <param name="existingValue">The current value.</param>
        /// <param name="serializer">Controls how the object is de-serialized.</param>
        /// <returns>Returns the de-serialized object.</returns>
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            RulesetInfo rulesetInfo = existingValue as RulesetInfo ?? new RulesetInfo();

            JObject jsonObject = JObject.Load(reader);
            var properties = jsonObject.Properties().ToList();

            for (int i = 0; i < properties.Count; i++)
            {
                var jsonProp = properties[i];
                if (!jsonProp.HasValues)
                {
                    continue;
                }

                switch (jsonProp.Name)
                {
                    case Constants.PropertyNameAuthority:
                        rulesetInfo.Authority = jsonProp.ToObject<string>();
                        break;

                    case Constants.PropertyNameMaxLocationChange:
                        rulesetInfo.MaxLocationChange = jsonProp.ToObject<double>();
                        break;

                    case Constants.PropertyNameMaxPollingSecs:
                        rulesetInfo.MaxPollingSecs = jsonProp.ToObject<int>();
                        break;

                    case Constants.PropertyNameMaxEirpHz:
                        rulesetInfo.MaxEirpHz = jsonProp.ToObject<double>();
                        break;

                    case Constants.PropertyNameMaxNominalChannelBwMhz:
                        rulesetInfo.MaxNominalChannelBwMhz = jsonProp.ToObject<double>();
                        break;

                    case Constants.PropertyNameMaxTotalBwMhz:
                        rulesetInfo.MaxTotalBwMhz = jsonProp.ToObject<double>();
                        break;

                    case Constants.PropertyNameRulesetIds:
                        rulesetInfo.RulesetId = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<string>() : null;
                        break;

                    //// default:
                    ////    rulesetInfo.UnKnownTypes.Add(value, reader.ReadAsString());
                    ////    break;
                }
            }

            return rulesetInfo;
        }
    }
}
