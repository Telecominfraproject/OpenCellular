#!/bin/sh 
# following steps will simulate a flash corruption to force recreating it at next reboot
# WARNING: all custom file in flash must be rewritten to flash after a following reboot
# any customization manually made in flash (like customizing passwd) will be lost after that 
rm /mnt/app/.sign1985
rm -rf /mnt/app/.ssh
sync
/etc/init.d/flashmnt
sync
