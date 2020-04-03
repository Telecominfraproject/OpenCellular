// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Xml.Serialization;
    using Microsoft.Whitespace.Entities.Versitcard;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents a MVPD Registration.
    /// </summary>
    public class MVPDRegistration : TableEntity
    {
        /// <summary>
        ///  Initialization of VCard for registrant
        /// </summary>
        private VCard registrant = new VCard();

        /// <summary>
        /// Initialization of VCard for contact
        /// </summary>
        private VCard contact = new VCard();

        /// <summary>
        /// Initialization of Location
        /// </summary>
        private Location loc = new Location();

        /// <summary>
        ///  Initialization of Transmit Location
        /// </summary>
        private Location transmitLocation = new Location();

        /// <summary>
        /// Initialization of Spectrum
        /// </summary>
        private TvSpectrum channel = new TvSpectrum();

        /// <summary>
        /// Gets or sets disposition json string
        /// </summary>
        public string MVPDRegDisposition { get; set; }

        /// <summary>
        /// Gets or sets registrant json string
        /// </summary>
        public string MVPDRegistrant { get; set; }

        /// <summary>
        /// Gets or sets contact json string
        /// </summary>
        public string MVPDContact { get; set; }

        /// <summary>
        /// Gets or sets MVPD Location json string
        /// </summary>
        public string MVPDLocation { get; set; }

        /// <summary>
        /// Gets or sets MVPD Channel json string
        /// </summary>
        public string MVPDChannel { get; set; }

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
        /// Gets or sets MVPD Transmit Location json string
        /// </summary>
        public string MVPDTransmitLocation { get; set; }

        /// <summary>
        /// Gets or sets the Disposition.
        /// </summary>
        [XmlElement]
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
        public Location Location 
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
        /// Gets or sets the TransmitLocation.
        /// </summary>
        public string WSDBA { get; set; }

        /// <summary>
        /// Gets or sets the UserId.
        /// </summary>
        public string UserId { get; set; }

        /// <summary>
        /// Complex type members is to be serialized to Json to store in Azure Table.
        /// </summary>
        public void SerializeObjectsToJston()
        {
            this.MVPDRegDisposition = JsonSerialization.SerializeObject(this.Disposition).ToString();
            this.MVPDRegistrant = JsonSerialization.SerializeObject(this.Registrant).ToString();
            this.MVPDContact = JsonSerialization.SerializeObject(this.Contact).ToString();
            this.MVPDLocation = JsonSerialization.SerializeObject(this.Location).ToString();
            this.MVPDChannel = JsonSerialization.SerializeObject(this.Channel).ToString();
            this.MVPDTransmitLocation = JsonSerialization.SerializeObject(this.TransmitLocation).ToString();
            
            if (this.MVPDLocation != null)
            {
                this.Latitude = this.Location.Latitude;
                this.Longitude = this.Location.Longitude;
            }
            
            if (this.MVPDChannel != null && this.MVPDChannel != string.Empty)
            {
                this.ChannelNumber = Convert.ToInt16(this.Channel.Channel);
            }

            if (this.MVPDTransmitLocation != null)
            {
                this.TxLatitude = this.TransmitLocation.Latitude;
                this.TxLongitude = this.transmitLocation.Longitude;
            }
        }

        /// <summary>
        /// Complex type members is to be de-serialized to object from json.
        /// </summary>
        public void DeSerializeObjectsFromJson()
        {
            this.Disposition = JsonSerialization.DeserializeString<RegistrationDisposition>(this.MVPDRegDisposition);
            this.Registrant = JsonSerialization.DeserializeString<VCard>(this.MVPDRegistrant);
            this.Contact = JsonSerialization.DeserializeString<VCard>(this.MVPDContact);
            this.Location = JsonSerialization.DeserializeString<Location>(this.MVPDLocation);
            this.Channel = JsonSerialization.DeserializeString<TvSpectrum>(this.MVPDChannel);
            this.TransmitLocation = JsonSerialization.DeserializeString<Location>(this.MVPDTransmitLocation);
        }

        /// <summary>
        /// Complex type members is to be de-serialized to object from json and clear the properties.
        /// </summary>
        public void DeSerializeAndClearObjectsFromJson()
        {
            this.Disposition = JsonSerialization.DeserializeString<RegistrationDisposition>(this.MVPDRegDisposition);
            this.Registrant = JsonSerialization.DeserializeString<VCard>(this.MVPDRegistrant);
            this.Contact = JsonSerialization.DeserializeString<VCard>(this.MVPDContact);
            this.Location = JsonSerialization.DeserializeString<Location>(this.MVPDLocation);
            this.Channel = JsonSerialization.DeserializeString<TvSpectrum>(this.MVPDChannel);
            this.TransmitLocation = JsonSerialization.DeserializeString<Location>(this.MVPDTransmitLocation);
            this.MVPDRegDisposition = string.Empty;
            this.MVPDRegistrant = string.Empty;
            this.MVPDContact = string.Empty;
            this.MVPDLocation = string.Empty;
            this.MVPDChannel = string.Empty;
            this.MVPDTransmitLocation = string.Empty;
        }
    }
}
