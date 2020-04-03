// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;

    /// <summary>
    ///     Represents Class LPAUX LicenseInfo.
    /// </summary>
    public class LpAuxLicenseInfo
    {
        /// <summary>
        ///     Gets or sets the call sign.
        /// </summary>
        /// <value>The call sign.</value>
        public string CallSign { get; set; }

        /// <summary>
        ///     Gets or sets the channel.
        /// </summary>
        /// <value>The channel.</value>
        public int Channel { get; set; }

        /// <summary>
        ///     Gets or sets the state of the registration.
        /// </summary>
        /// <value>The state of the registration.</value>
        public string RegState { get; set; }

        /// <summary>
        ///     Gets or sets the name of the entity.
        /// </summary>
        /// <value>The name of the entity.</value>
        public string RegEntityName { get; set; }

        /// <summary>
        ///     Gets or sets the first name.
        /// </summary>
        /// <value>The first name.</value>
        public string RegFirstName { get; set; }

        /// <summary>
        ///     Gets or sets the last name.
        /// </summary>
        /// <value>The last name.</value>
        public string RegLastName { get; set; }

        /// <summary>
        ///     Gets or sets the phone.
        /// </summary>
        /// <value>The phone.</value>
        public string RegPhone { get; set; }

        /// <summary>
        ///     Gets or sets the fax.
        /// </summary>
        /// <value>The fax.</value>
        public string RegFax { get; set; }

        /// <summary>
        ///     Gets or sets the email.
        /// </summary>
        /// <value>The email.</value>
        public string RegEmail { get; set; }

        /// <summary>
        ///     Gets or sets the street address.
        /// </summary>
        /// <value>The street address.</value>
        public string RegStreetAddress { get; set; }

        /// <summary>
        ///     Gets or sets the city.
        /// </summary>
        /// <value>The city.</value>
        public string RegCity { get; set; }

        /// <summary>
        ///     Gets or sets the zip code.
        /// </summary>
        /// <value>The zip code.</value>
        public string RegZipCode { get; set; }

        /// <summary>
        ///     Gets or sets the po box.
        /// </summary>
        /// <value>The po box.</value>
        public string RegPOBox { get; set; }

        /// <summary>
        ///     Gets or sets the name of the entity.
        /// </summary>
        /// <value>The name of the entity.</value>
        public string ContactEntityName { get; set; }

        /// <summary>
        ///     Gets or sets the first name.
        /// </summary>
        /// <value>The first name.</value>
        public string ContactFirstName { get; set; }

        /// <summary>
        ///     Gets or sets the last name.
        /// </summary>
        /// <value>The last name.</value>
        public string ContactLastName { get; set; }

        /// <summary>
        ///     Gets or sets the phone.
        /// </summary>
        /// <value>The phone.</value>
        public string ContactPhone { get; set; }

        /// <summary>
        ///     Gets or sets the fax.
        /// </summary>
        /// <value>The fax.</value>
        public string ContactFax { get; set; }

        /// <summary>
        ///     Gets or sets the email.
        /// </summary>
        /// <value>The email.</value>
        public string ContactEmail { get; set; }

        /// <summary>
        ///     Gets or sets the street address.
        /// </summary>
        /// <value>The street address.</value>
        public string ContactStreetAddress { get; set; }

        /// <summary>
        ///     Gets or sets the city.
        /// </summary>
        /// <value>The city.</value>
        public string ContactCity { get; set; }

        /// <summary>
        ///     Gets or sets the zip code.
        /// </summary>
        /// <value>The zip code.</value>
        public string ContactZipCode { get; set; }

        /// <summary>
        ///     Gets or sets the po box.
        /// </summary>
        /// <value>The po box.</value>
        public string ContactPOBox { get; set; }

        /// <summary>
        ///     Gets or sets the state of the contact.
        /// </summary>
        /// <value>The state of the contact.</value>
        public string ContactState { get; set; }

        /// <summary>
        ///     Gets or sets the ULS file number.
        /// </summary>
        /// <value>The ULS file number.</value>
        public string ULSFileNumber { get; set; }

        /// <summary>
        /// Gets or sets the name of the owner.
        /// </summary>
        /// <value>The name of the owner.</value>
        public string OwnerName { get; set; }

        /// <summary>
        /// Gets or sets the grant date.
        /// </summary>
        /// <value>The grant date.</value>
        public DateTimeOffset GrantDate { get; set; }

        /// <summary>
        /// Gets or sets the expire date.
        /// </summary>
        /// <value>The expire date.</value>
        public DateTimeOffset ExpireDate { get; set; }

        /// <summary>
        /// Gets or sets the maximum wireless microphones.
        /// </summary>
        /// <value>The maximum wireless microphones.</value>
        public string MaxWirelessMicrophones { get; set; }

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
        /// Gets or sets the channels.
        /// </summary>
        /// <value>The channels.</value>
        public int[] Channels { get; set; }
    }
}
