// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System.Collections.Generic;

    public class SearchCriteria
    {
        public int RequestStatus { get; set; }

        public string[] NameStartsWith { get; set; }

        public string SelectedRole { get; set; }

        public string SelectedCountry { get; set; }      
    }
}
