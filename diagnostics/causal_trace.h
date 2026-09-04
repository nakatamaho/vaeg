/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_DIAGNOSTICS_CAUSAL_TRACE_H
#define VAEG_DIAGNOSTICS_CAUSAL_TRACE_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t max_events;
    uint32_t ring_events;
    const char *cpu_filter;
    const char *device_filter;
    const char *io_filter;
    const char *memory_filter;
    const char *start_event;
    const char *stop_event;
    uint32_t post_stop_events;
    const char *fetch_filter;
    const char *event_filter;
} VAEG_CAUSAL_TRACE_CONFIG;

/* Stable public identifiers for producer/consumer provenance.  These values
 * are independent of source locations, pointers, and guest addresses. */
typedef enum {
    VAEG_CAUSAL_COMPONENT_MAIN_CPU = 1,
    VAEG_CAUSAL_COMPONENT_FD_SUBSYSTEM = 2,
    VAEG_CAUSAL_COMPONENT_DRIVE = 3,
    VAEG_CAUSAL_COMPONENT_FDC = 4,
    VAEG_CAUSAL_COMPONENT_MAILBOX = 5
} VAEG_CAUSAL_COMPONENT_ID;

typedef enum {
    VAEG_CAUSAL_FIELD_REQUEST_PHASE = 1,
    VAEG_CAUSAL_FIELD_HANDSHAKE_PHASE = 2,
    VAEG_CAUSAL_FIELD_COMMAND_PHASE = 3,
    VAEG_CAUSAL_FIELD_MOTOR_STATE = 4,
    VAEG_CAUSAL_FIELD_DRIVE_READY = 5,
    VAEG_CAUSAL_FIELD_MEDIA_SENSE = 6,
    VAEG_CAUSAL_FIELD_RESPONSE_STATUS = 7,
    VAEG_CAUSAL_FIELD_RESPONSE_MAILBOX = 8,
    VAEG_CAUSAL_FIELD_RESPONSE_IRQ = 9,
    VAEG_CAUSAL_FIELD_COMMAND_QUEUE = 10,
    VAEG_CAUSAL_FIELD_FDC_LIFECYCLE = 11,
    VAEG_CAUSAL_FIELD_SECTOR_TRANSFER = 12
} VAEG_CAUSAL_FIELD_ID;

typedef enum {
    VAEG_CAUSAL_SITE_MAIN_REQUEST_EMITTER = 1,
    VAEG_CAUSAL_SITE_SUBSYSTEM_REQUEST_ACCEPTOR = 2,
    VAEG_CAUSAL_SITE_SUBSYSTEM_REQUEST_CONSUMER = 3,
    VAEG_CAUSAL_SITE_SUBSYSTEM_COMMAND_PHASE = 4,
    VAEG_CAUSAL_SITE_MOTOR_SETTLE = 5,
    VAEG_CAUSAL_SITE_DRIVE_READY = 6,
    VAEG_CAUSAL_SITE_MEDIA_SENSE = 7,
    VAEG_CAUSAL_SITE_RESPONSE_STATUS = 8,
    VAEG_CAUSAL_SITE_RESPONSE_MAILBOX = 9,
    VAEG_CAUSAL_SITE_RESPONSE_CONSUMER = 10,
    VAEG_CAUSAL_SITE_RESPONSE_IRQ = 11,
    VAEG_CAUSAL_SITE_COMMAND_QUEUE = 12,
    VAEG_CAUSAL_SITE_FDC_ATTEMPT = 13,
    VAEG_CAUSAL_SITE_FDC_ISSUE = 14,
    VAEG_CAUSAL_SITE_FDC_REJECT = 15,
    VAEG_CAUSAL_SITE_SECTOR_TRANSFER = 16,
    VAEG_CAUSAL_SITE_MAILBOX_ROUTE = 17,
    VAEG_CAUSAL_SITE_MAILBOX_ENQUEUE = 18,
    VAEG_CAUSAL_SITE_MAILBOX_VISIBILITY = 19,
    VAEG_CAUSAL_SITE_SUBSYSTEM_DISPATCH = 20,
    VAEG_CAUSAL_SITE_MAILBOX_DEQUEUE = 21,
    VAEG_CAUSAL_SITE_SUBSYSTEM_CALLBACK = 22,
    VAEG_CAUSAL_SITE_FDC_COMPLETE = 23
} VAEG_CAUSAL_PRODUCER_SITE_ID;

