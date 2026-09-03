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
#include "causal_trace.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

enum {
    CAUSAL_LINE_SIZE = 1024,
    CAUSAL_MAX_RANGES = 32
};

typedef struct {
    uint32_t first;
    uint32_t last;
} CAUSAL_RANGE;

typedef struct {
    FILE *stream;
    VAEG_CAUSAL_TRACE_CONFIG config;
    CAUSAL_RANGE io_ranges[CAUSAL_MAX_RANGES];
    CAUSAL_RANGE memory_ranges[CAUSAL_MAX_RANGES];
    uint32_t io_count;
    uint32_t memory_count;
    uint32_t event_count;
    uint32_t sequence;
    uint32_t step;
    uint32_t ring_next;
    uint32_t ring_count;
    char *ring;
    uint32_t transfer_first;
    uint32_t transfer_last;
    int transfer_valid;
    uint32_t next_request_id;
    uint32_t current_request_id;
    int io_all;
    int memory_all;
    int stop_requested;
    int stop_written;
    char current_class[48];
    char stop_reason[32];
} CAUSAL_STATE;

static CAUSAL_STATE causal_state;

static const char *component_name(uint32_t id) {
    switch (id) {
    case VAEG_CAUSAL_COMPONENT_MAIN_CPU:
        return "main-cpu";
    case VAEG_CAUSAL_COMPONENT_FD_SUBSYSTEM:
        return "fd-subsystem";
    case VAEG_CAUSAL_COMPONENT_DRIVE:
        return "drive";
    case VAEG_CAUSAL_COMPONENT_FDC:
        return "fdc";
    case VAEG_CAUSAL_COMPONENT_MAILBOX:
        return "mailbox";
    default:
        return "unknown";
    }
}

static const char *field_name(uint32_t id) {
    switch (id) {
    case VAEG_CAUSAL_FIELD_REQUEST_PHASE:
        return "request_phase";
    case VAEG_CAUSAL_FIELD_HANDSHAKE_PHASE:
        return "handshake_phase";
    case VAEG_CAUSAL_FIELD_COMMAND_PHASE:
        return "command_phase";
    case VAEG_CAUSAL_FIELD_MOTOR_STATE:
        return "motor_state";
    case VAEG_CAUSAL_FIELD_DRIVE_READY:
        return "drive_ready";
    case VAEG_CAUSAL_FIELD_MEDIA_SENSE:
        return "media_sense";
    case VAEG_CAUSAL_FIELD_RESPONSE_STATUS:
        return "response_status";
    case VAEG_CAUSAL_FIELD_RESPONSE_MAILBOX:
        return "response_mailbox";
    case VAEG_CAUSAL_FIELD_RESPONSE_IRQ:
        return "response_irq";
    case VAEG_CAUSAL_FIELD_COMMAND_QUEUE:
        return "command_queue";
    case VAEG_CAUSAL_FIELD_FDC_LIFECYCLE:
        return "fdc_lifecycle";
    case VAEG_CAUSAL_FIELD_SECTOR_TRANSFER:
        return "sector_transfer";
    default:
        return "unknown";
    }
}

static const char *producer_site_name(uint32_t id) {
    switch (id) {
    case VAEG_CAUSAL_SITE_MAIN_REQUEST_EMITTER:
        return "main_request_emitter";
    case VAEG_CAUSAL_SITE_SUBSYSTEM_REQUEST_ACCEPTOR:
        return "subsystem_request_acceptor";
    case VAEG_CAUSAL_SITE_SUBSYSTEM_REQUEST_CONSUMER:
        return "subsystem_request_consumer";
    case VAEG_CAUSAL_SITE_SUBSYSTEM_COMMAND_PHASE:
        return "subsystem_command_phase";
    case VAEG_CAUSAL_SITE_MOTOR_SETTLE:
        return "motor_settle";
    case VAEG_CAUSAL_SITE_DRIVE_READY:
        return "drive_ready";
    case VAEG_CAUSAL_SITE_MEDIA_SENSE:
        return "media_sense";
    case VAEG_CAUSAL_SITE_RESPONSE_STATUS:
        return "response_status";
    case VAEG_CAUSAL_SITE_RESPONSE_MAILBOX:
        return "response_mailbox";
    case VAEG_CAUSAL_SITE_RESPONSE_CONSUMER:
        return "response_consumer";
    case VAEG_CAUSAL_SITE_RESPONSE_IRQ:
        return "response_irq";
    case VAEG_CAUSAL_SITE_COMMAND_QUEUE:
        return "command_queue";
    case VAEG_CAUSAL_SITE_FDC_ATTEMPT:
        return "fdc_attempt";
    case VAEG_CAUSAL_SITE_FDC_ISSUE:
        return "fdc_issue";
    case VAEG_CAUSAL_SITE_FDC_REJECT:
        return "fdc_reject";
    case VAEG_CAUSAL_SITE_SECTOR_TRANSFER:
        return "sector_transfer";
    case VAEG_CAUSAL_SITE_MAILBOX_ROUTE:
        return "mailbox_route";
    case VAEG_CAUSAL_SITE_MAILBOX_ENQUEUE:
        return "mailbox_enqueue";
    case VAEG_CAUSAL_SITE_MAILBOX_VISIBILITY:
        return "mailbox_visibility";
    case VAEG_CAUSAL_SITE_SUBSYSTEM_DISPATCH:
        return "subsystem_dispatch";
    case VAEG_CAUSAL_SITE_MAILBOX_DEQUEUE:
        return "mailbox_dequeue";
    case VAEG_CAUSAL_SITE_SUBSYSTEM_CALLBACK:
        return "subsystem_callback";
    default:
        return "unknown";
    }
}

