// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.Whitespace.Entities.Versitcard;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents a LP Aux Registration.
    /// </summary>
    [XmlRoot("LP-Aux_Registration", Namespace = Constants.RegistrationRecordEnsembleXmlns)]
    public class LPAuxRegistration : TableEntity
    {
        /// <summary>
        /// Represents licensed value.
        /// </summary>
        private bool licensed;

        /// <summary>
        /// Represents white space Data Base Administrator
        /// </summary>
        private string wsdba;

        /// <summary>
        /// Represents white space User Id
        /// </summary>
        private string userId;

        /// <summary>
        /// Represents Registration Disposition object.
        /// </summary>
        private RegistrationDisposition regDisposition;

        /// <summary>
        /// Represents Registrant object
        /// </summary>
        [XmlElement("lpauxRegistrant")]
        private VCard registrant = new VCard();

        /// <summary>
        /// Represents contact object
        /// </summary>
        [XmlElement("lpauxContact")]
        private VCard contact = new VCard();

        /// <summary>
        /// Represents venue name
        /// </summary>
        private string venueName;

        /// <summary>
        /// Represents latitude
        /// </summary>
        private double latitude;

        /// <summary>
        /// Represents longitude
        /// </summary>
        private double longitude;

        /// <summary>
        /// Represents registration Id
        /// </summary>
        private string regId;

        /// <summary>
        /// Represents positions of the registrations
        /// </summary>
        private Position[] pointsArea;

        /// <summary>
        /// Represents Quadrilateral Area positions of the registrations
        /// </summary>
        private QuadrilateralArea[] qpointsArea;

        /// <summary>
        /// Represents call sign object
        /// </summary>
        private TvSpectrum callSign = new TvSpectrum();

        /// <summary>
        /// Represents event object
        /// </summary>
        private Event auxEvent = new Event();

        /// <summary>
        /// Gets or sets disposition json string
        /// </summary>
        public string AuxRegDisposition { get; set; }

        /// <summary>
        /// Gets or sets registrant json string
        /// </summary>
        public string AuxRegistrant { get; set; }

        /// <summary>
        /// Gets or sets contact json string
        /// </summary>
        public string AuxContact { get; set; }

        /// <summary>
        /// Gets or sets Points Area json string
        /// </summary>
        public string AuxPointsArea { get; set; }

        /// <summary>
        /// Gets or sets Quadrilateral Area json string
        /// </summary>
        public string AuxQuadPoints { get; set; }

        /// <summary>
        /// Gets or sets spectrum Area json string
        /// </summary>
        public string AuxTvSpectrum { get; set; }

        /// <summary>
        /// Gets or sets Event json string
        /// </summary>
        public string AuxEvent { get; set; }

        /// <summary>
        /// Gets or sets the Disposition.
        /// </summary>
        [XmlElement]
        public RegistrationDisposition Disposition 
        {
            get { return this.regDisposition; }
            set { this.regDisposition = value; }
        }

        /// <summary>
        /// Gets or sets the Disposition.
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
        /// Gets or sets the VenueName.
        /// </summary>
        public string VenueName 
        {
            get { return this.venueName; }
            set { this.venueName = value; }
        }

        /// <summary>
        /// Gets or sets the PointArea.
        /// </summary>
        public Position[] PointsArea 
        {
            get { return this.pointsArea; }
            set { this.pointsArea = value; }
        }

        /// <summary>
        /// Gets or sets the QuadrilateralArea.
        /// </summary>
        public QuadrilateralArea[] QuadrilateralArea 
        {
            get { return this.qpointsArea; }
            set { this.qpointsArea = value; }
        }

        /// <summary>
        /// Gets or sets the CallSign.
        /// </summary>
        public TvSpectrum CallSign 
        {
            get { return this.callSign; }
            set { this.callSign = value; }
        }

        /// <summary>
        /// Gets or sets the Event.
        /// </summary>
        public Event Event 
        {
            get { return this.auxEvent; }
            set { this.auxEvent = value; }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the registration is licensed.
        /// </summary>
        public bool Licensed 
        {
            get { return this.licensed; }
            set { this.licensed = value; }
        }

        /// <summary>
        /// Gets or sets a value OF Originating WSDBA
        /// </summary>
        public string WSDBA
        {
            get { return this.wsdba; }
            set { this.wsdba = value; }
        }

        /// <summary>
        /// Gets or sets the UserId.
        /// </summary>
        public string UserId
        {
            get { return this.userId; }
            set { this.userId = value; }
        }

        /// <summary>
        /// Gets or sets the Latitude.
        /// </summary>
        public double Latitude
        {
            get { return this.latitude; }
            set { this.latitude = value; }
        }

        /// <summary>
        /// Gets or sets the Longitude.
        /// </summary>
        public double Longitude
        {
            get { return this.longitude; }
            set { this.longitude = value; }
        }

        /// <summary>
        /// Gets or sets the registration Id.
        /// </summary>
        public string RegId
        {
            get { return this.regId; }
            set { this.regId = value; }
        }

        /// <summary>
        /// Gets or sets the ULS file number.
        /// </summary>
        /// <value>The ULS file number.</value>
        public string ULSFileNumber { get; set; }

        /// <summary>
        /// Gets or sets the channel.
        /// </summary>
        /// <value>The channel.</value>
        public int Channel { get; set; }

        /// <summary>
        /// Complex type members is to be serialized to Json to store in Azure Table.
        /// </summary>
        public void SerializeObjectsToJston()
        {
            this.AuxRegDisposition = JsonSerialization.SerializeObject(this.Disposition).ToString();
            this.AuxRegistrant  = JsonSerialization.SerializeObject(this.registrant).ToString();
            this.AuxContact = JsonSerialization.SerializeObject(this.contact).ToString();
            this.AuxPointsArea = JsonSerialization.SerializeObject(this.pointsArea).ToString();
            this.AuxQuadPoints = JsonSerialization.SerializeObject(this.QuadrilateralArea).ToString();
            this.AuxTvSpectrum = JsonSerialization.SerializeObject(this.callSign).ToString();
            this.AuxEvent = JsonSerialization.SerializeObject(this.auxEvent).ToString();
            this.Channel = this.callSign.Channel.Value;
        }

        /// <summary>
        /// Complex type members is to be de-serialized to object from json.
        /// </summary>
        public void DeSerializeObjectsFromJson()
        {
            this.Disposition = JsonSerialization.DeserializeString<RegistrationDisposition>(this.AuxRegDisposition);
            this.registrant = JsonSerialization.DeserializeString<VCard>(this.AuxRegistrant);
            this.contact = JsonSerialization.DeserializeString<VCard>(this.AuxContact);
            if (this.AuxPointsArea != null)
            {
                this.pointsArea = JsonSerialization.DeserializeString<Position[]>(this.AuxPointsArea);
            }

            if (this.AuxQuadPoints != null)
            {
                this.QuadrilateralArea = JsonSerialization.DeserializeString<QuadrilateralArea[]>(this.AuxQuadPoints);
            }

            this.callSign = JsonSerialization.DeserializeString<TvSpectrum>(this.AuxTvSpectrum);
            this.auxEvent = JsonSerialization.DeserializeString<Event>(this.AuxEvent);
            this.Channel = this.callSign.Channel.Value;
        }

        /// <summary>
        /// Complex type members is to be de-serialized to object from json and clear the properties.
        /// </summary>
        public void DeSerializeAndClearObjectsFromJson()
        {
            if (!string.IsNullOrEmpty(this.AuxRegDisposition))
            {
                this.Disposition = JsonSerialization.DeserializeString<RegistrationDisposition>(this.AuxRegDisposition);
            }

            if (!string.IsNullOrEmpty(this.AuxRegistrant))
            {
                this.registrant = JsonSerialization.DeserializeString<VCard>(this.AuxRegistrant);
            }

            if (!string.IsNullOrEmpty(this.AuxContact))
            {
                this.contact = JsonSerialization.DeserializeString<VCard>(this.AuxContact);
            }

            if (this.AuxPointsArea != null)
            {
                this.pointsArea = JsonSerialization.DeserializeString<Position[]>(this.AuxPointsArea);
            }

            if (this.AuxQuadPoints != null)
            {
                this.QuadrilateralArea = JsonSerialization.DeserializeString<QuadrilateralArea[]>(this.AuxQuadPoints);
            }

            if (!string.IsNullOrEmpty(this.AuxTvSpectrum))
            {
                this.callSign = JsonSerialization.DeserializeString<TvSpectrum>(this.AuxTvSpectrum);
            }

            if (!string.IsNullOrEmpty(this.AuxEvent))
            {
                this.auxEvent = JsonSerialization.DeserializeString<Event>(this.AuxEvent);
            }

            if (this.callSign != null && this.callSign.Channel.HasValue)
            {
                this.Channel = this.callSign.Channel.Value;
            }

            this.AuxRegDisposition = string.Empty;
            this.AuxRegistrant = string.Empty;
            this.AuxContact = string.Empty;
            this.AuxPointsArea = string.Empty;
            this.AuxQuadPoints = string.Empty;
            this.AuxTvSpectrum = string.Empty;
            this.AuxEvent = string.Empty;
        }
    }
}