typedef enum {
    VAEG_CAUSAL_CONSUMER_NONE = 0,
    VAEG_CAUSAL_CONSUMER_REQUEST_ACCEPTOR = 1,
    VAEG_CAUSAL_CONSUMER_MAILBOX_ROUTE = 2,
    VAEG_CAUSAL_CONSUMER_MAILBOX_ENQUEUE = 3,
    VAEG_CAUSAL_CONSUMER_MAILBOX_STORAGE = 4,
    VAEG_CAUSAL_CONSUMER_SUBSYSTEM_SCHEDULER = 5,
    VAEG_CAUSAL_CONSUMER_MAILBOX_DEQUEUE = 6,
    VAEG_CAUSAL_CONSUMER_SUBSYSTEM_CALLBACK = 7,
    VAEG_CAUSAL_CONSUMER_REQUEST_STATE = 8,
    VAEG_CAUSAL_CONSUMER_RESPONSE_ELIGIBILITY = 9
} VAEG_CAUSAL_CONSUMER_ID;

typedef enum {
    VAEG_CAUSAL_CHANNEL_NONE = 0,
    VAEG_CAUSAL_CHANNEL_MAIN_TO_SUBSYSTEM = 1,
    VAEG_CAUSAL_CHANNEL_MAIN_MAILBOX = 2,
    VAEG_CAUSAL_CHANNEL_SUBSYSTEM_MAILBOX = 3
} VAEG_CAUSAL_CHANNEL_ID;

typedef enum {
    VAEG_CAUSAL_MAILBOX_BOUNDARY_REQUEST_ACCEPTED = 1,
    VAEG_CAUSAL_MAILBOX_BOUNDARY_ROUTE_SELECTED = 2,
    VAEG_CAUSAL_MAILBOX_BOUNDARY_ENQUEUE_ATTEMPTED = 3,
    VAEG_CAUSAL_MAILBOX_BOUNDARY_ENQUEUE_COMMITTED = 4,
    VAEG_CAUSAL_MAILBOX_BOUNDARY_REQUEST_VISIBLE = 5,
    VAEG_CAUSAL_MAILBOX_BOUNDARY_SUBSYSTEM_DISPATCHED = 6,
    VAEG_CAUSAL_MAILBOX_BOUNDARY_DEQUEUE_ATTEMPTED = 7,
    VAEG_CAUSAL_MAILBOX_BOUNDARY_CALLBACK_ENTERED = 8,
    VAEG_CAUSAL_MAILBOX_BOUNDARY_REQUEST_CONSUMED = 9,
    VAEG_CAUSAL_MAILBOX_BOUNDARY_RESPONSE_ELIGIBLE = 10
} VAEG_CAUSAL_MAILBOX_BOUNDARY_ID;

typedef enum {
    VAEG_CAUSAL_MAILBOX_REASON_NONE = 0,
    VAEG_CAUSAL_MAILBOX_REASON_ACCEPTED = 1,
    VAEG_CAUSAL_MAILBOX_REASON_ROUTED = 2,
    VAEG_CAUSAL_MAILBOX_REASON_ATTEMPTED = 3,
    VAEG_CAUSAL_MAILBOX_REASON_COMMITTED = 4,
    VAEG_CAUSAL_MAILBOX_REASON_VISIBLE = 5,
    VAEG_CAUSAL_MAILBOX_REASON_DISPATCHED = 6,
    VAEG_CAUSAL_MAILBOX_REASON_DEQUEUE = 7,
    VAEG_CAUSAL_MAILBOX_REASON_CALLBACK = 8,
    VAEG_CAUSAL_MAILBOX_REASON_CONSUMED = 9,
    VAEG_CAUSAL_MAILBOX_REASON_ELIGIBLE = 10,
    VAEG_CAUSAL_MAILBOX_REASON_REJECTED = 11,
    VAEG_CAUSAL_MAILBOX_REASON_SKIPPED = 12
} VAEG_CAUSAL_MAILBOX_REASON_ID;