static const char *consumer_name(uint32_t id) {
    switch (id) {
    case VAEG_CAUSAL_CONSUMER_NONE:
        return "none";
    case VAEG_CAUSAL_CONSUMER_REQUEST_ACCEPTOR:
        return "request_acceptor";
    case VAEG_CAUSAL_CONSUMER_MAILBOX_ROUTE:
        return "mailbox_route";
    case VAEG_CAUSAL_CONSUMER_MAILBOX_ENQUEUE:
        return "mailbox_enqueue";
    case VAEG_CAUSAL_CONSUMER_MAILBOX_STORAGE:
        return "mailbox_storage";
    case VAEG_CAUSAL_CONSUMER_SUBSYSTEM_SCHEDULER:
        return "subsystem_scheduler";
    case VAEG_CAUSAL_CONSUMER_MAILBOX_DEQUEUE:
        return "mailbox_dequeue";
    case VAEG_CAUSAL_CONSUMER_SUBSYSTEM_CALLBACK:
        return "subsystem_callback";
    case VAEG_CAUSAL_CONSUMER_REQUEST_STATE:
        return "request_state";
    case VAEG_CAUSAL_CONSUMER_RESPONSE_ELIGIBILITY:
        return "response_eligibility";
    default:
        return "unknown";
    }
}

static const char *channel_name(uint32_t id) {
    switch (id) {
    case VAEG_CAUSAL_CHANNEL_NONE:
        return "none";
    case VAEG_CAUSAL_CHANNEL_MAIN_TO_SUBSYSTEM:
        return "main-to-subsystem";
    case VAEG_CAUSAL_CHANNEL_MAIN_MAILBOX:
        return "main-mailbox";
    case VAEG_CAUSAL_CHANNEL_SUBSYSTEM_MAILBOX:
        return "subsystem-mailbox";
    default:
        return "unknown";
    }
}

static const char *mailbox_boundary_name(uint32_t id) {
    switch (id) {
    case VAEG_CAUSAL_MAILBOX_BOUNDARY_REQUEST_ACCEPTED:
        return "REQUEST_ACCEPTED";
    case VAEG_CAUSAL_MAILBOX_BOUNDARY_ROUTE_SELECTED:
        return "ROUTE_SELECTED";
    case VAEG_CAUSAL_MAILBOX_BOUNDARY_ENQUEUE_ATTEMPTED:
        return "MAILBOX_ENQUEUE_ATTEMPTED";
    case VAEG_CAUSAL_MAILBOX_BOUNDARY_ENQUEUE_COMMITTED:
        return "MAILBOX_ENQUEUE_COMMITTED";
    case VAEG_CAUSAL_MAILBOX_BOUNDARY_REQUEST_VISIBLE:
        return "MAILBOX_REQUEST_VISIBLE";
    case VAEG_CAUSAL_MAILBOX_BOUNDARY_SUBSYSTEM_DISPATCHED:
        return "SUBSYSTEM_DISPATCHED";
    case VAEG_CAUSAL_MAILBOX_BOUNDARY_DEQUEUE_ATTEMPTED:
        return "MAILBOX_DEQUEUE_ATTEMPTED";
    case VAEG_CAUSAL_MAILBOX_BOUNDARY_CALLBACK_ENTERED:
        return "CONSUMER_CALLBACK_ENTERED";
    case VAEG_CAUSAL_MAILBOX_BOUNDARY_REQUEST_CONSUMED:
        return "REQUEST_CONSUMED";
    case VAEG_CAUSAL_MAILBOX_BOUNDARY_RESPONSE_ELIGIBLE:
        return "RESPONSE_ELIGIBLE";
    default:
        return "UNKNOWN";
    }
}

