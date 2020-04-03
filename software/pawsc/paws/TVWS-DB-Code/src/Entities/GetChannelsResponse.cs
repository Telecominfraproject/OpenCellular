// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using Newtonsoft.Json;

    /// <summary>
    /// Represents Class GetChannelsResponse.
    /// </summary>
    public class GetChannelsResponse
    {
        /// <summary>
        /// Gets or sets the channels information.
        /// </summary>
        /// <value>The channels information.</value>
        public ChannelInfo[] ChannelsInfo { get; set; }

        /// <summary>
        /// Gets or sets the channels in CSV format.
        /// </summary>
        /// <value>The channels in CSV format.</value>
        public string ChannelsInCSVFormat { get; set; }

        /// <summary>
        /// Gets or sets the intermediate result1.
        /// </summary>
        /// <value>The intermediate result1.</value>
        public string IntermediateResult1 { get; set; }

        /// <summary>
        /// Gets or sets the intermediate result2.
        /// </summary>
        /// <value>The intermediate result2.</value>
        public string IntermediateResult2 { get; set; }
    }
}
