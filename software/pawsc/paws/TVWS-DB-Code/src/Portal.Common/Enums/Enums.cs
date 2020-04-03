// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common.Enums
{
    public enum ChannelOperationMode : int
    {
        LowPower = 0,
        HighPower = 1,
        None = 2
    }

    public enum RequestStatus : int
    {
        Pending  = 0,
        Approved = 1,
        Reject = 2
    }

    public enum RegistrationType : int
    {
        LpAux = 0,
        TBas = 1,
        Mvpd = 2
    }
}