static const char *mailbox_predecessor_name(uint32_t id) {
	return id == 0 ? "none" : mailbox_boundary_name(id);
}

static const char *mailbox_reason_name(uint32_t id) {
    switch (id) {
    case VAEG_CAUSAL_MAILBOX_REASON_NONE:
        return "none";
    case VAEG_CAUSAL_MAILBOX_REASON_ACCEPTED:
        return "accepted";
    case VAEG_CAUSAL_MAILBOX_REASON_ROUTED:
        return "routed";
    case VAEG_CAUSAL_MAILBOX_REASON_ATTEMPTED:
        return "attempted";
    case VAEG_CAUSAL_MAILBOX_REASON_COMMITTED:
        return "committed";
    case VAEG_CAUSAL_MAILBOX_REASON_VISIBLE:
        return "visible";
    case VAEG_CAUSAL_MAILBOX_REASON_DISPATCHED:
        return "dispatched";
    case VAEG_CAUSAL_MAILBOX_REASON_DEQUEUE:
        return "dequeue";
    case VAEG_CAUSAL_MAILBOX_REASON_CALLBACK:
        return "callback";
    case VAEG_CAUSAL_MAILBOX_REASON_CONSUMED:
        return "consumed";
    case VAEG_CAUSAL_MAILBOX_REASON_ELIGIBLE:
        return "eligible";
    case VAEG_CAUSAL_MAILBOX_REASON_REJECTED:
        return "rejected";
    case VAEG_CAUSAL_MAILBOX_REASON_SKIPPED:
        return "skipped";
    default:
        return "unknown";
    }
}

static const char *transition_name(uint32_t id) {
    switch (id) {
    case VAEG_CAUSAL_TRANSITION_REQUEST_EMITTED:
        return "REQUEST_EMITTED";
    case VAEG_CAUSAL_TRANSITION_REQUEST_ACCEPTED:
        return "REQUEST_ACCEPTED";
    case VAEG_CAUSAL_TRANSITION_REQUEST_CONSUMED:
        return "REQUEST_CONSUMED";
    case VAEG_CAUSAL_TRANSITION_MOTOR_SETTLE_STARTED:
        return "MOTOR_SETTLE_STARTED";
    case VAEG_CAUSAL_TRANSITION_MOTOR_SETTLE_COMPLETED:
        return "MOTOR_SETTLE_COMPLETED";
    case VAEG_CAUSAL_TRANSITION_DRIVE_READY_CHANGED:
        return "DRIVE_READY_CHANGED";
    case VAEG_CAUSAL_TRANSITION_MEDIA_SENSE_COMPLETED:
        return "MEDIA_SENSE_COMPLETED";
    case VAEG_CAUSAL_TRANSITION_RESPONSE_STATUS_WRITTEN:
        return "RESPONSE_STATUS_WRITTEN";
    case VAEG_CAUSAL_TRANSITION_MAILBOX_RESPONSE_WRITTEN:
        return "MAILBOX_RESPONSE_WRITTEN";
    case VAEG_CAUSAL_TRANSITION_MAILBOX_RESPONSE_CONSUMED:
        return "MAILBOX_RESPONSE_CONSUMED";
    case VAEG_CAUSAL_TRANSITION_IRQ_RESPONSE_ASSERTED:
        return "IRQ_RESPONSE_ASSERTED";
    case VAEG_CAUSAL_TRANSITION_COMMAND_QUEUE_INSERTED:
        return "COMMAND_QUEUE_INSERTED";
    case VAEG_CAUSAL_TRANSITION_FDC_COMMAND_ATTEMPTED:
        return "FDC_COMMAND_ATTEMPTED";
    case VAEG_CAUSAL_TRANSITION_FDC_COMMAND_ISSUED:
        return "FDC_COMMAND_ISSUED";
    case VAEG_CAUSAL_TRANSITION_FDC_COMMAND_REJECTED:
        return "FDC_COMMAND_REJECTED";
    default:
        return "UNKNOWN";
    }
}

