#!/bin/sh

NTPDATE_CONF=/etc/default/ntpdate
NTP_CONF=/etc/ntp.conf

[ -r /etc/default/udhcpc-ntp ] && . /etc/default/udhcpc-ntp

if [ "$USE_NTPSERVER" != "yes" ]; then
    exit 0
fi

case "$1" in
        renew|bound)
                if [ -z "$ntpsrv" ]; then
                      exit 0
                fi

                printf "ntpserver is %s\n" $ntpsrv

                grep -q -s $ntpsrv ${NTPDATE_CONF}
                NTPDATE_UPDATE=$?
                if [ -w "${NTPDATE_CONF}" -a ${NTPDATE_UPDATE} != 0 ]; then
                    printf "changing ntpserver for ntpdate\n"
                    sed -i -e "s/\(NTPSERVERS=\)\(.*\)/\1\"$ntpsrv\"/g" ${NTPDATE_CONF}
                    /etc/network/if-up.d/ntpdate-sync
                fi

                grep -q -s $ntpsrv ${NTP_CONF}
                NTP_UPDATE=$?
                if [ -w "${NTP_CONF}" -a ${NTP_UPDATE} != 0 ]; then
                    printf "changing ntpserver for ntpd\n"
                    sed -i -e "1,/^server/s/^\(server\)\(.*\)/\1 $ntpsrv iburst/g" ${NTP_CONF}
                    systemctl restart ntpd
                fi
        ;;
esac
