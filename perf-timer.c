#include "perf-timer.h"

#include "jevents/rdpmc.h"
#include "jevents/jevents.h"

#include <stdio.h>
#include <string.h>
#include <linux/perf_event.h>

typedef struct {
    const char *name, *short_name;
    const char *event_string;
} event;

    const event ALL_EVENTS[] = { //123456789012
    { "cpu_clk_unhalted.thread_p"   , "CYCLES"       , "cpu/event=0x3c/"           } ,
    { "hw_interrupts.received"      , "INTERRUPTS"   , "cpu/config=0x1cb/"         } ,
    { "l2_rqsts.references"         , "L2.ALL"       , "cpu/umask=0xFF,event=0x24/"} ,
    { "l2_rqsts.all_rfo"            , "L2.RFO_ALL"   , "cpu/umask=0xE2,event=0x24/"} ,
    { "l2_rqsts.rfo_miss"           , "L2.RFO_MISS"  , "cpu/umask=0x22,event=0x24/"} ,
    { "l2_rqsts.miss"               , "L2.ALL_MISS"  , "cpu/umask=0x3F,event=0x24/"} ,
    { "l2_rqsts.all_pf"             , "L2.ALL_PF"    , "cpu/umask=0xF8,event=0x24/"} ,
    { "llc.ref"                     , "LLC.REFS"     , "cpu/umask=0x4F,event=0x2E/"} ,
    { "llc.miss"                    , "LLC.MISS"     , "cpu/umask=0x41,event=0x2E/"} ,
    { "mem_inst_retired.all_stores" , "ALL_STORES"   , "cpu/umask=0x82,event=0xd0/"} ,
    { 0 } // sentinel
};

typedef struct {
    // the associated event
    const event *event;
    // the jevents context object
    struct rdpmc_ctx jevent_ctx;
} event_ctx;

size_t context_count;
event_ctx contexts[MAX_EVENTS];  // programmed events go here

/**
 * Take a perf_event_attr objects and return a string representation suitable
 * for use as an event for perf, or just for display.
 */
void printf_perf_attr(FILE *f, const struct perf_event_attr* attr) {
    char* pmu = resolve_pmu(attr->type);
    fputs(pmu ? pmu : "???", f);

#define APPEND_IF_NZ1(field) APPEND_IF_NZ2(field,field)
#define APPEND_IF_NZ2(name, field) if (attr->field) fprintf(f, "/" #name "=0x%lx, ", (long)attr->field);

    APPEND_IF_NZ1(config);
    APPEND_IF_NZ1(config1);
    APPEND_IF_NZ1(config2);
    APPEND_IF_NZ2(period, sample_period);
    APPEND_IF_NZ1(sample_type);
    APPEND_IF_NZ1(read_format);

    fprintf(f, "/");
}

void print_caps(FILE *f, const struct rdpmc_ctx *ctx) {
    fprintf(f, " caps: R: %d UT: %d ZT: %d index: 0x%u",
        ctx->buf->cap_user_rdpmc, ctx->buf->cap_user_time, ctx->buf->cap_user_time_zero, ctx->buf->index);
}

/* list the events in markdown format */
void list_events() {
    const char *fmt = "| %-27s | %-12s |\n";
    printf(fmt, "Full Name", "Short Name");
    printf(fmt, "-------------------------", "-----------");
    for (const event *e = ALL_EVENTS; e->name; e++) {
        printf(fmt, e->name, e->short_name);
    }
}


void setup_counters(bool verbose, const char *counter_string) {
    for (const event *e = ALL_EVENTS; e->name; e++) {
        if (!strstr(counter_string, e->short_name)) {
            continue;
        }
        fprintf(stderr, "Enabling event %s (%s)\n", e->short_name, e->name);
        struct perf_event_attr attr = {};
        int err = jevent_name_to_attr(e->event_string, &attr);
        if (err) {
            fprintf(stderr, "Unable to resolve event %s - report this as a bug along with your CPU model string\n", e->name);
            fprintf(stderr, "jevents error %2d: %s\n", err, jevent_error_to_string(err));
            fprintf(stderr, "jevents details : %s\n", jevent_get_error_details());
        } else {
            struct rdpmc_ctx ctx = {};
            attr.sample_period = 0;
            if (rdpmc_open_attr(&attr, &ctx, 0)) {
                fprintf(stderr, "Failed to program event '%s'. Resolved to: ", e->name);
                printf_perf_attr(stderr, &attr);
            } else {
                if (verbose) {
                    printf("Resolved and programmed event '%s' to '", e->name);
                    printf_perf_attr(stdout, &attr);
                    print_caps(stdout, &ctx);
                    printf("\n");
                }
                contexts[context_count++] = (event_ctx){e, ctx};
                if (context_count == MAX_EVENTS) {
                    break;
                }
            }
        }
    }
}

void print_counter_headings(const char* format) {
    for (size_t i = 0; i < context_count; i++) {
        printf(format, contexts[i].event->short_name);
    }
}

event_counts read_counters() {
    event_counts ret = {};
    for (size_t i=0; i < context_count; i++) {
        ret.counts[i] = rdpmc_read(&contexts[i].jevent_ctx);
    }
    return ret;
}

size_t num_counters() {
    return context_count;
}

event_counts calc_delta(event_counts before, event_counts after) {
    event_counts ret = {};
    for (size_t i=0; i < MAX_EVENTS; i++) {
        ret.counts[i] = after.counts[i] - before.counts[i];
    }
    return ret;
}
