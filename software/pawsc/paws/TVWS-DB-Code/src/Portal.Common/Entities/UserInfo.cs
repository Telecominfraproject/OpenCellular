// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System;
    using System.Collections.Generic;

    public class UserInfo
    {
       public string UserId { get; set; }

       public string UserName { get; set; }

       public string FirstName { get; set; }

       public string LastName { get; set; }

       public string City { get; set; }

       public string PreferredEmail { get; set; }

       public string AccountEmail { get; set; }

       public string Address1 { get; set; }

       public string Address2 { get; set; }

       public string Phone { get; set; }

       public string Country { get; set; }

       public string ZipCode { get; set; }

       public string PhoneCountryCode { get; set; }

       public bool SuperAdmin { get; set; }

       public string RegisteredDate { get; set; }

       public Dictionary<string, string> RolesInfo { get; set; }
    }
}
