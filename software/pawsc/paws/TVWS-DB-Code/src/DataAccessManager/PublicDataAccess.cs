// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.DataAccessManager
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Entities;
    using Newtonsoft.Json;

    public class PublicDataAccess : IPublicDataAccess
    {
        private IHttpClientManager httpManager;

        public PublicDataAccess(IHttpClientManager httpClientManager)
        {
            this.httpManager = httpClientManager;
        }

        public string GetPublicDataWithEvents(string entityType, string regionName)
        {
            return this.GetJsonString(regionName, Microsoft.WhiteSpaces.Common.Constants.MethodGetPublicDataWithEvents, entityType);
        }

        public string GetPublicData(string entityType, string regionName)
        {
            return this.GetJsonString(regionName, Microsoft.WhiteSpaces.Common.Constants.MethodGetPublicData, entityType);
        }

        public string GetAuthorizedDeviceModels(string regionName)
        {
            return this.GetJsonString(regionName, Microsoft.WhiteSpaces.Common.Constants.MethodGetAuthorizedDeviceModels);
        }

        private string GetJsonString(string regionName, string methodName, string entityType = null)
        {
            Request request = new Request
            {
                Method = methodName,
                Params = new Parameters
                {
                    IncumbentType = entityType,
                }
            };

            RegionManagementResponse response = null;

            if (entityType != null)
            {
                var regionManagementResponsestring = this.httpManager.Post(request, regionName);

                response = JsonConvert.DeserializeObject<RegionManagementResponse>(regionManagementResponsestring);
            }
            else
            {
                response = this.httpManager.Get<RegionManagementResponse>(request, regionName);
            }

            if (response != null)
            {
                var error = response.Error;

                if (error == null)
                {
                    if (response.Result.IncumbentList != null)
                    {
                        return JsonConvert.SerializeObject(response.Result.IncumbentList);
                    }
                }
                else if (string.Equals(error.Data, Microsoft.Whitespace.Entities.Constants.ErrorMessageNoData, StringComparison.OrdinalIgnoreCase))
                {
                    return Microsoft.Whitespace.Entities.Constants.ErrorMessageNoData;
                }
                else
                {
                    throw new ResponseErrorException(error.Type, error.Data, error.Code, error.Message);
                }
            }

            return string.Empty;
        }
    }
}
