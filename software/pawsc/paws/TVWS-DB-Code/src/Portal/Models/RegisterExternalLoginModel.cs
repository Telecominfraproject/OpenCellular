// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System.ComponentModel;
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// view model of user registration
    /// </summary>
    public class RegisterExternalLoginModel
    {
        /// <summary>
        /// Gets or sets user name
        /// </summary>        
        public string UserName { get; set; }

        /// <summary>
        /// Gets or sets first name
        /// </summary>
        [Required(ErrorMessage = "Required")]
        [StringLength(50)]
        [DisplayName("First Name *")]
        public string FirstName { get; set; }

        /// <summary>
        /// Gets or sets last name
        /// </summary>
        [StringLength(50)]
        [DisplayName("Last Name")]
        public string LastName { get; set; }
       
        [DisplayName("TimeZone")]
        public string TimeZone { get; set; }        

        /// <summary>
        /// Gets or sets email
        /// </summary>
        [Required(ErrorMessage = "Required.")]
        [DataType(DataType.EmailAddress)]
        [EmailAddress(ErrorMessage = "Invalid Format.")]
        [StringLength(100)]
        [DisplayName("Preferred Email *")]
        public string PreferredEmail { get; set; }

        [DisplayName("Account Email ")]
        public string AccountEmail { get; set; }

        /// <summary>
        /// Gets or sets Address1
        /// </summary>
        [Required(ErrorMessage = "Required")]
        [DisplayName("Address Line 1 *")]
        public string Address1 { get; set; }

        /// <summary>
        /// Gets or sets Address2
        /// </summary>
        [DisplayName("Address Line 2")]
        public string Address2 { get; set; }

        /// <summary>
        /// Gets or sets City
        /// </summary>
        [Required(ErrorMessage = "Required.")]
        [StringLength(60)]
        [DisplayName("City *")]
        public string City { get; set; }

        /// <summary>
        /// Gets or sets State
        /// </summary>
        [Required(ErrorMessage = "Required.")]
        [StringLength(60)]
        [DisplayName("State/Province/Region *")]
        public string State { get; set; }

        /// <summary>
        /// Gets or sets State
        /// </summary>
        [Required(ErrorMessage = "*Required.")]
        [StringLength(60)]
        [DisplayName("Zip/Postal Code *")]
        public string ZipCode { get; set; }      

        /// <summary>
        /// Gets or sets country
        /// </summary>
        [DisplayName("Country")]
        public string Country { get; set; }

        /// <summary>
        /// Gets or sets Country Code
        /// </summary>
        [DisplayName("Country Code")]
        public string PhoneCountryCode { get; set; }

        /// <summary>
        /// Gets or sets Phone
        /// </summary>
        [DisplayName("Phone Number")]
        public string Phone { get; set; }

        /// <summary>
        /// Gets or sets External Login Data
        /// </summary>
        public string ExternalLoginData { get; set; }
    }
}
