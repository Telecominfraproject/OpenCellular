// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents Class MergedData.
    /// </summary>
    public class MergedData : TableEntity
    {
        /// <summary>
        /// Gets or sets Location Latitude
        /// </summary>
        /// <value>The latitude.</value>
        public double Latitude { get; set; }

        /// <summary>
        /// Gets or sets Location Longitude
        /// </summary>
        /// <value>The longitude.</value>
        public double Longitude { get; set; }

        /// <summary>
        /// Gets or sets Location Longitude
        /// </summary>
        /// <value>The channel number.</value>
        public int ChannelNumber { get; set; }

        /// <summary>
        /// Gets or sets the channel.
        /// </summary>
        /// <value>The channel.</value>
        public int Channel { get; set; }

        /// <summary>
        /// Gets or sets Transmit Location Latitude
        /// </summary>
        /// <value>The Transmit latitude.</value>
        public double TxLatitude { get; set; }

        /// <summary>
        /// Gets or sets Transmit Location Longitude
        /// </summary>
        /// <value>The Transmit longitude.</value>
        public double TxLongitude { get; set; }

        /// <summary>
        /// Gets or sets the parent latitude.
        /// </summary>
        /// <value>The parent latitude.</value>
        public double ParentLatitude { get; set; }

        /// <summary>
        /// Gets or sets the parent longitude.
        /// </summary>
        /// <value>The parent longitude.</value>
        public double ParentLongitude { get; set; }

        /// <summary>
        /// Gets or sets the temporary bas event.
        /// </summary>
        /// <value>The temporary bas event.</value>
        public string TempBASEvent { get; set; }

        /// <summary>
        /// Gets or sets the call sign.
        /// </summary>
        /// <value>The call sign.</value>
        public string CallSign { get; set; }

        /// <summary>
        /// Gets or sets the combined data identifier.
        /// </summary>
        /// <value>The combined data identifier.</value>
        public string MergedDataIdentifier { get; set; }
    }
}
