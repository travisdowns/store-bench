Tests interleaved stores.

## CPU Counters

We support recording various Intel performance counters using pmu-tools' jevents library. To record events, set them in the `CPU_COUNTERS` environment variable, using the _short name_ for as shown in the supported events table:

| Full Name                   | Short Name   |
| -------------------------   | -----------  |
| cpu_clk_unhalted.thread_p   | CYCLES       |
| hw_interrupts.received      | INTERRUPTS   |
| l2_rqsts.references         | L2_RQSTS.ALL |
| l2_rqsts.all_rfo            | L2.RFO_ALL   |
| l2_rqsts.rfo_miss           | L2.RFO_MISS  |
| l2_rqsts.miss               | L2.ALL_MISS  |
| l2_rqsts.all_pf             | L2.ALL_PF    |
| llc.ref                     | LLC.REFS     |
| llc.miss                    | LLC.MISS     |
| mem_inst_retired.all_stores | ALL_STORES   |
