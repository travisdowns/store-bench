#!/bin/bash
source "$(dirname "$0")/common.sh"

export CPU_COUNTERS="L2.RFO_ALL,L2.RFO_MISS,L2.ALL"
export TITLE="Interleaved Stores w/ Selected Counters${NEWLINE}(microcode rev: $MICROCODE)"
export COLS="0 1"
export COLS2="2 3 4"
export CLABELS="region size (KiB),interleaved"
export YLABEL="cycles/iteration"

"$PLOT1" interleaved
