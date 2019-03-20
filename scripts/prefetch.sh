#!/bin/bash
source "$(dirname "$0")/common.sh"

export TITLE="Interleaved with Prefetching${NEWLINE}(microcode rev: $MICROCODE)"
export COLS="0 1 3 5 7"
export CLABELS="region size (KiB),interleaved,prefetch fixed,prefetch variable,prefetch both"
export YLABEL="cycles/iteration"

"$PLOT4" interleaved interleaved-pf-fixed interleaved-pf-var interleaved-pf-both