static const char *cause_name(uint32_t id) {
    switch (id) {
    case VAEG_CAUSAL_CAUSE_REQUEST:
        return "request";
    case VAEG_CAUSAL_CAUSE_HANDSHAKE:
        return "handshake";
    case VAEG_CAUSAL_CAUSE_SCHEDULER:
        return "scheduler";
    case VAEG_CAUSAL_CAUSE_TIMER:
        return "timer";
    case VAEG_CAUSAL_CAUSE_DRIVE:
        return "drive";
    case VAEG_CAUSAL_CAUSE_MEDIA:
        return "media";
    case VAEG_CAUSAL_CAUSE_COMMAND:
        return "command";
    case VAEG_CAUSAL_CAUSE_DMA:
        return "dma";
    case VAEG_CAUSAL_CAUSE_FDC_RESULT:
        return "fdc-result";
    default:
        return "unknown";
    }
}

static int is_all(const char *text) {
    return (text != NULL) &&
           ((strcmp(text, "all") == 0) || (strcmp(text, "*") == 0));
}

static int parse_number(const char **cursor, uint32_t *value) {
    const char *start = *cursor;
    char *end;
    unsigned long parsed;
    int base = 10;

    if ((start[0] == '0') && ((start[1] == 'x') || (start[1] == 'X'))) {
        base = 16;
        start += 2;
    }
    if (!isxdigit((unsigned char)*start)) {
        return 0;
    }
    parsed = strtoul(start, &end, base);
    if ((end == start) || (parsed > UINT32_MAX)) {
        return 0;
    }
    *value = (uint32_t)parsed;
    *cursor = end;
    return 1;
}

static int parse_ranges(const char *text, CAUSAL_RANGE *ranges, uint32_t *count,
                        int *all) {
    const char *cursor;
    uint32_t first;
    uint32_t last;

    *count = 0;
    *all = 0;
    if ((text == NULL) || (text[0] == '\0')) {
        return 1;
    }
    if (is_all(text)) {
        *all = 1;
        return 1;
    }
    cursor = text;
    while (*cursor != '\0') {
        if ((*count >= CAUSAL_MAX_RANGES) || !parse_number(&cursor, &first)) {
            return 0;
        }
        last = first;
        if (*cursor == '-') {
            cursor++;
            if (!parse_number(&cursor, &last) || (last < first)) {
                return 0;
            }
        }
        ranges[*count].first = first;
        ranges[*count].last = last;
        (*count)++;
        if (*cursor == ',') {
            cursor++;
            if (*cursor == '\0') {
                return 0;
            }
        } else if (*cursor != '\0') {
            return 0;
        }
    }
    return 1;
}

static int in_ranges(uint32_t value, const CAUSAL_RANGE *ranges, uint32_t count,
                     int all) {
    uint32_t index;

    if (all) {
        return 1;
    }
    for (index = 0; index < count; index++) {
        if ((value >= ranges[index].first) && (value <= ranges[index].last)) {
            return 1;
        }
    }
    return 0;
}

static int filter_matches(const char *filter, const char *value) {
    return (filter == NULL) || is_all(filter) ||
           ((value != NULL) && (strcmp(filter, value) == 0));
}

static int event_class_known(const char *event_class) {
    static const char *const classes[] = {
        "cpu_step", "io_read", "io_write", "mem_read", "mem_write",
        "irq_assert", "irq_clear", "irq_accept", "device_schedule", "mailbox",
        "drive_state", "fdc_command", "fdc_position", "sector_transfer", "dma",
        "instruction_fetch_correlation", "state_transition", "mailbox_boundary", "stop"
    };
    size_t index;

    if (event_class == NULL) {
        return 0;
    }
    for (index = 0; index < sizeof(classes) / sizeof(classes[0]); index++) {
        if (strcmp(event_class, classes[index]) == 0) {
            return 1;
        }
    }
    return 0;
}

static void json_text(FILE *stream, const char *text) {
    const unsigned char *cursor =
        (const unsigned char *)((text != NULL) ? text : "");

    fputc('"', stream);
    while (*cursor != '\0') {
        if ((*cursor == '\\') || (*cursor == '"')) {
            fputc('\\', stream);
            fputc(*cursor, stream);
        } else if (*cursor == '\n') {
            fputs("\\n", stream);
        } else if (*cursor == '\r') {
            fputs("\\r", stream);
        } else if (*cursor == '\t') {
            fputs("\\t", stream);
        } else if (*cursor < 0x20) {
            fprintf(stream, "\\u%04x", *cursor);
        } else {
            fputc(*cursor, stream);
        }
        cursor++;
    }
    fputc('"', stream);
}

static int append_text(char *line, size_t capacity, size_t *used,
                       const char *format, ...) {
    va_list arguments;
    int written;

    va_start(arguments, format);
    written = vsnprintf(line + *used, capacity - *used, format, arguments);
    va_end(arguments);
    if ((written < 0) || ((size_t)written >= (capacity - *used))) {
        return 0;
    }
    *used += (size_t)written;
    return 1;
}

