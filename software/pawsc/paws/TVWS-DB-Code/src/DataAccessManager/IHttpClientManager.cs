// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.DataAccessManager
{
    using System;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Entities;

    public interface IHttpClientManager
    {
        string Post(Request request, string regionName, string accessToken = null);

        T Post<T>(Request request, string regionName, string accessToken = null) where T : class;

        Task<T> PostAsync<T>(Request request, string regionName, string accessToken = null) where T : class;

        T Get<T>(Request request, string regionName) where T : class;

        Task<T> GetAsync<T>(Request request, string regionName, string callsign) where T : class;

        Task<T> GetAsync<T>(Request request, string regionName) where T : class;

        bool Delete(Request request, string regionName, string accessToken = null);             
    }
}
