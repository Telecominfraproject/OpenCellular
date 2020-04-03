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
    /// Represents a Temp BAS Registration.
    /// </summary>
    public class TempBASRegistration : TableEntity
    {
        /// <summary>
        /// Initialization of Event
        /// </summary>
        private Event tempBasEvent = new Event();

        /// <summary>
        /// Initialization of VCARD for Registrant
        /// </summary>
        private VCard registrant = new VCard();

        /// <summary>
        /// Initialization of VCard for contact.
        /// </summary>
        private VCard contact = new VCard();

        /// <summary>
        /// Initialization of Location
        /// </summary>
        private Location loc = new Location();

        /// <summary>
        /// Initialization of Transmit Location
        /// </summary>
        private Location transmitLocation = new Location();

        /// <summary>
        /// Initialization of spectrum
        /// </summary>
        private TvSpectrum channel = new TvSpectrum();

        /// <summary>
        /// Gets or sets disposition json string
        /// </summary>
        public string TempBasRegDisposition { get; set; }

        /// <summary>
        /// Gets or sets registrant json string
        /// </summary>
        public string TempBasRegistrant { get; set; }

        /// <summary>
        /// Gets or sets contact json string
        /// </summary>
        public string TempBasContact { get; set; }

        /// <summary>
        /// Gets or sets TempBas Location json string
        /// </summary>
        public string TempBasLocation { get; set; }

        /// <summary>
        /// Gets or sets TempBas Channel json string
        /// </summary>
        public string TempBasChannel { get; set; }

        /// <summary>
        /// Gets or sets TempBas Transmit Location json string
        /// </summary>
        public string TempBasTransmitLocation { get; set; }

        /// <summary>
        /// Gets or sets Temp Bas Registration Event json string
        /// </summary>
        public string TempBasEvent { get; set; }

        /// <summary>
        /// Gets or sets the registration disposition.
        /// </summary>
        public RegistrationDisposition Disposition { get; set; }

        /// <summary>
        /// Gets or sets the registration Registrant.
        /// </summary>
        public VCard Registrant
        {
            get { return this.registrant; }
            set { this.registrant = value; }
        }

        /// <summary>
        /// Gets or sets the registration Contact.
        /// </summary>
        public VCard Contact
        {
            get { return this.contact; }
            set { this.contact = value; }
        }

        /// <summary>
        /// Gets or sets the Location.
        /// </summary>
        public Location RecvLocation
        {
            get { return this.loc; }
            set { this.loc = value; }
        }

        /// <summary>
        /// Gets or sets the Channel.
        /// </summary>
        public TvSpectrum Channel
        {
            get { return this.channel; }
            set { this.channel = value; }
        }

        /// <summary>
        /// Gets or sets the TransmitLocation.
        /// </summary>
        public Location TransmitLocation
        {
            get { return this.transmitLocation; }
            set { this.transmitLocation = value; }
        }

        /// <summary>
        /// Gets or sets the registration Event.
        /// </summary>
        public Event Event { get; set; }

        /// <summary>
        /// Gets or sets the TransmitLocation.
        /// </summary>
        public string WSDBA { get; set; }

        /// <summary>
        /// Gets or sets Location Latitude
        /// </summary>
        public double Latitude { get; set; }

        /// <summary>
        /// Gets or sets Location Longitude 
        /// </summary>
        public double Longitude { get; set; }

        /// <summary>
        /// Gets or sets Location Longitude 
        /// </summary>
        public int ChannelNumber { get; set; }

        /// <summary>
        /// Gets or sets Transmit Location Latitude
        /// </summary>
        public double TxLatitude { get; set; }

        /// <summary>
        /// Gets or sets Transmit Location Longitude 
        /// </summary>
        public double TxLongitude { get; set; }

        /// <summary>
        /// Gets or sets the UserId.
        /// </summary>
        public string UserId { get; set; }

        /// <summary>
        /// Complex type members is to be serialized to Json to store in Azure Table.
        /// </summary>
        public void SerializeObjectsToJston()
        {
            this.TempBasRegDisposition = JsonSerialization.SerializeObject(this.Disposition).ToString();
            this.TempBasRegistrant = JsonSerialization.SerializeObject(this.Registrant).ToString();
            this.TempBasContact = JsonSerialization.SerializeObject(this.Contact).ToString();
            this.TempBasLocation = JsonSerialization.SerializeObject(this.RecvLocation).ToString();
            this.TempBasChannel = JsonSerialization.SerializeObject(this.Channel).ToString();
            this.TempBasTransmitLocation = JsonSerialization.SerializeObject(this.TransmitLocation).ToString();
            this.TempBasEvent = JsonSerialization.SerializeObject(this.Event).ToString();

            if (this.RecvLocation != null)
            {
                this.Latitude = this.RecvLocation.Latitude;
                this.Longitude = this.RecvLocation.Longitude;
            }

            if (this.TransmitLocation != null)
            {
                this.TxLatitude = this.TransmitLocation.Latitude;
                this.TxLongitude = this.TransmitLocation.Longitude;
            }

            if (this.TempBasChannel != null && this.TempBasChannel != string.Empty)
            {
                this.ChannelNumber = Convert.ToInt16(this.Channel.Channel);
            }
        }

        /// <summary>
        /// Complex type members is to be de-serialized to object from json.
        /// </summary>
        public void DeSerializeObjectsFromJson()
        {
            this.Disposition = JsonSerialization.DeserializeString<RegistrationDisposition>(this.TempBasRegDisposition);
            this.Registrant = JsonSerialization.DeserializeString<VCard>(this.TempBasRegistrant);
            this.Contact = JsonSerialization.DeserializeString<VCard>(this.TempBasContact);
            this.RecvLocation = JsonSerialization.DeserializeString<Location>(this.TempBasLocation);
            this.Channel = JsonSerialization.DeserializeString<TvSpectrum>(this.TempBasChannel);
            this.TransmitLocation = JsonSerialization.DeserializeString<Location>(this.TempBasTransmitLocation);
            this.Event = JsonSerialization.DeserializeString<Event>(this.TempBasEvent);
        }

        /// <summary>
        /// Complex type members is to be de-serialized to object from json and clear the properties.
        /// </summary>
        public void DeSerializeAndClearObjectsFromJson()
        {
            this.Disposition = JsonSerialization.DeserializeString<RegistrationDisposition>(this.TempBasRegDisposition);
            this.Registrant = JsonSerialization.DeserializeString<VCard>(this.TempBasRegistrant);
            this.Contact = JsonSerialization.DeserializeString<VCard>(this.TempBasContact);
            this.RecvLocation = JsonSerialization.DeserializeString<Location>(this.TempBasLocation);
            this.Channel = JsonSerialization.DeserializeString<TvSpectrum>(this.TempBasChannel);
            this.TransmitLocation = JsonSerialization.DeserializeString<Location>(this.TempBasTransmitLocation);
            this.Event = JsonSerialization.DeserializeString<Event>(this.TempBasEvent);

            this.TempBasRegDisposition = string.Empty;
            this.TempBasRegistrant = string.Empty;
            this.TempBasContact = string.Empty;
            this.TempBasLocation = string.Empty;
            this.TempBasChannel = string.Empty;
            this.TempBasTransmitLocation = string.Empty;
            this.TempBasEvent = string.Empty;
        }
    }
}