static int append_json(char *line, size_t capacity, size_t *used,
                       const char *text) {
    size_t position;
    unsigned char character;

    if (*used + 2 >= capacity) {
        return 0;
    }
    line[(*used)++] = '"';
    for (position = 0; text != NULL && text[position] != '\0'; position++) {
        character = (unsigned char)text[position];
        if ((character == '\\') || (character == '"')) {
            if (*used + 2 >= capacity) {
                return 0;
            }
            line[(*used)++] = '\\';
            line[(*used)++] = (char)character;
        } else if (character == '\n') {
            if (*used + 2 >= capacity) {
                return 0;
            }
            line[(*used)++] = '\\';
            line[(*used)++] = 'n';
        } else if (character == '\r') {
            if (*used + 2 >= capacity) {
                return 0;
            }
            line[(*used)++] = '\\';
            line[(*used)++] = 'r';
        } else if (character == '\t') {
            if (*used + 2 >= capacity) {
                return 0;
            }
            line[(*used)++] = '\\';
            line[(*used)++] = 't';
        } else {
            if (*used + 1 >= capacity) {
                return 0;
            }
            line[(*used)++] = (char)character;
        }
    }
    if (*used + 1 >= capacity) {
        return 0;
    }
    line[(*used)++] = '"';
    line[*used] = '\0';
    return 1;
}

static void retain_line(const char *line) {
    char *slot;

    if (causal_state.config.ring_events != 0) {
        slot = causal_state.ring +
               causal_state.ring_next * CAUSAL_LINE_SIZE;
        strncpy(slot, line, CAUSAL_LINE_SIZE - 1);
        slot[CAUSAL_LINE_SIZE - 1] = '\0';
        causal_state.ring_next =
            (causal_state.ring_next + 1) % causal_state.config.ring_events;
        if (causal_state.ring_count < causal_state.config.ring_events) {
            causal_state.ring_count++;
        }
    } else {
        fputs(line, causal_state.stream);
        fputc('\n', causal_state.stream);
    }
}

static void flush_ring(void) {
    uint32_t index;
    uint32_t first;

    if ((causal_state.stream == NULL) || (causal_state.config.ring_events == 0)) {
        return;
    }
    first = (causal_state.ring_count == causal_state.config.ring_events) ?
                causal_state.ring_next : 0;
    for (index = 0; index < causal_state.ring_count; index++) {
        uint32_t slot = (first + index) % causal_state.config.ring_events;
        fputs(causal_state.ring + slot * CAUSAL_LINE_SIZE, causal_state.stream);
        fputc('\n', causal_state.stream);
    }
}

static void emit_stop(const char *reason) {
    if (causal_state.stop_written || (causal_state.stream == NULL)) {
        return;
    }
    causal_state.stop_written = 1;
    flush_ring();
    fprintf(causal_state.stream,
            "{\"seq\":%u,\"class\":\"stop\",\"step\":%u,"
            "\"reason\":",
            causal_state.sequence++, causal_state.step);
    json_text(causal_state.stream, (reason != NULL) ? reason : "normal");
    fprintf(causal_state.stream, ",\"events\":%u}\n", causal_state.event_count);
    fflush(causal_state.stream);
}

static void request_stop(const char *reason) {
    causal_state.stop_requested = 1;
    strncpy(causal_state.stop_reason, (reason != NULL) ? reason : "normal",
            sizeof(causal_state.stop_reason) - 1);
    causal_state.stop_reason[sizeof(causal_state.stop_reason) - 1] = '\0';
}

static int event_allowed(const char *event_class, const char *actor,
                         const char *device, uint32_t address) {
    if ((causal_state.stream == NULL) || causal_state.stop_requested ||
        !filter_matches(causal_state.config.device_filter, device)) {
        return 0;
    }
    if ((strcmp(event_class, "cpu_step") == 0) &&
        !filter_matches(causal_state.config.cpu_filter, actor)) {
        return 0;
    }
    if ((strcmp(event_class, "io_read") == 0) ||
        (strcmp(event_class, "io_write") == 0)) {
        return in_ranges(address, causal_state.io_ranges, causal_state.io_count,
                         causal_state.io_all);
    }
    if ((strcmp(event_class, "mem_read") == 0) ||
        (strcmp(event_class, "mem_write") == 0)) {
        return in_ranges(address, causal_state.memory_ranges,
                         causal_state.memory_count, causal_state.memory_all);
    }
    return 1;
}

