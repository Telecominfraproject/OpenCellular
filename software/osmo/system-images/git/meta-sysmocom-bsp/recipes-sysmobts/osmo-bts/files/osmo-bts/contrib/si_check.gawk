#!/usr/bin/gawk -f

# Usage example:
# tshark -2 -t r -E 'header=n' -E 'separator=,' -E 'quote=n' -T fields -e gsmtap.frame_nr -e gsmtap.ts -e gsmtap.arfcn -e _ws.col.Info -Y 'gsmtap' -r test.pcapng.gz | grep Information | env ARFCN=878 ./si_check.gawk
# read summary on number of bis/ter messages and adjust BT_BOTH and BT_NONE environment variables accordingly

BEGIN {
	FS = ","
	FAILED = 0
	IGNORE = 0
	BIS = 0
	TER = 0
	QUA = 0
	BT_BOTH = ENVIRON["BOTH"]
	BT_NONE = ENVIRON["NONE"]
	TC_INDEX = 0
	TC4[4] = 0
}

{ # expected .csv input as follows: gsmtap.frame_nr,gsmtap.ts,gsmtap.arfcn,_ws.col.Info
	if ("ARFCN" in ENVIRON) { # ARFCN filtering is enabled
		if (ENVIRON["ARFCN"] != $3) { # ignore other ARFCNs
			IGNORE++
			next
		}
	}
	type = get_si_type($4)
	tc = get_tc($1)
	result = "FAIL"

	if (1 == check_si_tc(tc, type)) { result = "OK" }
	else { FAILED++ }

	if (4 == tc) {
		TC4[TC_INDEX] = type
		TC_INDEX = int((TC_INDEX + 1) % 4)
		if (0 == check_tc4c(type) && "OK" == result) {
			result = "FAIL"
			FAILED++
		}
	}
	if (type == "2bis") { BIS++ }
	if (type == "2ter") { TER++ }
	if (type == "2quater") { QUA++ }
	# for (i in TC4) print TC4[i] # debugging
	printf "ARFCN=%d FN=%d TS=%d TC=%d TYPE=%s %s\n", $3, $1, $2, tc, type, result
}

END {
	printf "check completed: total %d, failed %d, ignored %d, ok %d\nSI2bis = %d, SI2ter = %d, SI2quater = %d\n", NR, FAILED, IGNORE, NR - FAILED - IGNORE, BIS, TER, QUA
	if ((BIS > 0 || TER > 0) && BT_NONE) { printf "please re-run with correct environment: unset 'NONE' variable\n" }
	if ((BIS > 0 && TER > 0) && !BT_BOTH) { printf "please re-run with correct environment: set 'BOTH' variable\n" }
}

func get_si_type(s, x) { # we rely on format of Info column in wireshark output - if it's changed we're screwed
	return x[split(s, x, " ")]
}

func get_tc(f) { # N. B: all numbers in awk are float
	return int(int(f / 51) % 8)
}

func check_tc4c(si, count) { # check for "once in 4 consecutive occurrences" rule
	count = 0
	if ("2quater" != si || "2ter" != si) { return 1 } # rules is not applicable to other types
	if (BT_NONE && "2quater" == si) { return 0 } # should be on TC=5 instead
	if (!BT_BOTH && "2ter" == si) { return 0 } # should be on TC=5 instead
	if (0 in TC4 && 1 in TC4 && 2 in TC4 && 3 in TC4) { # only check if we have 4 consecutive occurrences already
		if (si == TC4[0]) { count++ }
		if (si == TC4[1]) { count++ }
		if (si == TC4[2]) { count++ }
		if (si == TC4[3]) { count++ }
		if (0 == count) { return 0 }
	}
	return 1
}

func check_si_tc(tc, si) { # check that SI scheduling on BCCH Norm is matching rules from 3GPP TS 05.02 § 6.3.1.3
	switch (si) {
	case "1":       return (0 == tc) ? 1 : 0
	case "2":       return (1 == tc) ? 1 : 0
	case "2bis":    return (5 == tc) ? 1 : 0
	case "13":      return (4 == tc) ? 1 : 0
	case "9":       return (4 == tc) ? 1 : 0
	case "2ter":    if (BT_BOTH) { return (4 == tc) ? 1 : 0 } else { return (5 == tc) ? 1 : 0 }
	case "2quater": if (BT_NONE) { return (5 == tc) ? 1 : 0 } else { return (4 == tc) ? 1 : 0 }
	case "3":       return (2 == tc || 6 == tc) ? 1 : 0
	case "4":       return (3 == tc || 7 == tc) ? 1 : 0
	}
	return 0
}
