#!/usr/bin/gawk -f

# Expected input format: FN TYPE

BEGIN {
	DELTA = 0
	ERR = 0
	FORCE = 0
	FN = 0
	SILENCE = 0
	TYPE = ""
	CHK = ""
	U_MAX = 8 * 20 + 120 / 26
	U_MIN = 8 * 20 - 120 / 26
	F_MAX = 3 * 20 + 120 / 26
	F_MIN = 3 * 20 - 120 / 26
}

{
	if (NR > 2) { # we have data from previous record to compare to
		DELTA = ($1 - FN) * 120 / 26
		CHK = "OK"
		if ("FACCH" == $2 && "ONSET" == TYPE) { # ONSET due to FACCH is NOT a talkspurt
			SILENCE = 1
		}
		if (("UPDATE" == TYPE || "FIRST" == TYPE) && ("FACCH" == $2 || "SPEECH" == $2)) { # check for missing ONSET:
				CHK = "FAIL: missing ONSET (" $2 ") after " TYPE "."
				ERR++
		}
		if ("SID_P1" == $2) {
			CHK = "FAIL: regular AMR payload with FT SID and STI=0 (should be either pyaload Update or STI=1)."
			ERR++
		}
		if ("FORCED_FIRST" == $2 || "FORCED_NODATA" == $2 || "FORCED_F_P2" == $2 || "FORCED_F_INH" == $2 || "FORCED_U_INH" == $2) {
			CHK = "FAIL: event " $2 " inserted by DSP."
			FORCE++
			ERR++
		}
		if ("FIRST_P2" != $2 && "FIRST_P1" == TYPE) {
			CHK = "FAIL: " TYPE " followed by " $2 " instead of P2."
			ERR++
		}
		if ("FIRST" == $2 && "FIRST" == TYPE) {
			CHK = "FAIL: multiple SID FIRST in a row."
			ERR++
		}
		if ("OK" == CHK && "ONSET" != $2) { # check inter-SID distances:
			if ("UPDATE" == TYPE) {
				if (DELTA > U_MAX) {
					CHK = "FAIL: delta (" $1 - FN "fn) from previous SID UPDATE (@" FN ") too big " DELTA "ms > " U_MAX "ms."
					ERR++
				}
				if ("UPDATE" == $2 && DELTA < U_MIN) {
					CHK = "FAIL: delta (" $1 - FN "fn) from previous SID UPDATE (@" FN ") too small " DELTA "ms < " U_MIN "ms."
					ERR++
				}
			}
			if ("FIRST" == TYPE) {
				if (DELTA > F_MAX) {
					CHK = "FAIL: delta (" $1 - FN "fn) from previous SID FIRST (@" FN ") too big " DELTA "ms > " F_MAX "ms."
					ERR++
				}
				if ("UPDATE" == $2 && DELTA < F_MIN) {
					CHK = "FAIL: delta (" $1 - FN "fn) from previous SID UPDATE (@" FN ") too small " DELTA "ms < " F_MIN "ms."
					ERR++
				}
			}
		}
		if ("FACCH" == TYPE && "FIRST" != $2 && "FACCH" != $2 && 1 == SILENCE) { # check FACCH handling
			CHK = "FAIL: incorrect silence resume with " $2 " after FACCH."
			ERR++
		}
	}
	if ("SPEECH" == $2 || "ONSET" == $2) { # talkspurt
		SILENCE = 0
	}
	if ("UPDATE" == $2 || "FIRST" == $2) { # silence
		SILENCE = 1
	}
	print $1, $2, CHK
	if ($2 != "EMPTY") { # skip over EMPTY records
		TYPE = $2
		FN = $1
	}
}

END {
	print "Check completed: found " ERR " errors (" FORCE " events inserted by DSP) in " NR " records."
}