static int event_begin(char *line, size_t capacity, size_t *used,
                       const char *event_class) {
    if ((causal_state.event_count >= causal_state.config.max_events) ||
        !append_text(line, capacity, used, "{\"seq\":%u,\"class\":",
                     causal_state.sequence++)) {
        request_stop("event-limit");
        emit_stop("event-limit");
        return 0;
    }
    if (!append_json(line, capacity, used, event_class)) {
        request_stop("line-overflow");
        emit_stop("line-overflow");
        return 0;
    }
    strncpy(causal_state.current_class, event_class,
            sizeof(causal_state.current_class) - 1);
    causal_state.current_class[sizeof(causal_state.current_class) - 1] = '\0';
    return 1;
}

static void event_finish(char *line) {
    retain_line(line);
    causal_state.event_count++;
    if ((causal_state.config.stop_event != NULL) &&
        (strcmp(causal_state.config.stop_event, "none") != 0) &&
        (strcmp(causal_state.current_class, causal_state.config.stop_event) == 0)) {
        request_stop("stop-event");
    }
    if (causal_state.event_count >= causal_state.config.max_events) {
        request_stop("event-limit");
    }
}

int vaeg_causal_trace_start(FILE *stream,
                            const VAEG_CAUSAL_TRACE_CONFIG *config) {
    if ((stream == NULL) || (config == NULL) || (config->max_events == 0) ||
        (config->max_events > 10000000U) || (config->ring_events > 1000000U)) {
        return 0;
    }
    if ((config->stop_event != NULL) &&
        (strcmp(config->stop_event, "none") != 0) &&
        !event_class_known(config->stop_event)) {
        return 0;
    }
    memset(&causal_state, 0, sizeof(causal_state));
    causal_state.stream = stream;
    causal_state.config = *config;
    if (!parse_ranges(config->io_filter, causal_state.io_ranges,
                      &causal_state.io_count, &causal_state.io_all) ||
        !parse_ranges(config->memory_filter, causal_state.memory_ranges,
                      &causal_state.memory_count, &causal_state.memory_all)) {
        memset(&causal_state, 0, sizeof(causal_state));
        return 0;
    }
    if (config->ring_events != 0) {
        causal_state.ring = (char *)calloc(config->ring_events, CAUSAL_LINE_SIZE);
        if (causal_state.ring == NULL) {
            memset(&causal_state, 0, sizeof(causal_state));
            return 0;
        }
    }
    fputs("{\"schema\":\"vaeg-causal-trace-v1\",\"encoding\":\"jsonl\"}\n",
          stream);
    return 1;
}

int vaeg_causal_trace_write_manifest(FILE *stream,
                                     const VAEG_CAUSAL_TRACE_CONFIG *config) {
    if ((stream == NULL) || (config == NULL) || (config->max_events == 0) ||
        (config->max_events > 10000000U) || (config->ring_events > 1000000U)) {
        return 0;
    }
    fprintf(stream,
            "{\"schema\":\"vaeg-causal-trace-manifest-v1\","
            "\"max_events\":%u,\"ring_events\":%u,\"cpu_filter\":",
            config->max_events, config->ring_events);
    json_text(stream, config->cpu_filter);
    fputs(",\"device_filter\":", stream);
    json_text(stream, config->device_filter);
    fputs(",\"io_filter\":", stream);
    json_text(stream, config->io_filter);
    fputs(",\"memory_filter\":", stream);
    json_text(stream, config->memory_filter);
    fputs(",\"stop_event\":", stream);
    json_text(stream, config->stop_event);
    fputs("}\n", stream);
    return ferror(stream) ? 0 : 1;
}

void vaeg_causal_trace_stop(const char *reason) {
    if (causal_state.stream != NULL) {
        emit_stop(causal_state.stop_requested ? causal_state.stop_reason : reason);
        free(causal_state.ring);
    }
    memset(&causal_state, 0, sizeof(causal_state));
}

int vaeg_causal_trace_active(void) {
    return (causal_state.stream != NULL) && !causal_state.stop_written &&
           !causal_state.stop_requested;
}

int vaeg_causal_trace_stop_requested(void) {
    return causal_state.stop_requested;
}

uint32_t vaeg_causal_trace_event_count(void) {
    return causal_state.event_count;
}

