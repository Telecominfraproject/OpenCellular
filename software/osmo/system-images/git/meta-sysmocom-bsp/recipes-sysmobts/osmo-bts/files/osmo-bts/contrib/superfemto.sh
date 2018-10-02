#!/bin/sh

# Split common DSP call log file (produced by superfemto-compatible firmware) into 4 separate call leg files (MO/MT & DL/UL) with events in format "FN EVENT_TYPE":
# MO Mobile Originated
# MT Mobile Terminated
# DL DownLink (BTS -> L1)
# UL UpLink (L1 -> BTS)

if [ -z $1 ]; then
    echo "expecting DSP log file name as parameter"
    exit 1
fi

# MO handle appear 1st in the logs
MO=$(grep 'h=' $1 | head -n 1 | cut -f2 -d',' | cut -f2 -d= | cut -f1 -d']')

# direction markers:
DLST="_CodeBurst"
ULST="_DecodeAndIdentify"

# DL sed filters:
D_EMP='s/ Empty frame request!/EMPTY/i'
D_FAC='s/ Coding a FACCH\/. frame !!/FACCH/i'
D_FST='s/ Coding a RTP SID First frame !!/FIRST/i'
D_FS1='s/ Coding a SID First P1 frame !!/FIRST_P1/i'
D_FS2='s/ Coding a SID First P2 frame !!/FIRST_P2/i'
D_RP1='s/ Coding a RTP SID P1 frame !!/SID_P1/i'
D_UPD='s/ Coding a RTP SID Update frame !!/UPDATE/i'
D_SPE='s/ Coding a RTP Speech frame !!/SPEECH/i'
D_ONS='s/ Coding a Onset frame !!/ONSET/i'
D_FO1='s/ A speech frame is following a NoData or SID First without an Onset./FORCED_FIRST/i'
D_FO2='s/ A speech frame is following a NoData without an Onset./FORCED_NODATA/i'
D_FP2='s/ A speech frame is following a NoData or SID_FIRST_P2 without an Onset./FORCED_F_P2/i'
D_FIN='s/ A speech frame is following a SID_FIRST without inhibit. A SID_FIRST_INH will be inserted./FORCED_F_INH/i'
D_UIN='s/ A speech frame is following a SID_UPDATE without inhibit. A SID_UPDATE_INH will be inserted./FORCED_U_INH/i'

# UL sed filters:
U_NOD='s/ It is a No Data frame !!/NODATA/i'
U_ONS='s/ It is an ONSET frame !!/ONSET/i'
U_UPD='s/ It is a SID UPDATE frame !!/UPDATE/i'
U_FST='s/ It is a SID FIRST frame !!/FIRST/i'
U_FP1='s/ It is a SID-First P1 frame !!/FIRST_P1/i'
U_FP2='s/ It is a SID-First P2 frame !!/FIRST_P2/i'
U_SPE='s/ It is a SPEECH frame *!!/SPEECH/i'
U_UIN='s/ It is a SID update InH frame !!/UPD_INH/i'
U_FIN='s/ It is a SID-First InH frame !!/FST_INH/i'
U_RAT='s/ It is a RATSCCH data frame !!/RATSCCH/i'

DL () { # filter downlink-related entries
    grep $DLST $1 > $1.DL.tmp
}

UL () { # uplink does not require special fix
    grep $ULST $1 > $1.UL.tmp.fix
}

DL $1
UL $1

FIX() { # add MO/MT marker from preceding line to inserted ONSETs so filtering works as expected
    cat $1.DL.tmp | awk 'BEGIN{ FS=" h="; H="" } { if (NF > 1) { H = $2; print $1 "h=" $2 } else { print $1 ", h=" H } }' > $1.DL.tmp.fix
}

FIX $1

MO() { # filter MO call DL or UL logs
    grep "h=$MO" $1.tmp.fix > $1.MO.raw
}

MT() { # filter MT call DL or UL logs
    grep -v "h=$MO" $1.tmp.fix > $1.MT.raw
}

MO $1.DL
MT $1.DL
MO $1.UL
MT $1.UL

PREP() { # prepare logs for reformatting
    cat $1.raw | cut -f2 -d')' | cut -f1 -d',' | cut -f2 -d'>' | sed 's/\[u32Fn/fn/' | sed 's/\[ u32Fn/fn/' | sed 's/fn = /fn=/' | sed 's/fn=//' | sed 's/\[Fn=//' | sed 's/ An Onset will be inserted.//' > $1.tmp1
}

PREP "$1.DL.MT"
PREP "$1.DL.MO"
PREP "$1.UL.MT"
PREP "$1.UL.MO"

RD() { # reformat DL logs for consistency checks
    cat $1.tmp1 | sed "$D_FST" | sed "$D_SPE" | sed "$D_FS1" | sed "$D_FS2" | sed "$D_UIN" | sed "$D_FIN" | sed "$D_UPD" | sed "$D_INH" | sed "$D_RP1" | sed "$D_ONS" | sed "$D_EMP" | sed "$D_FAC" | sed "$D_FO1" | sed "$D_FO2" | sed "$D_FP2" > $1.tmp2
}

RU() { # reformat UL logs for consistency checks
    cat $1.tmp1 | sed "$U_FST" | sed "$U_SPE" | sed "$U_FP1" | sed "$U_FP2" | sed "$U_UPD" | sed "$U_ONS" | sed "$U_NOD" | sed "$U_UIN" | sed "$U_FIN" | sed "$U_RAT" > $1.tmp2
}

RD "$1.DL.MT"
RD "$1.DL.MO"
RU "$1.UL.MT"
RU "$1.UL.MO"

SW() { # swap fields
    cat $1.tmp2 | awk '{ print $2, $1 }' > $1
}

SW "$1.DL.MT"
SW "$1.DL.MO"
SW "$1.UL.MT"
SW "$1.UL.MO"

rm $1.*.tmp*
