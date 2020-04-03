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
    /// Represents a TV Receive Site Registration.
    /// </summary>
    public class TVReceiveSiteRegistration : TableEntity
    {
        /// <summary>
        /// Initialization of VCARD for registrant
        /// </summary>
        private VCard registrant = new VCard();
        
        /// <summary>
        /// Initialization of VCARD for contact
        /// </summary>
        private VCard contact = new VCard();

        /// <summary>
        /// Gets or sets Location json string
        /// </summary>
        private Location receiveloc = new Location();

        /// <summary>
        /// Gets or sets Transmit Location json string
        /// </summary>
        private Location transmitLocation = new Location();
        
        /// <summary>
        /// Gets or sets Channel json string
        /// </summary>
        private TvSpectrum channel = new TvSpectrum();
        
        /// <summary>
        /// Gets or sets Receive Call sign json string
        /// </summary>
        private TvSpectrum receivingCallSign = new TvSpectrum();

        /// <summary>
        /// Gets or sets disposition json string
        /// </summary>
        public string TVReceiveSiteRegDisposition { get; set; }

        /// <summary>
        /// Gets or sets registrant json string
        /// </summary>
        public string TVReceiveSiteRegistrant { get; set; }

        /// <summary>
        /// Gets or sets contact json string
        /// </summary>
        public string TVReceiveSiteContact { get; set; }

        /// <summary>
        /// Gets or sets Receive Location json string
        /// </summary>
        public string TVReceiveSiteReceveLocation { get; set; }

        /// <summary>
        /// Gets or sets Transmit Channel json string
        /// </summary>
        public string TVReceiveSiteChannel { get; set; }

        /// <summary>
        /// Gets or sets Transmit Location json string
        /// </summary>
        public string TVReceiveSiteTransmitLocation { get; set; }

        /// <summary>
        /// Gets or sets Transmit Receive Call Sign json string
        /// </summary>
        public string TVReceiveSiteReceiveCallSign { get; set; }

        /// <summary>
        /// Gets or sets the Registration Disposition.
        /// </summary>
        public RegistrationDisposition Disposition { get; set; }

        /// <summary>
        /// Gets or sets the Registration Registrant.
        /// </summary>
        public VCard Registrant
        {
            get { return this.registrant; }
            set { this.registrant = value; }
        }

        /// <summary>
        /// Gets or sets the Registration Contact.
        /// </summary>
        public VCard Contact
        {
            get { return this.contact; }
            set { this.contact = value; }
        }

        /// <summary>
        /// Gets or sets the Registration TransmitLocation.
        /// </summary>
        public Location TransmitLocation
        {
            get { return this.transmitLocation; }
            set { this.transmitLocation = value; }
        }

        /// <summary>
        /// Gets or sets the Registration TransmitChannel.
        /// </summary>
        public TvSpectrum TransmitChannel
        {
            get { return this.channel; }
            set { this.channel = value; }
        }

        /// <summary>
        /// Gets or sets the Registration ReceiveLocation.
        /// </summary>
        public Location ReceiveLocation
        {
            get { return this.receiveloc; }
            set { this.receiveloc = value; }
        }

        /// <summary>
        /// Gets or sets the Registration ReceiveCallSign.
        /// </summary>
        public TvSpectrum ReceiveCallSign
        {
            get { return this.receivingCallSign; }
            set { this.receivingCallSign = value; }
        }

        /// <summary>
        /// Gets or sets the White Space DBA's name.
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
        /// Gets or sets the broadcast station contour.
        /// </summary>
        /// <value>The broadcast station contour.</value>
        public string BroadcastStationContour { get; set; }

        /// <summary>
        /// Complex type members is to be serialized to Json to store in Azure Table.
        /// </summary>
        public void SerializeObjectsToJston()
        {
            this.TVReceiveSiteRegDisposition = JsonSerialization.SerializeObject(this.Disposition).ToString();
            this.TVReceiveSiteRegistrant = JsonSerialization.SerializeObject(this.Registrant).ToString();
            this.TVReceiveSiteContact = JsonSerialization.SerializeObject(this.Contact).ToString();
            this.TVReceiveSiteTransmitLocation = JsonSerialization.SerializeObject(this.TransmitLocation).ToString();
            this.TVReceiveSiteReceveLocation = JsonSerialization.SerializeObject(this.ReceiveLocation).ToString();
            this.TVReceiveSiteChannel = JsonSerialization.SerializeObject(this.TransmitChannel).ToString();
            this.TVReceiveSiteReceiveCallSign = JsonSerialization.SerializeObject(this.ReceiveCallSign).ToString();

            if (this.ReceiveLocation != null)
            {
                this.Latitude = this.ReceiveLocation.Latitude;
                this.Longitude = this.ReceiveLocation.Longitude;
            }

            if (this.TransmitLocation != null)
            {
                this.TxLatitude = this.TransmitLocation.Latitude;
                this.TxLongitude = this.TransmitLocation.Longitude;
            }

            if (this.ReceiveLocation != null && this.TVReceiveSiteChannel != string.Empty)
            {
                this.ChannelNumber = Convert.ToInt16(this.TransmitChannel.Channel);
            }
        }

        /// <summary>
        /// Complex type members is to be de-serialized to object from json.
        /// </summary>
        public void DeSerializeObjectsFromJson()
        {
            this.Disposition = JsonSerialization.DeserializeString<RegistrationDisposition>(this.TVReceiveSiteRegDisposition);
            this.Registrant = JsonSerialization.DeserializeString<VCard>(this.TVReceiveSiteRegistrant);
            this.Contact = JsonSerialization.DeserializeString<VCard>(this.TVReceiveSiteContact);
            this.TransmitLocation = JsonSerialization.DeserializeString<Location>(this.TVReceiveSiteTransmitLocation);
            this.ReceiveLocation = JsonSerialization.DeserializeString<Location>(this.TVReceiveSiteReceveLocation);
            this.TransmitChannel = JsonSerialization.DeserializeString<TvSpectrum>(this.TVReceiveSiteChannel);
            this.ReceiveCallSign = JsonSerialization.DeserializeString<TvSpectrum>(this.TVReceiveSiteReceiveCallSign);
        }
    }
}
