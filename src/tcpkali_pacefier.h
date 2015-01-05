/*
 * Copyright (c) 2014  Machine Zone, Inc.
 * 
 * Original author: Lev Walkin <lwalkin@machinezone.com>
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef TCPKALI_PACEFIER_H
#define TCPKALI_PACEFIER_H

struct pacefier {
    double previous_ts;
};

static inline void
pacefier_init(struct pacefier *p, double now) {
    p->previous_ts = now;
}

/*
 * Get the number of events we can emit now, since we've advanced our time
 * forward a little.
 */
static inline size_t
pacefier_allow(struct pacefier *p, double events_per_second, double now) {
    double elapsed = now - p->previous_ts;
    ssize_t emit_events = elapsed * events_per_second;  /* Implicit rounding */
    if(emit_events > 0)
        return emit_events;
    else
        return 0;
}

/*
 * Record the actually emitted events.
 */
static inline void
pacefier_emitted(struct pacefier *p, double events_per_second, size_t emitted, double now) {
    double elapsed = now - p->previous_ts;
    double emit_events = elapsed * events_per_second;
    /*
     * The number of allowed events is almost always less
     * than what's actually computed, due to rounding.
     * That means that we can't just say that the nearest event
     * should be emitted at exactly (now + 1.0/events_per_second) seconds.
     * We'd need to accomodate the unsent fraction of events that
     * should have been but weren't emitted in the previous step.
     * We do that by not setting previous_ts to now, but by pushing
     * it a little to the past to allow the next pacefier_allow() operation
     * to produce a tiny little bit greater number.
     * Test: if at t1 we were allowed to emit EPS=5 events and emitted 4,
     * that means that at we should record t1-(5-4/5) instead of t1.
     */
    p->previous_ts = now - (emit_events - emitted)/events_per_second;
    /*
     * If the process cannot keep up with the pace, it will result in
     * previous_ts shifting in the past more and more with time.
     * We don't allow more than 5 seconds skew to prevent too sudden
     * bursts of events and to avoid overfilling the integers.
     */
    if((now - p->previous_ts) > 5)
        p->previous_ts = now - 5;
}


#endif  /* TCPKALI_PACEFIER_H */