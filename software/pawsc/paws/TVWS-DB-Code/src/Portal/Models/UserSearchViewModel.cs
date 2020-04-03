// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System.Collections.Generic;
    using System.Web.Mvc;
    using Microsoft.WhiteSpaces.Common;

    public class UserSearchViewModel
    {
        public bool AllSelected { get; set; }

        public bool ApprovedSelected { get; set; }

        public bool RejectedSelected { get; set; }

        public char[] Alphabets { get; set; }

        public string SelectedCity { get; set; }

        public string SelectedCountry { get; set; }

        public string[] SelectedAlphabets { get; set; }

        public List<SelectListItem> RoleList { get; set; }

        public List<SelectListItem> CountryList { get; set; }

        public IEnumerable<IntialUserInfo> UserList { get; set; }

        public int Page { get; set; }
    }
}
