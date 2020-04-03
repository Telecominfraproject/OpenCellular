// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Linq;
    using System.Web;
    using Microsoft.WhiteSpaces.AzureOAuthProvider;
    using Microsoft.WhiteSpaces.AzureTableAccess;
    using Microsoft.WhiteSpaces.Common;    

    public class AccessElevationViewModel
    {
        public AccessElevationViewModel()
        {
            // Default value
            this.Roles = new Role[]
                {
                    new Role(string.Empty, -1, string.Empty, null)                    
                };

            this.Regulatories = Utility.GetAllAuthorities();
        }
        
        public string UserId { get; set; }

        [Required(ErrorMessage = "Required")]
        [Display(Name = "Regulatory *")]
        public string Regulatory { get; set; }

        [Required(ErrorMessage = "Required")]
        [Display(Name = "Access Role *")]
        public string AccessRole { get; set; }

        [Required(ErrorMessage = "Required")]
        [Display(Name = "Justification *")]

        public string Justification { get; set; }

        public Role[] Roles { get; set; }

        public List<Authority> Regulatories { get; private set; }
    }
}
