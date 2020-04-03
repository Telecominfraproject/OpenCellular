// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents Class ULSRecord.
    /// </summary>
    public class ULSRecord : TableEntity
    {
        /// <summary>
        /// Gets or sets the call sign.
        /// </summary>
        /// <value>The call sign.</value>
        public string CallSign { get; set; }

        /// <summary>
        /// Gets or sets the unique system identifier.
        /// </summary>
        /// <value>The unique system identifier.</value>
        public int UniqueSystemIdentifier { get; set; }

        /// <summary>
        /// Gets or sets the channel.
        /// </summary>
        /// <value>The channel.</value>
        public int Channel { get; set; }

        /// <summary>
        /// Gets or sets the class station code.
        /// </summary>
        /// <value>The class station code.</value>
        public string ClassStationCode { get; set; }

        /// <summary>
        /// Gets or sets the frequency assigned.
        /// </summary>
        /// <value>The frequency assigned.</value>
        public int FrequencyAssigned { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether this instance is new.
        /// </summary>
        /// <value><c>true</c> if this instance is new; otherwise, <c>false</c>.</value>
        public bool IsNew { get; set; }

        /// <summary>
        /// Gets or sets the latitude.
        /// </summary>
        /// <value>The latitude.</value>
        public double Latitude { get; set; }

        /// <summary>
        /// Gets or sets the longitude.
        /// </summary>
        /// <value>The longitude.</value>
        public double Longitude { get; set; }

        /// <summary>
        /// Gets or sets the license status.
        /// </summary>
        /// <value>The license status.</value>
        public string LicenseStatus { get; set; }

        /// <summary>
        /// Gets or sets the radio service code.
        /// </summary>
        /// <value>The radio service code.</value>
        public string RadioServiceCode { get; set; }

        /// <summary>
        /// Gets or sets the radius of operation.
        /// </summary>
        /// <value>The radius of operation.</value>
        public double RadiusOfOperation { get; set; }

        /// <summary>
        /// Gets or sets the transmitter power.
        /// </summary>
        /// <value>The transmitter power.</value>
        public double TxPower { get; set; }

        /// <summary>
        /// Gets or sets the state.
        /// </summary>
        /// <value>The state.</value>
        public string State { get; set; }

        /// <summary>
        /// Gets or sets the city.
        /// </summary>
        /// <value>The city.</value>
        public string City { get; set; }

        /// <summary>
        /// Gets or sets the state of the registration.
        /// </summary>
        /// <value>The state of the registration.</value>
        public string RegState { get; set; }

        /// <summary>
        /// Gets or sets the broadcast call sign.
        /// </summary>
        /// <value>The broadcast call sign.</value>
        public string BroadcastCallSign { get; set; }

        /// <summary>
        /// Gets or sets the broadcast city.
        /// </summary>
        /// <value>The broadcast city.</value>
        public string BroadcastCity { get; set; }

        /// <summary>
        /// Gets or sets the state of the broadcast.
        /// </summary>
        /// <value>The state of the broadcast.</value>
        public string BroadcastState { get; set; }

        /// <summary>
        /// Gets or sets the facility identifier of parent station.
        /// </summary>
        /// <value>The facility identifier of parent station.</value>
        public int FacilityIDOfParentStation { get; set; }

        /// <summary>
        /// Gets or sets the radio service code of parent station.
        /// </summary>
        /// <value>The radio service code of parent station.</value>
        public string RadioServiceCodeOfParentStation { get; set; }

        /// <summary>
        /// Gets or sets the TX latitude.
        /// </summary>
        /// <value>The TX latitude.</value>
        public double TxLatitude { get; set; }

        /// <summary>
        /// Gets or sets the TX longitude.
        /// </summary>
        /// <value>The TX longitude.</value>
        public double TxLongitude { get; set; }

        /// <summary>
        /// Gets or sets the broadcast station contour.
        /// </summary>
        /// <value>The broadcast station contour.</value>
        public string BroadcastStationContour { get; set; }

        /// <summary>
        /// Gets or sets the name of the entity.
        /// </summary>
        /// <value>The name of the entity.</value>
        public string RegEntityName { get; set; }

        /// <summary>
        /// Gets or sets the first name.
        /// </summary>
        /// <value>The first name.</value>
        public string RegFirstName { get; set; }

        /// <summary>
        /// Gets or sets the last name.
        /// </summary>
        /// <value>The last name.</value>
        public string RegLastName { get; set; }

        /// <summary>
        /// Gets or sets the phone.
        /// </summary>
        /// <value>The phone.</value>
        public string RegPhone { get; set; }

        /// <summary>
        /// Gets or sets the fax.
        /// </summary>
        /// <value>The fax.</value>
        public string RegFax { get; set; }

        /// <summary>
        /// Gets or sets the email.
        /// </summary>
        /// <value>The email.</value>
        public string RegEmail { get; set; }

        /// <summary>
        /// Gets or sets the street address.
        /// </summary>
        /// <value>The street address.</value>
        public string RegStreetAddress { get; set; }

        /// <summary>
        /// Gets or sets the city.
        /// </summary>
        /// <value>The city.</value>
        public string RegCity { get; set; }

        /// <summary>
        /// Gets or sets the zip code.
        /// </summary>
        /// <value>The zip code.</value>
        public string RegZipCode { get; set; }

        /// <summary>
        /// Gets or sets the po box.
        /// </summary>
        /// <value>The po box.</value>
        public string RegPOBox { get; set; }

        /// <summary>
        /// Gets or sets the name of the entity.
        /// </summary>
        /// <value>The name of the entity.</value>
        public string ContactEntityName { get; set; }

        /// <summary>
        /// Gets or sets the first name.
        /// </summary>
        /// <value>The first name.</value>
        public string ContactFirstName { get; set; }

        /// <summary>
        /// Gets or sets the last name.
        /// </summary>
        /// <value>The last name.</value>
        public string ContactLastName { get; set; }

        /// <summary>
        /// Gets or sets the phone.
        /// </summary>
        /// <value>The phone.</value>
        public string ContactPhone { get; set; }

        /// <summary>
        /// Gets or sets the fax.
        /// </summary>
        /// <value>The fax.</value>
        public string ContactFax { get; set; }

        /// <summary>
        /// Gets or sets the email.
        /// </summary>
        /// <value>The email.</value>
        public string ContactEmail { get; set; }

        /// <summary>
        /// Gets or sets the street address.
        /// </summary>
        /// <value>The street address.</value>
        public string ContactStreetAddress { get; set; }

        /// <summary>
        /// Gets or sets the city.
        /// </summary>
        /// <value>The city.</value>
        public string ContactCity { get; set; }

        /// <summary>
        /// Gets or sets the zip code.
        /// </summary>
        /// <value>The zip code.</value>
        public string ContactZipCode { get; set; }

        /// <summary>
        /// Gets or sets the po box.
        /// </summary>
        /// <value>The po box.</value>
        public string ContactPOBox { get; set; }

        /// <summary>
        /// Gets or sets the state of the contact.
        /// </summary>
        /// <value>The state of the contact.</value>
        public string ContactState { get; set; }

        /// <summary>
        /// Gets or sets the FCC registration number.
        /// </summary>
        /// <value>The FCC registration number.</value>
        public string FCCRegistrationNumber { get; set; }

        /// <summary>
        /// Gets or sets the status code.
        /// </summary>
        /// <value>The status code.</value>
        public string StatusCode { get; set; }

        /// <summary>
        /// Gets or sets the ULS file number.
        /// </summary>
        /// <value>The ULS file number.</value>
        public string ULSFileNumber { get; set; }

        /// <summary>
        /// Gets or sets the entity type code.
        /// </summary>
        /// <value>The entity type code.</value>
        public string EntityTypeCode { get; set; }

        /// <summary>
        /// Gets or sets the azimuth to parent.
        /// </summary>
        /// <value>The azimuth to parent.</value>
        public double AzimuthToParent { get; set; }

        /// <summary>
        /// Gets or sets the location number.
        /// </summary>
        /// <value>The location number.</value>
        public int LocationNumber { get; set; }

        /// <summary>
        /// Gets or sets the location class code.
        /// </summary>
        /// <value>The location class code.</value>
        public string LocationClassCode { get; set; }

        /// <summary>
        /// Gets or sets the key hole radius in MTRS.
        /// </summary>
        /// <value>The key hole radius in MTRS.</value>
        public double KeyHoleRadiusMtrs { get; set; }

        /// <summary>
        /// Gets or sets the distance to base station.
        /// </summary>
        /// <value>The distance to base station.</value>
        public int DistanceToBaseStation { get; set; }

        /// <summary>
        /// Gets or sets the name of the venue.
        /// </summary>
        /// <value>The name of the venue.</value>
        public string VenueName { get; set; }

        /// <summary>
        /// Gets or sets the venue type code.
        /// </summary>
        /// <value>The venue type code.</value>
        public string VenueTypeCode { get; set; }

        /// <summary>
        /// Gets or sets the DEV or STA mode.
        /// </summary>
        /// <value>The DEV or STA mode.</value>
        public string DevOrSTAMode { get; set; }

        /// <summary>
        /// Gets or sets the corresponding fixed location.
        /// </summary>
        /// <value>The corresponding fixed location.</value>
        public string CorrespondingFixedLocation { get; set; }

        /// <summary>
        /// Gets or sets the frequency number.
        /// </summary>
        /// <value>The frequency number.</value>
        public int FrequencyNumber { get; set; }

        /// <summary>
        /// Gets or sets the frequency location number.
        /// </summary>
        /// <value>The frequency location number.</value>
        public int FrequencyLocationNumber { get; set; }

        /// <summary>
        /// Gets or sets the name of the owner.
        /// </summary>
        /// <value>The name of the owner.</value>
        public string OwnerName { get; set; }

        /// <summary>
        /// Gets or sets the grant date.
        /// </summary>
        /// <value>The grant date.</value>
        public DateTimeOffset? GrantDate { get; set; }

        /// <summary>
        /// Gets or sets the expire date.
        /// </summary>
        /// <value>The expire date.</value>
        public DateTimeOffset? ExpireDate { get; set; }

        /// <summary>
        /// Gets or sets the maximum wireless microphones.
        /// </summary>
        /// <value>The maximum wireless microphones.</value>
        public string MaxWirelessMicrophones { get; set; }

        /// <summary>
        /// Gets or sets the channels.
        /// </summary>
        /// <value>The channels.</value>
        public int[] Channels
        {
            get
            {
                if (!string.IsNullOrEmpty(this.MultipleChannelsString))
                {
                    return JsonSerialization.DeserializeString<int[]>(this.MultipleChannelsString);
                }

                return null;
            }

            set
            {
                if (value != null)
                {
                    this.MultipleChannelsString = JsonSerialization.SerializeObject(value);
                }
            }
        }

        /// <summary>
        /// Gets or sets the multiple channels.
        /// </summary>
        /// <value>The multiple channels.</value>
        public string MultipleChannelsString { get; set; }

        /// <summary>
        /// Gets the location.
        /// </summary>
        /// <value>The location.</value>
        public Location Location
        {
            get
            {
                return new Location(this.Latitude, this.Longitude);
            }
        }

        /// <summary>
        /// Gets the transmitter location.
        /// </summary>
        /// <value>The transmitter location.</value>
        public Location TxLocation
        {
            get
            {
                return new Location(this.TxLatitude, this.TxLongitude);
            }
        }

        /// <summary>
        /// Merges the location data.
        /// </summary>
        /// <param name="objRecord">The record location.</param>
        public void MergePLMRSLocationData(ULSRecord objRecord)
        {
            this.LocationNumber = objRecord.LocationNumber;
            this.CorrespondingFixedLocation = objRecord.CorrespondingFixedLocation;
            this.Latitude = objRecord.Latitude;
            this.Longitude = objRecord.Longitude;
            this.State = objRecord.State;
            this.City = objRecord.City;
            this.RadiusOfOperation = objRecord.RadiusOfOperation;
            this.DistanceToBaseStation = objRecord.DistanceToBaseStation;
        }

        /// <summary>
        /// Merges the location data.
        /// </summary>
        /// <param name="objRecord">The record location.</param>
        public void MergeLocationData(ULSRecord objRecord)
        {
            if (objRecord != null)
            {
                this.LocationNumber = objRecord.LocationNumber;
                this.CorrespondingFixedLocation = objRecord.CorrespondingFixedLocation;

                // if site is Receiver
                if (string.IsNullOrEmpty(objRecord.LocationClassCode) || objRecord.LocationClassCode == "R")
                {
                    this.Latitude = objRecord.Latitude;
                    this.Longitude = objRecord.Longitude;
                    this.State = objRecord.State;
                    this.RadiusOfOperation = objRecord.RadiusOfOperation;
                    this.DistanceToBaseStation = objRecord.DistanceToBaseStation;
                }
                else if (objRecord.LocationClassCode == "T")
                {
                    // if site is transmitter
                    this.TxLatitude = objRecord.Latitude;
                    this.TxLongitude = objRecord.Longitude;
                }
            }
        }

        /// <summary>
        /// Merges the frequency data.
        /// </summary>
        /// <param name="objRecord">The record Frequency.</param>
        public void MergeFrequencyData(ULSRecord objRecord)
        {
            if (objRecord != null)
            {
                this.FrequencyAssigned = objRecord.FrequencyAssigned;
                this.TxPower = objRecord.TxPower;
                this.Channel = objRecord.Channel;
                this.ClassStationCode = objRecord.ClassStationCode;
                this.FrequencyNumber = objRecord.FrequencyNumber;
                this.FrequencyLocationNumber = objRecord.FrequencyLocationNumber;
            }
        }

        /// <summary>
        /// Merges the broadcast Station data.
        /// </summary>
        /// <param name="objRecord">The object record.</param>
        public void MergeBroadcastCallSignData(ULSRecord objRecord)
        {
            if (objRecord != null)
            {
                this.BroadcastCallSign = objRecord.BroadcastCallSign;
                this.FacilityIDOfParentStation = objRecord.FacilityIDOfParentStation;
                this.BroadcastCity = objRecord.BroadcastCity;
                this.BroadcastState = objRecord.BroadcastState;
                this.RadioServiceCodeOfParentStation = objRecord.RadioServiceCodeOfParentStation;
                this.TxLatitude = objRecord.TxLatitude;
                this.TxLongitude = objRecord.TxLongitude;
                this.BroadcastStationContour = objRecord.BroadcastStationContour;
            }
        }

        /// <summary>
        /// Merges the entity registration data.
        /// </summary>
        /// <param name="objRecord">The object record.</param>
        public void MergeEntityRegistrationData(ULSRecord objRecord)
        {
            if (objRecord != null)
            {
                if (objRecord.EntityTypeCode == "CL" || objRecord.EntityTypeCode == "C")
                {
                    this.ContactEntityName = objRecord.RegEntityName;
                    this.ContactFirstName = objRecord.RegFirstName;
                    this.ContactLastName = objRecord.RegLastName;
                    this.ContactPhone = objRecord.RegPhone;
                    this.ContactFax = objRecord.RegFax;
                    this.ContactEmail = objRecord.RegEmail;
                    this.ContactStreetAddress = objRecord.RegStreetAddress;
                    this.ContactCity = objRecord.RegCity;
                    this.ContactState = objRecord.RegState;
                    this.ContactZipCode = objRecord.RegZipCode;
                    this.ContactPOBox = objRecord.RegPOBox;
                }
                else
                {
                    this.RegEntityName = objRecord.RegEntityName;
                    this.RegFirstName = objRecord.RegFirstName;
                    this.RegLastName = objRecord.RegLastName;
                    this.RegPhone = objRecord.RegPhone;
                    this.RegFax = objRecord.RegFax;
                    this.RegEmail = objRecord.RegEmail;
                    this.RegStreetAddress = objRecord.RegStreetAddress;
                    this.RegCity = objRecord.RegCity;
                    this.State = objRecord.State;
                    this.RegZipCode = objRecord.RegZipCode;
                    this.RegPOBox = objRecord.RegPOBox;
                    this.FCCRegistrationNumber = objRecord.FCCRegistrationNumber;
                    this.StatusCode = objRecord.StatusCode;
                }
            }
        }

        /// <summary>
        /// Merges the NA data.
        /// </summary>
        /// <param name="objRecord">The object record.</param>
        public void MergeNAData(ULSRecord objRecord)
        {
            if (objRecord != null)
            {
                if (objRecord.EntityTypeCode == "R")
                {
                    this.RegEntityName = objRecord.RegEntityName;
                    this.RegFirstName = objRecord.RegFirstName;
                    this.RegLastName = objRecord.RegLastName;
                    this.RegPhone = objRecord.RegPhone;
                    this.RegFax = objRecord.RegFax;
                    this.RegEmail = objRecord.RegEmail;
                    this.RegStreetAddress = objRecord.RegStreetAddress;
                    this.RegCity = objRecord.RegCity;
                    this.RegState = objRecord.RegState;
                    this.RegZipCode = objRecord.RegZipCode;
                    this.RegPOBox = objRecord.RegPOBox;
                    this.FCCRegistrationNumber = objRecord.FCCRegistrationNumber;
                    this.StatusCode = objRecord.StatusCode;
                    this.EntityTypeCode = objRecord.EntityTypeCode;
                }
                else if (objRecord.EntityTypeCode == "C")
                {
                    this.ContactEntityName = objRecord.RegEntityName;
                    this.ContactFirstName = objRecord.RegFirstName;
                    this.ContactLastName = objRecord.RegLastName;
                    this.ContactPhone = objRecord.RegPhone;
                    this.ContactFax = objRecord.RegFax;
                    this.ContactEmail = objRecord.RegEmail;
                    this.ContactStreetAddress = objRecord.RegStreetAddress;
                    this.ContactCity = objRecord.RegCity;
                    this.ContactState = objRecord.RegState;
                    this.ContactZipCode = objRecord.RegZipCode;
                    this.ContactPOBox = objRecord.RegPOBox;
                }
            }
        }

        /// <summary>
        /// Merges the venue information.
        /// </summary>
        /// <param name="objRecord">The object record.</param>
        public void MergeVenueInformation(ULSRecord objRecord)
        {
            if (objRecord != null)
            {
                this.VenueName = objRecord.VenueName;
                this.VenueTypeCode = objRecord.VenueTypeCode;
                this.RegStreetAddress = objRecord.RegStreetAddress;
                this.RegCity = objRecord.RegCity;
                this.RegState = objRecord.RegState;
                this.RegZipCode = objRecord.RegZipCode;
            }
        }
    }
}
