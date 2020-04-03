// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;

    /// <summary>
    /// Represents an antenna height type. 
    /// </summary>
    public enum HeightType
    {
        /// <summary>represents None</summary>
        None = 0,

        /// <summary>
        /// Above Ground Level (AGL).
        /// </summary>
        AGL,

        /// <summary>
        /// Above Mean Sea Level.
        /// </summary>
        AMSL,

        /// <summary>The Above mean sea level</summary>
        ASL
    }

    /// <summary>
    /// Represents the Antenna Characteristics.
    /// </summary>
    public class AntennaCharacteristics : TableEntity
    {
        /// <summary>
        /// Gets or sets the antenna height.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameHeight)]
        public double Height { get; set; }

        /// <summary>
        /// Gets or sets the antenna height type.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameHeightType)]        
        public HeightType HeightType { get; set; }

        /// <summary>
        /// Gets or sets the antenna height uncertainty.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameHeightUncertainty)]
        public double HeightUncertainty { get; set; }
    }
}
