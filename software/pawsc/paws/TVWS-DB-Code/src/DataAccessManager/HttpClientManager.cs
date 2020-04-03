// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.DataAccessManager
{
    using System;
    using System.IO;
    using System.Net.Http;
    using System.Net.Http.Headers;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Entities;

    public class HttpClientManager : IHttpClientManager
    {
        private readonly string authorizeSchema;

        public HttpClientManager(string authorizeSchema)
        {
            this.authorizeSchema = authorizeSchema;
        }

        public string WorkingRegion { get; set; }

        public string Post(Request request, string regionName, string accessToken = null)
        {
            HttpResponseMessage response = this.GetPostResponse(request, regionName, accessToken);

            return response.Content.ReadAsStringAsync().Result;
        }

        public T Post<T>(Request request, string regionName, string accessToken = null) where T : class
        {
            HttpResponseMessage response = this.GetPostResponse(request, regionName, accessToken);

            return response.Content.ReadAsAsync<T>().Result;
        }

        public async Task<T> PostAsync<T>(Request request, string regionName, string accessToken = null) where T : class
        {
            var baseAddress = string.Empty;

            try
            {
                var requestStream = this.GetRequestStream(request);
                string requestUrl = request.Method;

                using (var httpClient = this.GetHttpClient(regionName, accessToken))
                {
                    baseAddress = httpClient.BaseAddress.ToString();

                    HttpResponseMessage response = await httpClient.PostAsync(requestUrl, new StreamContent(requestStream));
                    response.EnsureSuccessStatusCode();
                    return await response.Content.ReadAsAsync<T>();
                }
            }
            catch (HttpRequestException exception)
            {
                throw new DataAccessException(baseAddress, request.Method, exception.Message);
            }
        }

        public T Get<T>(Request request, string regionName) where T : class
        {
            var baseAddress = string.Empty;

            try
            {
                using (var httpClient = this.GetHttpClient(regionName))
                {
                    baseAddress = httpClient.BaseAddress.ToString();
                    HttpResponseMessage response = httpClient.GetAsync(this.GetRequestUrl(request)).Result;
                    response.EnsureSuccessStatusCode();
                    return response.Content.ReadAsAsync<T>().Result;
                }
            }
            catch (HttpRequestException exception)
            {
                throw new DataAccessException(baseAddress, request.Method, exception.Message);
            }
        }

        public async Task<T> GetAsync<T>(Request request, string regionName) where T : class
        {
            var baseAddress = string.Empty;

            try
            {
                using (var httpClient = this.GetHttpClient(regionName))
                {
                    baseAddress = httpClient.BaseAddress.ToString();
                    HttpResponseMessage response = await httpClient.GetAsync(this.GetRequestUrl(request));
                    response.EnsureSuccessStatusCode();
                    return JsonHelper.DeserializeObject<T>(await response.Content.ReadAsStringAsync());
                }
            }
            catch (HttpRequestException exception)
            {
                throw new DataAccessException(baseAddress, request.Method, exception.Message);
            }
        }

        public async Task<T> GetAsync<T>(Request request, string regionName, string callsign) where T : class
        {
            var baseAddress = string.Empty;

            try
            {
                using (var httpClient = this.GetHttpClient(regionName))
                {
                    baseAddress = httpClient.BaseAddress.ToString();
                    string url = request.Method + "?callsign=" + callsign;

                    HttpResponseMessage response = await httpClient.GetAsync(url);
                    response.EnsureSuccessStatusCode();
                    return await response.Content.ReadAsAsync<T>();
                }
            }
            catch (HttpRequestException exception)
            {
                throw new DataAccessException(baseAddress, request.Method, exception.Message);
            }
        }



        public bool Delete(Request request, string regionName, string accessToken = null)
        {
            using (var httpClient = this.GetHttpClient(regionName, accessToken))
            {
                HttpResponseMessage response = httpClient.DeleteAsync(request.Method).Result;
                response.EnsureSuccessStatusCode();
                return true;
            }
        }

        private string GetRequestUrl(Request request)
        {
            string parameters = JsonHelper.SerializeObject(request);
            return request.Method;
        }

        private MemoryStream GetRequestStream(Request request)
        {
            var jsonString = JsonHelper.SerializeObject(request);
            byte[] byteArray = Encoding.UTF8.GetBytes(jsonString);

            return new MemoryStream(byteArray);
        }

        private HttpResponseMessage GetPostResponse(Request request, string regionName, string accessToken = null)
        {
            var baseAddress = string.Empty;
            string requestUrl = string.Empty;

            try
            {
                var requestStream = this.GetRequestStream(request);

                if (regionName != "paws")
                {
                    requestUrl = request.Method;
                }

                using (var httpClient = this.GetHttpClient(regionName, accessToken))
                {
                    baseAddress = httpClient.BaseAddress.ToString();
                    HttpResponseMessage response = httpClient.PostAsync(requestUrl, new StreamContent(requestStream)).Result;
                    response.EnsureSuccessStatusCode();
                    return response;
                }
            }
            catch (HttpRequestException exception)
            {
                throw new DataAccessException(baseAddress, request.Method, exception.Message);
            }
        }

        private HttpClient GetHttpClient(string regionName, string accessToken = null)
        {
            var httpClient = new HttpClient();
            httpClient.DefaultRequestHeaders.Accept.Clear();
            httpClient.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));

            string apiAddress = string.Empty;
            if (!string.IsNullOrEmpty(regionName))
            {
                if (regionName != Microsoft.WhiteSpaces.Common.Constants.Paws)
                {
                    Region region = CommonUtility.GetRegionByName(regionName);

                    if (region != null)
                    {
                        apiAddress = ConfigHelper.GetApiAddressByKey(region.Regulatory.ServiceApi);
                    }

                    if (!string.IsNullOrEmpty(apiAddress))
                    {
                        httpClient.BaseAddress = new Uri(apiAddress);
                    }
                    else
                    {
                        throw new Exception("Region not supported");
                    }
                }
                else
                {
                    httpClient.BaseAddress = new Uri(ConfigHelper.PawsApi);
                }
            }

            if (accessToken != null)
            {
                httpClient.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue(this.authorizeSchema, accessToken);
            }

            return httpClient;
        }
    }
}