void vaeg_causal_trace_cpu_step(uint32_t step, uint16_t cs, uint16_t ip,
                                uint32_t physical, uint8_t opcode, uint32_t ax,
                                uint32_t bx, uint32_t cx, uint32_t dx, uint32_t si,
                                uint32_t di, uint32_t bp, uint32_t sp, uint32_t es,
                                uint32_t ss, uint32_t ds, uint32_t flags,
                                uint32_t memory_backend) {
    char line[CAUSAL_LINE_SIZE];
    size_t used = 0;

    causal_state.step = step;
    if (causal_state.transfer_valid && (physical >= causal_state.transfer_first) &&
        (physical <= causal_state.transfer_last)) {
        vaeg_causal_trace_named("instruction_fetch_correlation", "main-cpu",
                                "memory", "sector-transfer-destination", physical,
                                opcode, 1);
    }
    if (!event_allowed("cpu_step", "main-cpu", "cpu", physical) ||
        !event_begin(line, sizeof(line), &used, "cpu_step")) {
        return;
    }
    if (!append_text(line, sizeof(line), &used,
                     ",\"step\":%u,\"actor\":\"main-cpu\","
                     "\"device\":\"cpu\",\"phase\":\"execute\","
                     "\"cs\":%u,\"ip\":%u,\"physical\":%u,\"opcode\":%u,"
                     "\"ax\":%u,\"bx\":%u,\"cx\":%u,\"dx\":%u,\"si\":%u,"
                     "\"di\":%u,\"bp\":%u,\"sp\":%u,\"es\":%u,"
                     "\"ss\":%u,\"ds\":%u,\"flags\":%u,\"if\":%u,"
                     "\"memory\":%u",
                     step, cs, ip, physical, opcode, ax, bx, cx, dx, si, di, bp,
                     sp, es, ss, ds, flags, (flags & 0x0200U) ? 1U : 0U,
                     memory_backend) ||
        !append_text(line, sizeof(line), &used, ",\"correlation\":%u}",
                     causal_state.current_request_id)) {
        causal_state.stop_requested = 1;
        emit_stop("line-overflow");
        return;
    }
    event_finish(line);
}

void vaeg_causal_trace_named(const char *event_class, const char *actor,
                             const char *device, const char *phase, uint32_t address,
                             uint32_t value, uint32_t width) {
    char line[CAUSAL_LINE_SIZE];
    size_t used = 0;

    if ((event_class == NULL) || (actor == NULL) || (device == NULL) ||
        (phase == NULL) || !event_allowed(event_class, actor, device, address) ||
        !event_begin(line, sizeof(line), &used, event_class)) {
        return;
    }
    if (!append_text(line, sizeof(line), &used, ",\"step\":%u,\"actor\":",
                     causal_state.step) ||
        !append_json(line, sizeof(line), &used, actor) ||
        !append_text(line, sizeof(line), &used, ",\"device\":") ||
        !append_json(line, sizeof(line), &used, device) ||
        !append_text(line, sizeof(line), &used, ",\"phase\":") ||
        !append_json(line, sizeof(line), &used, phase) ||
        !append_text(line, sizeof(line), &used,
                     ",\"address\":%u,\"value\":%u,\"width\":%u",
                     address, value, width) ||
        !append_text(line, sizeof(line), &used, ",\"correlation\":%u",
                     causal_state.current_request_id) ||
        !append_text(line, sizeof(line), &used, "}")) {
        causal_state.stop_requested = 1;
        emit_stop("line-overflow");
        return;
    }
    event_finish(line);
}

void vaeg_causal_trace_io(const char *phase, const char *actor, uint32_t port,
                          uint32_t value, uint32_t width) {
    vaeg_causal_trace_named(
        (phase != NULL && strcmp(phase, "read") == 0) ? "io_read" : "io_write",
        actor, "io", phase, port, value, width);
}

void vaeg_causal_trace_memory(const char *phase, const char *actor, uint32_t address,
                              uint32_t value, uint32_t width) {
    vaeg_causal_trace_named(
        (phase != NULL && strcmp(phase, "read") == 0) ? "mem_read" : "mem_write",
        actor, "memory", phase, address, value, width);
}

void vaeg_causal_trace_sector_transfer(const char *phase, uint32_t destination,
                                       uint32_t end, uint32_t byte_count,
                                       uint32_t status) {
    if ((destination != UINT32_MAX) && (end != UINT32_MAX)) {
        causal_state.transfer_first = (destination < end) ? destination : end;
        causal_state.transfer_last = (destination < end) ? end : destination;
        causal_state.transfer_valid = 1;
    }
    vaeg_causal_trace_named("sector_transfer", "fdc", "fdd", phase, destination,
                            byte_count, status);
}

