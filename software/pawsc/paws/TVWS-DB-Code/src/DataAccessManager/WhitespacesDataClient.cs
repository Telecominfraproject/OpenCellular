// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.DataAccessManager
{
    using System;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Entities;
    using MWEC = Microsoft.Whitespace.Entities.Constants;
    using System.Threading.Tasks;

    public class WhitespacesDataClient : IWhitespacesDataClient
    {
        private IHttpClientManager httpManager;

        public WhitespacesDataClient(IHttpClientManager httpClientManager)
        {
            this.httpManager = httpClientManager;
        }

        public Result AddIncumbent(IRequestParameters requestParams)
        {
            var regionManagementResponse = this.GetRegionManagementResponse(requestParams, Microsoft.WhiteSpaces.Common.Constants.MethodAddIncumbent, requestParams.AccessToken);

            return this.GetResultFromResponse(regionManagementResponse);
        }

        public Result GetChannelList(IRequestParameters requestParams)
        {
            var regionManagementResponse = this.GetRegionManagementResponse(requestParams, Microsoft.WhiteSpaces.Common.Constants.MethodGetChannelList);

            return this.GetResultFromResponse(regionManagementResponse);
        }

        public Result GetIncumbents(IRequestParameters requestParams)
        {
            //// TODO: There should two GetIncumbents overloaded methods, one with authentication header and other without authentication header.
            //// as it is not neccessary that user has to be authenticated to get incumbents all the time.
            //// this.httpManager.SetAuthorizationHeader(requestParams.AccessToken);
            var regionManagementResponse = this.GetRegionManagementResponse(requestParams, Microsoft.WhiteSpaces.Common.Constants.MethodGetIncumbents);

            return this.GetResultFromResponse(regionManagementResponse);
        }

        public Result DeleteIncumbent(IRequestParameters requestParams)
        {
            var regionManagementResponse = this.GetRegionManagementResponse(requestParams, Microsoft.WhiteSpaces.Common.Constants.MethodDeleteIncumbent, requestParams.AccessToken);

            return this.GetResultFromResponse(regionManagementResponse);
        }

        public Result GetDevices(IRequestParameters requestParams)
        {
            var regionManagementResponse = this.GetRegionManagementResponse(requestParams, Microsoft.WhiteSpaces.Common.Constants.MethodGetDeviceList);

            return this.GetResultFromResponse(regionManagementResponse);
        }

        public Result ExcludeChannel(IRequestParameters requestParams)
        {
            var regionManagementResponse = this.GetRegionManagementResponse(requestParams, Microsoft.WhiteSpaces.Common.Constants.MethodExcludeChannel);

            return this.GetResultFromResponse(regionManagementResponse);
        }

        public Result ExcludeIds(IRequestParameters requestParams)
        {
            var regionManagementResponse = this.GetRegionManagementResponse(requestParams, Microsoft.WhiteSpaces.Common.Constants.MethodExcludeIds);

            return this.GetResultFromResponse(regionManagementResponse);
        }

        public Result GetNearByTvStations(IRequestParameters requestParams)
        {
            var regionManagementResponse = this.GetRegionManagementResponse(requestParams, Microsoft.WhiteSpaces.Common.Constants.MethodSearchTvStations);

            return this.GetResultFromResponse(regionManagementResponse);
        }

        public async Task<Result> GetMvpdCallsignInfoAsync(string callsign,string regionName)
        {
            Request request = new Request
            {
                Method = Microsoft.WhiteSpaces.Common.Constants.MethodGetMVPDCallSignInfo,                
            };

            RegionManagementResponse response = await this.httpManager.GetAsync<RegionManagementResponse>(request, regionName, callsign);
            var error = response.Error;

            if (error != null)
            {
                throw new ResponseErrorException(error.Type, error.Data, error.Code, error.Message);
            }

            return response.Result;
        }

        public Result GetAllUlsFileNumbers(string regionName)
        {
            return this.GetUlsData(Microsoft.WhiteSpaces.Common.Constants.MethodGetULSFileNumbers, regionName);
        }

        public Result GetAllUlsCallSigns(string regionName)
        {
            return this.GetUlsData(Microsoft.WhiteSpaces.Common.Constants.MethodGetULSCallSigns, regionName);
        }

        public void SubmitPawsInterference(IRequestParameters requestParams)
        {
            Request request = new Request
            {
                Method = Microsoft.Whitespace.Entities.Constants.MethodNameInterferenceQuery,
                Id = "1",
                JsonRpc = "2.0",
                Params = requestParams.Params
            };

            PawsResponse response = this.httpManager.Post<PawsResponse>(request, requestParams.RegionName, null);

            var error = response.Error;

            if (error == null)
            {
                throw new ResponseErrorException(error.Type, error.Data, error.Code, error.Message);
            }
        }

        private Result GetUlsData(string method, string regionName)
        {
            Request request = new Request
            {
                Method = method
            };

            var response = this.httpManager.Get<RegionManagementResponse>(request, regionName);
            var error = response.Error;

            if (error != null)
            {
                throw new ResponseErrorException(error.Type, error.Data, error.Code, error.Message);
            }

            return response.Result;
        }

        private Result GetResultFromResponse(RegionManagementResponse response)
        {
            var error = response.Error;

            if (error == null)
            {
                return response.Result;
            }
            else if (error != null && string.Compare(error.Data, MWEC.ErrorMessageNoData, StringComparison.OrdinalIgnoreCase) == 0)
            {
                return response.Error;
            }
            else
            {
                throw new ResponseErrorException(error.Type, error.Data, error.Code, error.Message);
            }
        }

        private RegionManagementResponse GetRegionManagementResponse(IRequestParameters requestParams, string methodName, string accessToken = null)
        {
            Request request = new Request
            {
                Method = methodName,
                Params = requestParams.Params
            };

            if (accessToken == null)
            {
                accessToken = requestParams.AccessToken;
            }

            Microsoft.WhiteSpaces.Common.Check.IsNotNull(requestParams.RegionName, "Region Name");

            return this.httpManager.Post<RegionManagementResponse>(request, requestParams.RegionName, accessToken);
        }        
    }
}
