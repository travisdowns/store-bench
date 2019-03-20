Various benchmarks mostly for streams of interleaved stores, in support of the blog post [What has your microcode done for you lately?](https://travisdowns.github.io/blog/2019/03/19/random-writes-and-microcode-oh-my.html).

## Building

Currently it only works on Linux, but I am interested in porting it to Windows. It should be enough to run make:

    make

## Usage

Run with no arguments for usage info, as follows:

```
Must provide 1 or 3 arguments

Usage:
	bench TEST_NAME

 TEST_NAME is one of:

 interleaved
	basic interleaved stores (1 fixed 1 variable)
 interleaved-pf-fixed
	interleaved with fixed region prefetch
 interleaved-pf-var
	interleaved with variable region prefetch
 interleaved-pf-both
	interleaved with both region prefetch
 interleaved-u2
	interleaved unrolled by 2x
 interleaved-u4
	interleaved unrolled by 4x
 interleaved-sfenceA
	interleaved with 1 sfence
 interleaved-sfenceB
	interleaved with 1 sfence
 interleaved-sfenceC
	interleaved with 2 sfences
 wrandom1
	single region random stores 
 wrandom1-unroll
	wrandom1 but unrolled and fast/cheaty RNG
 wlinear1
	linear 64B stide writes over one stream
 wlinearHL
	linear with lfence
 wlinearHS
	linear with sfence
 wlinear1-sfence
	linear with sfence
 rlinear1
	linear 64B stride reads over one region
 lcg
	raw LCG test
 pcg
	raw PCG test
```

At a minimum you need to provide the test name from the list above.

If you provide only the test name, default starting and stopping sizes for the region are used (4 KiB to 512 KiB):

    ./bench interleaved

Otherwise, you can provide your starting and stopping points as the 2nd and 3rd arguments, in KiB. The plots in the blog post all use 1 to 100,000 KiB as follows:

    ./bench interleaved 1 100000

Values are rounded up to the next power of two, so 100,000 becomes 131,072.

## Plots

The `/scripts` directory contains a bunch of `.sh` scripts that I use to generate the various plots. In particular, to generate _all_ plots it should be as simple as running the `all.sh` script:

    SUFFIX=new scripts/all.sh

The `SUFFIX` here is appended to each plot name and indicates whether an old or new microcode version was used (the exact microcode revion is also automatically added to the plot title). The output appears in the `/assets` directory.

You can also run any of the individual plots that `all.sh` creates directly, e.g., `scripts/rwrite-1-vs-2.sh` will generate the first plot from the post. My default these individual scripts will pop up an interactive window with the plot, but you can write to a file by setting the `OUTFILE` environment variable. There are a variety of other variables you can set too, for example `STOP=1000 scripts/rwrite-1-vs-2.sh` will change the stopping point to 1000 KiB which results in much faster plot generation. You can take a peek at `all.sh` for some examples or other variables.

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
