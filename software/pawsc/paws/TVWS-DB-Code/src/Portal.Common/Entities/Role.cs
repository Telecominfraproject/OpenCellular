// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    public class Role
    {
        public Role(string name, int id, string friendlyname, string[] applicableRegulatories)
        {
            this.Name = name;
            this.Id = id;
            this.FriendlyName = friendlyname;
            this.ApplicableRegulatories = applicableRegulatories;
        }

        public string Name { get; set; }

        public int Id { get; set; }

        public string FriendlyName { get; set; }

        public string[] ApplicableRegulatories { get; set; }
    }
}
