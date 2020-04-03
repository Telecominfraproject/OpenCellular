// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Xml.Serialization;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    /// <summary>
    /// Converts a json VCard representation into a VCard instance.
    /// </summary>
    internal class VcardConverter : BaseJsonConverter
    {
        /// <summary>
        /// Logic called while de-serializing json string to object
        /// </summary>
        /// <param name="reader">The serialized json reader data.</param>
        /// <param name="objectType">The type of object being de-serialized.</param>
        /// <param name="existingValue">The current value.</param>
        /// <param name="serializer">Controls how the object is de-serialized.</param>
        /// <returns>Returns the de-serialized object.</returns>
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            Vcard vcard = existingValue as Vcard ?? new Vcard();

            JObject jsonObject = JObject.Load(reader);
            var properties = jsonObject.Properties().ToList();

            for (int i = 0; i < properties.Count; i++)
            {
                var jsonProp = properties[i];
                switch (jsonProp.Name)
                {
                    case Constants.PropertyNameFullName:
                        vcard.FullName = jsonProp.Value.ToObject<string>();
                        break;

                    case Constants.PropertyNameOrganization:
                        vcard.Organization = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Organization>() : null;
                        break;

                    case Constants.PropertyNameAddress:
                        vcard.Address = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Address>() : null;
                        break;

                    case Constants.PropertyNamePhone:
                        vcard.Phone = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Telephone>() : null;
                        break;
                    case Constants.PropertyNameEmail:
                        vcard.Email = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Email>() : null;
                        break;

                    default:
                        vcard.UnKnownTypes.Add(jsonProp.Name, jsonProp.ToObject<string>());
                        break;
                }
            }

            return vcard;
        }
    }
}
