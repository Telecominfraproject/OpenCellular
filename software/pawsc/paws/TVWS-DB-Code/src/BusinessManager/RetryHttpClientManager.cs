// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.BusinessManager
{
    using System;
    using System.Threading.Tasks;
    using Microsoft.Practices.EnterpriseLibrary.TransientFaultHandling;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.DataAccessManager;

    public class RetryHttpClientManager : IHttpClientManager
    {
        private readonly IHttpClientManager httpClientManager;
        private readonly RetryPolicy retryPolicy;

        public RetryHttpClientManager(IHttpClientManager httpClientManager, RetryPolicy retryPolicy)
        {
            if (httpClientManager == null)
            {
                throw new ArgumentNullException("httpClientManager");
            }

            if (retryPolicy == null)
            {
                throw new ArgumentNullException("retryPolicy");
            }

            this.httpClientManager = httpClientManager;
            this.retryPolicy = retryPolicy;
        }

        public string Post(Request request, string regionName, string accessToken = null)
        {
            return this.retryPolicy.ExecuteAction<string>(() =>
                {
                    return this.httpClientManager.Post(request, regionName, accessToken);
                });
        }

        public T Post<T>(Request request, string regionName, string accessToken = null) where T : class
        {
            return this.retryPolicy.ExecuteAction<T>(() =>
            {
                return this.httpClientManager.Post<T>(request, regionName, accessToken);
            });
        }

        public Task<T> PostAsync<T>(Request request, string regionName, string accessToken = null) where T : class
        {
            return this.retryPolicy.ExecuteAsync<T>(() =>
                {
                    return this.httpClientManager.PostAsync<T>(request, regionName, accessToken);
                });
        }

        public T Get<T>(Request request, string regionName) where T : class
        {
            return this.retryPolicy.ExecuteAction<T>(() =>
                {
                    return this.httpClientManager.Get<T>(request, regionName);
                });
        }

        public Task<T> GetAsync<T>(Request request, string regionName) where T : class
        {
            return this.retryPolicy.ExecuteAsync<T>(() =>
            {
                return this.httpClientManager.GetAsync<T>(request, regionName);
            });
        }

        public bool Delete(Request request, string regionName, string accessToken = null)
        {
            return this.retryPolicy.ExecuteAction<bool>(() =>
                {
                    return this.httpClientManager.Delete(request, regionName, accessToken);
                });
        }


        public Task<T> GetAsync<T>(Request request, string regionName, string callsign) where T : class
        {
            return this.retryPolicy.ExecuteAsync<T>(() =>
            {
                return this.httpClientManager.GetAsync<T>(request, regionName, callsign);
            });
        }
    }
}