uint32_t vaeg_causal_trace_request_begin(uint32_t producer_site_id) {
    uint32_t request_id;

    if (!vaeg_causal_trace_active()) {
        return 0;
    }
    request_id = ++causal_state.next_request_id;
    if (request_id == 0) {
        request_stop("request-id-limit");
        emit_stop("request-id-limit");
        return 0;
    }
    causal_state.current_request_id = request_id;
    vaeg_causal_trace_state_transition(
        VAEG_CAUSAL_COMPONENT_MAIN_CPU, VAEG_CAUSAL_FIELD_REQUEST_PHASE, 0, 1,
        VAEG_CAUSAL_CAUSE_REQUEST, producer_site_id,
        VAEG_CAUSAL_TRANSITION_REQUEST_EMITTED,
        VAEG_CAUSAL_PREDICATE_TRUE);
    return request_id;
}

void vaeg_causal_trace_request_bind(uint32_t request_id) {
    if (vaeg_causal_trace_active()) {
        causal_state.current_request_id = request_id;
    }
}

uint32_t vaeg_causal_trace_request_current(void) {
    return causal_state.current_request_id;
}

void vaeg_causal_trace_state_transition(uint32_t component_id, uint32_t field_id,
                                         uint32_t old_state, uint32_t new_state,
                                         uint32_t cause_id, uint32_t producer_site_id,
                                         uint32_t transition_id, int predicate) {
    char line[CAUSAL_LINE_SIZE];
    size_t used = 0;
    const char *component = component_name(component_id);

    if (!event_allowed("state_transition", component, component, 0) ||
        !event_begin(line, sizeof(line), &used, "state_transition")) {
        return;
    }
    if (!append_text(line, sizeof(line), &used,
                     ",\"step\":%u,\"component\":",
                     causal_state.step) ||
        !append_json(line, sizeof(line), &used, component) ||
        !append_text(line, sizeof(line), &used, ",\"field\":") ||
        !append_json(line, sizeof(line), &used, field_name(field_id)) ||
        !append_text(line, sizeof(line), &used,
                     ",\"old\":%u,\"new\":%u,\"cause\":",
                     old_state, new_state) ||
        !append_json(line, sizeof(line), &used, cause_name(cause_id)) ||
        !append_text(line, sizeof(line), &used, ",\"producer\":") ||
        !append_json(line, sizeof(line), &used, producer_site_name(producer_site_id)) ||
        !append_text(line, sizeof(line), &used,
                     ",\"transition\":") ||
        !append_json(line, sizeof(line), &used, transition_name(transition_id)) ||
        !append_text(line, sizeof(line), &used,
                     ",\"correlation\":%u,\"predicate\":%d}",
                     causal_state.current_request_id, predicate)) {
        causal_state.stop_requested = 1;
        emit_stop("line-overflow");
        return;
    }
    event_finish(line);
}

void vaeg_causal_trace_mailbox_boundary(uint32_t boundary_id,
                                        uint32_t producer_site_id,
                                        uint32_t consumer_id,
                                        uint32_t channel_id,
                                        uint32_t predecessor_id,
                                        int predicate,
                                        uint32_t reason_id) {
    char line[CAUSAL_LINE_SIZE];
    size_t used = 0;

    if (!event_allowed("mailbox_boundary", "mailbox", "fd-subsystem", 0) ||
        !event_begin(line, sizeof(line), &used, "mailbox_boundary")) {
        return;
    }
    if (!append_text(line, sizeof(line), &used,
                     ",\"step\":%u,\"boundary\":",
                     causal_state.step) ||
        !append_json(line, sizeof(line), &used, mailbox_boundary_name(boundary_id)) ||
        !append_text(line, sizeof(line), &used, ",\"producer\":") ||
        !append_json(line, sizeof(line), &used, producer_site_name(producer_site_id)) ||
        !append_text(line, sizeof(line), &used, ",\"consumer\":") ||
        !append_json(line, sizeof(line), &used, consumer_name(consumer_id)) ||
        !append_text(line, sizeof(line), &used, ",\"channel\":") ||
        !append_json(line, sizeof(line), &used, channel_name(channel_id)) ||
        !append_text(line, sizeof(line), &used, ",\"predecessor\":") ||
        !append_json(line, sizeof(line), &used, mailbox_predecessor_name(predecessor_id)) ||
        !append_text(line, sizeof(line), &used,
                     ",\"correlation\":%u,\"predicate\":%d,\"reason\":",
                     causal_state.current_request_id, predicate) ||
        !append_json(line, sizeof(line), &used, mailbox_reason_name(reason_id)) ||
        !append_text(line, sizeof(line), &used, "}")) {
        causal_state.stop_requested = 1;
        emit_stop("line-overflow");
        return;
    }
    event_finish(line);
}
