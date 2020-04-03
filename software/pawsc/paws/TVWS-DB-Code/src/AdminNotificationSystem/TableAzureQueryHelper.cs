// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AdminNotificationSystem
{
    using Microsoft.WindowsAzure.Storage.Table;

    public class TableAzureQueryHelper
    {
        public static TableQuery<DynamicTableEntity> AddCondition(TableQuery<DynamicTableEntity> query, string tableOperator, string condition)
        {
            if (query.FilterString == null)
            {
                query = query.Where(condition);
            }
            else
            {
                query = query.Where(TableQuery.CombineFilters(query.FilterString, tableOperator, condition));
            }

            return query;
        }
    }
}
