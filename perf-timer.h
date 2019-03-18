/* simple PMU counting capabilities */
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_EVENTS 8

typedef struct {
    uint64_t counts[MAX_EVENTS];
} event_counts;

void list_events();

void setup_counters(bool verbose, const char *counter_string);

void print_counter_headings(const char *delim);

event_counts read_counters();

/* number of succesfully programmed counters */
size_t num_counters();

event_counts calc_delta(event_counts before, event_counts after);