typedef enum {
    VAEG_CAUSAL_TRANSITION_REQUEST_EMITTED = 1,
    VAEG_CAUSAL_TRANSITION_REQUEST_ACCEPTED = 2,
    VAEG_CAUSAL_TRANSITION_REQUEST_CONSUMED = 3,
    VAEG_CAUSAL_TRANSITION_MOTOR_SETTLE_STARTED = 4,
    VAEG_CAUSAL_TRANSITION_MOTOR_SETTLE_COMPLETED = 5,
    VAEG_CAUSAL_TRANSITION_DRIVE_READY_CHANGED = 6,
    VAEG_CAUSAL_TRANSITION_MEDIA_SENSE_COMPLETED = 7,
    VAEG_CAUSAL_TRANSITION_RESPONSE_STATUS_WRITTEN = 8,
    VAEG_CAUSAL_TRANSITION_MAILBOX_RESPONSE_WRITTEN = 9,
    VAEG_CAUSAL_TRANSITION_MAILBOX_RESPONSE_CONSUMED = 10,
    VAEG_CAUSAL_TRANSITION_IRQ_RESPONSE_ASSERTED = 11,
    VAEG_CAUSAL_TRANSITION_COMMAND_QUEUE_INSERTED = 12,
    VAEG_CAUSAL_TRANSITION_FDC_COMMAND_ATTEMPTED = 13,
    VAEG_CAUSAL_TRANSITION_FDC_COMMAND_ISSUED = 14,
    VAEG_CAUSAL_TRANSITION_FDC_COMMAND_REJECTED = 15,
    VAEG_CAUSAL_TRANSITION_FDC_COMMAND_COMPLETED = 16
} VAEG_CAUSAL_TRANSITION_ID;

typedef enum {
    VAEG_CAUSAL_CAUSE_REQUEST = 1,
    VAEG_CAUSAL_CAUSE_HANDSHAKE = 2,
    VAEG_CAUSAL_CAUSE_SCHEDULER = 3,
    VAEG_CAUSAL_CAUSE_TIMER = 4,
    VAEG_CAUSAL_CAUSE_DRIVE = 5,
    VAEG_CAUSAL_CAUSE_MEDIA = 6,
    VAEG_CAUSAL_CAUSE_COMMAND = 7,
    VAEG_CAUSAL_CAUSE_DMA = 8,
    VAEG_CAUSAL_CAUSE_FDC_RESULT = 9
} VAEG_CAUSAL_CAUSE_ID;

typedef enum {
    VAEG_CAUSAL_PREDICATE_NOT_OBSERVABLE = -1,
    VAEG_CAUSAL_PREDICATE_FALSE = 0,
    VAEG_CAUSAL_PREDICATE_TRUE = 1,
    VAEG_CAUSAL_PREDICATE_NOT_PRODUCED = 2
} VAEG_CAUSAL_PREDICATE;

#ifdef VAEG_Z80_COMPAT_INTEGRATION_TRACE

int vaeg_causal_trace_start(FILE *stream, const VAEG_CAUSAL_TRACE_CONFIG *config);
void vaeg_causal_trace_stop(const char *reason);
int vaeg_causal_trace_active(void);
int vaeg_causal_trace_stop_requested(void);
uint32_t vaeg_causal_trace_event_count(void);
int vaeg_causal_trace_write_manifest(FILE *stream,
                                     const VAEG_CAUSAL_TRACE_CONFIG *config);

void vaeg_causal_trace_cpu_step(uint32_t step, uint16_t cs, uint16_t ip,
                                uint32_t physical, uint8_t opcode, uint32_t ax,
                                uint32_t bx, uint32_t cx, uint32_t dx, uint32_t si,
                                uint32_t di, uint32_t bp, uint32_t sp, uint32_t es,
                                uint32_t ss, uint32_t ds, uint32_t flags,
                                uint32_t memory_backend);
void vaeg_causal_trace_subsystem_cpu_step(uint16_t pc, uint16_t next_pc,
                                          uint8_t opcode, uint32_t af,
                                          uint32_t bc, uint32_t de, uint32_t hl,
                                          uint32_t sp, uint32_t ix, uint32_t iy,
                                          uint32_t iff1, uint32_t iff2,
                                          uint32_t interrupt_mode);
void vaeg_causal_trace_io(const char *phase, const char *actor, uint32_t port,
                          uint32_t value, uint32_t width);
void vaeg_causal_trace_memory(const char *phase, const char *actor, uint32_t address,
                              uint32_t value, uint32_t width);
void vaeg_causal_trace_named(const char *event_class, const char *actor,
                             const char *device, const char *phase, uint32_t address,
                             uint32_t value, uint32_t width);
void vaeg_causal_trace_sector_buffer_ready(uint32_t drive, uint32_t byte_count,
                                           uint32_t status);
