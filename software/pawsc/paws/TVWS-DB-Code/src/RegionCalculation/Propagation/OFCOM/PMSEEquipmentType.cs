// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.RegionCalculation.Propagation
{
    /// <summary>enumeration PMSEUseCase</summary>
    public enum PMSEEquipmentType
    {
        /// <summary>The none</summary>
        None,

        /// <summary>The talkback</summary>
        Talkback = 1,

        /// <summary>The wireless microphones</summary>
        WirelessMicrophones = 2,

        /// <summary>The audio links</summary>
        ProgrammeAudioLinks = 4,

        /// <summary>The video links</summary>
        ProgrammeVideoLinks = 8,

        /// <summary>The data links</summary>
        DataLinks = 16,

        /// <summary>The in ear monitors</summary>
        InEarMonitors = 64,
    }
}
