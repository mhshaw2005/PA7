#include <stdio.h> 
#include <stdlib.h>
#include "simulator.h"
 
void pageit(Pentry q[MAXPROCESSES]) { 
 
    /* Static vars */
    static int initialized = 0;
    static int tick = 1; // artificial time
    static int timestamps[MAXPROCESSES][MAXPROCPAGES];
    static int outstart[MAXPROCESSES][MAXPROCPAGES]; // tick a pageout began (0 = none)
 
    /* Local vars */
    int proc;
    int page;
 
    /* initialize timestamp array on first run */
    if (!initialized) {
        for (proc=0; proc < MAXPROCESSES; proc++) {
            for (page=0; page < MAXPROCPAGES; page++) {
                timestamps[proc][page] = 0; 
                outstart[proc][page] = 0;
            }
        }
        initialized = 1;
    }
 
    /* Pass 1: record usage and count pageouts still in flight.
     * Inactive slots get their history cleared so a new process reusing
     * the slot doesn't inherit stale data. */
    int pending_out = 0;
    for (proc = 0; proc < MAXPROCESSES; proc++) {
        if (!q[proc].active) {
            for (page = 0; page < MAXPROCPAGES; page++) {
                timestamps[proc][page] = 0;
                outstart[proc][page] = 0;
            }
            continue;
        }
        page = q[proc].pc / PAGESIZE;
        if (page >= 0 && page < MAXPROCPAGES)
            timestamps[proc][page] = tick;
 
        for (page = 0; page < MAXPROCPAGES; page++) {
            if (outstart[proc][page] && tick - outstart[proc][page] > PAGEWAIT + 1)
                outstart[proc][page] = 0;           /* pageout finished */
            if (outstart[proc][page])
                pending_out++;
        }
    }
 
    /* Pass 2: make sure each active process's current page is resident */
    int needed = 0;      /* processes currently waiting for a free frame */
    for (proc = 0; proc < MAXPROCESSES; proc++) {
        if (!q[proc].active) continue;
 
        page = q[proc].pc / PAGESIZE;
        if (page < 0 || page >= MAXPROCPAGES) continue;
 
        if (q[proc].pages[page]) continue;          /* already in memory */
        if (pagein(proc, page)) continue;           /* started / in progress */
        if (outstart[proc][page]) continue;         /* its own pageout still running */
 
        /* pagein failed for lack of frames */
        needed++;
        if (needed <= pending_out) continue;        /* frames already on the way */
 
        /* evict the globally least recently used page not used this tick */
        int vproc = -1, vpage = -1, oldest = tick;
        for (int p = 0; p < MAXPROCESSES; p++) {
            for (int pg = 0; pg < MAXPROCPAGES; pg++) {
                if (!q[p].pages[pg]) continue;      /* not resident */
                if (timestamps[p][pg] < oldest) {
                    oldest = timestamps[p][pg];
                    vproc = p;
                    vpage = pg;
                }
            }
        }
        if (vproc >= 0 && pageout(vproc, vpage)) {
            outstart[vproc][vpage] = tick;
            pending_out++;
        }
    }
 
    /* advance time for next pageit iteration */
    tick++;
} 
