/*
Copyright(c) Microsoft Corporation.All rights reserved.
Licensed under the MIT License.
*/

#ifndef SCTP_CFG_OPTIONS_H_
#define SCTP_CFG_OPTIONS_H_


#define REBOOT_AFTER_RFTXSTATUS_DROP			// some FAPs do not automatically restart the radio once it had been turned off.
												// With this setting, we will force a reboot if the eNB socket is opened folliwng a radio drop.



#endif // SCTP_CFG_OPTIONS_H_
