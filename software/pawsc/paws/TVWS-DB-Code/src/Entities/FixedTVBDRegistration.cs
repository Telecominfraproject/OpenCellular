// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Entities.Versitcard;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents Fixed TVBD Registration.
    /// </summary>
    public class FixedTVBDRegistration : TableEntity
    {
        /// <summary>
        /// Holds Fixed TVBD Location
        /// </summary>
        private Location tvbdLoc = new Location();

        /// <summary>
        /// Holds Fixed TVBD Location
        /// </summary>
        private DeviceOwner tvbdDeviceOwner = new DeviceOwner();

        /// <summary>
        /// Initialization of VCARD for Registrant
        /// </summary>
        private VCard registrant = new VCard();

        /// <summary>
        /// Initialization of VCARD for Contact
        /// </summary>
        private VCard contact = new VCard();

        /// <summary>
        /// Gets or sets Fixed TVBD Contact
        /// </summary>
        public DeviceOwner TVBDDeviceOwner
        {
            get { return this.tvbdDeviceOwner; }
            set { this.tvbdDeviceOwner = value; }
        }

        /// <summary>
        /// Gets or sets Fixed TVBD Contact
        /// </summary>
        public string TVBDContact { get; set; }

        /// <summary>
        /// Gets or sets Fixed TVBD Registrant
        /// </summary>
        public string TVBDRegistrant { get; set; }

        /// <summary>
        /// Gets or sets the Disposition.
        /// </summary>
        public RegistrationDisposition Disposition { get; set; }

        /// <summary>
        /// Gets or sets the Registrant.
        /// </summary>
        public VCard Registrant
        {
            get { return this.registrant; }
            set { this.registrant = value; }
        }

        /// <summary>
        /// Gets or sets the Contact.
        /// </summary>
        public VCard Contact
        {
            get { return this.contact; }
            set { this.contact = value; }
        }

        /// <summary>
        /// Gets or sets the Location.
        /// </summary>
        public Location Loc
        {
            get { return this.tvbdLoc; }
            set { this.tvbdLoc = value; }
        }

        /// <summary>
        /// Gets or sets the DeviceId.
        /// </summary>
        public DeviceId DeviceId { get; set; }

        /// <summary>
        /// Gets or sets the RegistrationDispositionJson.
        /// </summary>
        public string RegistrationDisposition { get; set; }

        /// <summary>
        /// Gets or sets the DeviceOwnerJson.
        /// </summary>
        public string DeviceOwner { get; set; }

        /// <summary>
        /// Gets or sets the LocationJson.
        /// </summary>
        public string Location { get; set; }

        /// <summary>
        /// Gets or sets the DeviceDescriptorJson.
        /// </summary>
        public string DeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the AntennaJson.
        /// </summary>
        public string Antenna { get; set; }

        /// <summary>
        /// Gets or sets the WSDBA.
        /// </summary>
        public string WSDBA { get; set; }

        /// <summary>
        /// Gets or sets the SerialNumber.
        /// </summary>
        public string SerialNumber { get; set; }

        /// <summary>
        /// Gets or sets the FCCId.
        /// </summary>
        public string FCCId { get; set; }

        /// <summary>
        /// Gets or sets the UniqueDeviceIdentifier.
        /// </summary>
        public string UniqueDeviceId { get; set; }

        /// <summary>
        /// Complex type members is to be serialized to Json to store in Azure Table.
        /// </summary>
        public void SerializeObjectsToJston()
        {
            this.RegistrationDisposition = JsonSerialization.SerializeObject(this.Disposition).ToString();
            this.DeviceDescriptor = JsonSerialization.SerializeObject(this.DeviceId).ToString();
            this.TVBDContact = JsonSerialization.SerializeObject(this.Contact).ToString();
            this.TVBDRegistrant = JsonSerialization.SerializeObject(this.Registrant).ToString();
            this.Location = JsonSerialization.SerializeObject(this.Loc).ToString();
            this.Antenna = JsonSerialization.SerializeObject(this.Antenna).ToString();
            this.DeviceOwner = this.TVBDContact;
        }

        /// <summary>
        /// Complex type members is to be de-serialized from Json.
        /// </summary>
        public void DeSerializeObjectsFromJson()
        {
            this.Disposition = JsonSerialization.DeserializeString<RegistrationDisposition>(this.RegistrationDisposition);
            this.DeviceId = JsonSerialization.DeserializeString<DeviceId>(this.DeviceDescriptor);
            if (this.DeviceOwner != null && this.DeviceOwner != string.Empty)
            {
                this.TVBDDeviceOwner = JsonSerialization.DeserializeString<DeviceOwner>(this.DeviceOwner);
                if (this.TVBDDeviceOwner != null && this.TVBDDeviceOwner.Owner != null)
                {
                    Versitcard.Organization org = new Versitcard.Organization();
                    org.OrganizationName = this.TVBDDeviceOwner.Owner.Organization.Text;
                    this.Registrant.Org = org;
                }

                if (this.TVBDDeviceOwner != null && this.TVBDDeviceOwner.Owner != null)
                {
                    Versitcard.Address add = new Versitcard.Address();
                    add.Code = this.TVBDDeviceOwner.Owner.Address.Code.ToString();
                    add.Country = this.TVBDDeviceOwner.Owner.Address.Country.ToString();
                    add.Locality = this.TVBDDeviceOwner.Owner.Address.Locality.ToString();
                    add.Region = this.TVBDDeviceOwner.Owner.Address.Region.ToString();
                    add.Street = this.TVBDDeviceOwner.Owner.Address.Street.ToString();
                    this.Contact.Address = add;
                }

                if (this.TVBDDeviceOwner != null)
                {
                    Versitcard.Email[] email = new Versitcard.Email[1];
                    email[0] = new Versitcard.Email();
                    email[0].EmailAddress = this.TVBDDeviceOwner.Owner.Email.Text;
                    this.contact.Email = email;
                }

                if (this.TVBDDeviceOwner != null)
                {
                    Versitcard.Telephone[] tel = new Versitcard.Telephone[1];
                    tel[0] = new Versitcard.Telephone();
                    tel[0].TelephoneNumber = this.tvbdDeviceOwner.Owner.Phone.Uri;
                    this.contact.Telephone = tel;
                }
            }
            else if (this.TVBDContact != null && this.TVBDContact != string.Empty)
            {
                this.Contact = JsonSerialization.DeserializeString<VCard>(this.TVBDContact);
            }

            if (this.TVBDRegistrant != null && this.TVBDRegistrant != string.Empty)
            {
                this.Registrant = JsonSerialization.DeserializeString<VCard>(this.TVBDRegistrant);
            }

            this.Loc = JsonSerialization.DeserializeString<Location>(this.Location);
        }
    }
}