void vaeg_causal_trace_sector_transfer(const char *phase, uint32_t destination,
                                       uint32_t end, uint32_t byte_count,
                                       uint32_t status);
uint32_t vaeg_causal_trace_request_begin(uint32_t producer_site_id);
void vaeg_causal_trace_request_bind(uint32_t request_id);
uint32_t vaeg_causal_trace_request_current(void);
uint32_t vaeg_causal_trace_request_active(void);
void vaeg_causal_trace_state_transition(uint32_t component_id, uint32_t field_id,
                                         uint32_t old_state, uint32_t new_state,
                                         uint32_t cause_id, uint32_t producer_site_id,
                                         uint32_t transition_id, int predicate);
void vaeg_causal_trace_mailbox_boundary(uint32_t boundary_id,
                                        uint32_t producer_site_id,
                                        uint32_t consumer_id,
                                        uint32_t channel_id,
                                        uint32_t predecessor_id,
                                        int predicate,
                                        uint32_t reason_id);

#else

#define vaeg_causal_trace_start(stream, config) ((void)(stream), (void)(config), 0)
#define vaeg_causal_trace_stop(reason) ((void)(reason))
#define vaeg_causal_trace_active() 0
#define vaeg_causal_trace_stop_requested() 0
#define vaeg_causal_trace_event_count() 0U
#define vaeg_causal_trace_write_manifest(stream, config) \
    ((void)(stream), (void)(config), 0)
#define vaeg_causal_trace_cpu_step(step, cs, ip, physical, opcode, ax, bx, cx, dx, si, di, bp, sp, es, ss, ds, flags, memory_backend) \
    ((void)(step), (void)(cs), (void)(ip), (void)(physical), (void)(opcode), (void)(ax), \
     (void)(bx), (void)(cx), (void)(dx), (void)(si), (void)(di), (void)(bp), (void)(sp), \
     (void)(es), (void)(ss), (void)(ds), (void)(flags), (void)(memory_backend))
#define vaeg_causal_trace_subsystem_cpu_step(pc, next_pc, opcode, af, bc, de, hl, sp, ix, iy, iff1, iff2, interrupt_mode) \
    ((void)(pc), (void)(next_pc), (void)(opcode), (void)(af), (void)(bc), (void)(de), \
     (void)(hl), (void)(sp), (void)(ix), (void)(iy), (void)(iff1), (void)(iff2), \
     (void)(interrupt_mode))
#define vaeg_causal_trace_io(phase, actor, port, value, width) \
    ((void)(phase), (void)(actor), (void)(port), (void)(value), (void)(width))
#define vaeg_causal_trace_memory(phase, actor, address, value, width) \
    ((void)(phase), (void)(actor), (void)(address), (void)(value), (void)(width))
#define vaeg_causal_trace_named(event_class, actor, device, phase, address, value, width) \
    ((void)(event_class), (void)(actor), (void)(device), (void)(phase), (void)(address), \
     (void)(value), (void)(width))
#define vaeg_causal_trace_sector_buffer_ready(drive, byte_count, status) \
    ((void)(drive), (void)(byte_count), (void)(status))
#define vaeg_causal_trace_sector_transfer(phase, destination, end, byte_count, status) \
    ((void)(phase), (void)(destination), (void)(end), (void)(byte_count), (void)(status))
#define vaeg_causal_trace_request_begin(producer_site_id) \
    ((void)(producer_site_id), 0U)
#define vaeg_causal_trace_request_bind(request_id) ((void)(request_id))
#define vaeg_causal_trace_request_current() 0U
#define vaeg_causal_trace_request_active() 0U
#define vaeg_causal_trace_state_transition(component_id, field_id, old_state, new_state, \
                                            cause_id, producer_site_id, transition_id, predicate) \
    ((void)(component_id), (void)(field_id), (void)(old_state), (void)(new_state), \
     (void)(cause_id), (void)(producer_site_id), (void)(transition_id), (void)(predicate))
#define vaeg_causal_trace_mailbox_boundary(boundary_id, producer_site_id, consumer_id, \
                                           channel_id, predecessor_id, predicate, reason_id) \
    ((void)(boundary_id), (void)(producer_site_id), (void)(consumer_id), \
     (void)(channel_id), (void)(predecessor_id), (void)(predicate), (void)(reason_id))

#endif

#ifdef __cplusplus
}
#endif

#endif
