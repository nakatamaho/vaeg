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
    const char *stop_event;
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
    VAEG_CAUSAL_SITE_SECTOR_TRANSFER = 16
} VAEG_CAUSAL_PRODUCER_SITE_ID;

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
    VAEG_CAUSAL_TRANSITION_FDC_COMMAND_REJECTED = 15
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
void vaeg_causal_trace_io(const char *phase, const char *actor, uint32_t port,
                          uint32_t value, uint32_t width);
void vaeg_causal_trace_memory(const char *phase, const char *actor, uint32_t address,
                              uint32_t value, uint32_t width);
void vaeg_causal_trace_named(const char *event_class, const char *actor,
                             const char *device, const char *phase, uint32_t address,
                             uint32_t value, uint32_t width);
void vaeg_causal_trace_sector_transfer(const char *phase, uint32_t destination,
                                       uint32_t end, uint32_t byte_count,
                                       uint32_t status);
uint32_t vaeg_causal_trace_request_begin(uint32_t producer_site_id);
void vaeg_causal_trace_request_bind(uint32_t request_id);
uint32_t vaeg_causal_trace_request_current(void);
void vaeg_causal_trace_state_transition(uint32_t component_id, uint32_t field_id,
                                         uint32_t old_state, uint32_t new_state,
                                         uint32_t cause_id, uint32_t producer_site_id,
                                         uint32_t transition_id, int predicate);

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
#define vaeg_causal_trace_io(phase, actor, port, value, width) \
    ((void)(phase), (void)(actor), (void)(port), (void)(value), (void)(width))
#define vaeg_causal_trace_memory(phase, actor, address, value, width) \
    ((void)(phase), (void)(actor), (void)(address), (void)(value), (void)(width))
#define vaeg_causal_trace_named(event_class, actor, device, phase, address, value, width) \
    ((void)(event_class), (void)(actor), (void)(device), (void)(phase), (void)(address), \
     (void)(value), (void)(width))
#define vaeg_causal_trace_sector_transfer(phase, destination, end, byte_count, status) \
    ((void)(phase), (void)(destination), (void)(end), (void)(byte_count), (void)(status))
#define vaeg_causal_trace_request_begin(producer_site_id) \
    ((void)(producer_site_id), 0U)
#define vaeg_causal_trace_request_bind(request_id) ((void)(request_id))
#define vaeg_causal_trace_request_current() 0U
#define vaeg_causal_trace_state_transition(component_id, field_id, old_state, new_state, \
                                            cause_id, producer_site_id, transition_id, predicate) \
    ((void)(component_id), (void)(field_id), (void)(old_state), (void)(new_state), \
     (void)(cause_id), (void)(producer_site_id), (void)(transition_id), (void)(predicate))

#endif

#ifdef __cplusplus
}
#endif

#endif
