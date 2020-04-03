// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System;
    using System.ComponentModel.DataAnnotations;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;
    using Microsoft.Whitespace.Entities;

    public class IncumbentInfoBase
    {
        [ObjectValidator]        
        [MaxLength(100)]
        public virtual string Name { get; set; }

        public virtual string Description { get; set; }

        [ObjectValidator]
        [Required]
        public virtual Microsoft.Whitespace.Entities.Versitcard.VCard RegistrantInfo { get; set; }

        [ObjectValidator]
        [Required]
        public virtual Microsoft.Whitespace.Entities.Versitcard.VCard Contact { get; set; }

        public virtual DateTime StartDate { get; set; }

        public virtual DateTime EndDate { get; set; }

        [ObjectValidator]
        [Required]
        public virtual string CallSign { get; set; }
    }
}
