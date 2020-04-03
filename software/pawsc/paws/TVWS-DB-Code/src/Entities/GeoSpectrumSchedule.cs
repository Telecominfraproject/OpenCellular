// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.ComponentModel.DataAnnotations;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents the paws geo spectrum schedule.
    /// </summary>
    [Serializable]
    [JsonConverter(typeof(GeoSpectrumScheduleConverter))]
    public class GeoSpectrumSchedule
    {
        /// <summary>
        /// Gets or sets the geo spectrum location.
        /// </summary>
        [Required(ErrorMessage = Constants.ErrorMessageGeoSpectrumLocationRequired)]
        [JsonProperty(Constants.PropertyNameLocation)]
        public GeoLocation Location { get; set; }

        /// <summary>
        /// Gets or sets the geo spectrum schedules.
        /// </summary>
        [Required(ErrorMessage = Constants.ErrorMessageSpectrumSchedulesRequired)]
        [JsonProperty(Constants.PropertyNameSpectrumSchedules)]
        public SpectrumSchedule[] SpectrumSchedules { get; set; }
    }
}
