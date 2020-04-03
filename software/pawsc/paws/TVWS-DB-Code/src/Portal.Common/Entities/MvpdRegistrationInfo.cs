// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System.ComponentModel.DataAnnotations;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;
    using Microsoft.Whitespace.Entities;

    public class MvpdRegistrationInfo : IncumbentInfoBase
    {
        public MvpdRegistrationInfo()
        {
            this.IncumbentType = Microsoft.Whitespace.Entities.IncumbentType.MVPD.ToString();
        }

        [ObjectValidator]
        [Required]
        public Location RecieveLocation { get; set; }

        [ObjectValidator]
        [Required]
        public Location TransmitLocation { get; set; }
       
        [ObjectValidator]
        [Required]
        public int Channel { get; set; }

        public string IncumbentType { get; private set; }
    }
}
