/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "diagnostics/causal_trace.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_stream(FILE *stream) {
	long size;
	char *buffer;

	if (fflush(stream) != 0 || fseek(stream, 0, SEEK_END) != 0) {
		return NULL;
	}
	size = ftell(stream);
	if (size < 0 || fseek(stream, 0, SEEK_SET) != 0) {
		return NULL;
	}
	buffer = (char *)malloc((size_t)size + 1U);
	if (buffer == NULL) {
		return NULL;
	}
	if (fread(buffer, 1, (size_t)size, stream) != (size_t)size) {
		free(buffer);
		return NULL;
	}
	buffer[size] = '\0';
	return buffer;
}

static int run_chain(const VAEG_CAUSAL_TRACE_CONFIG *config, char **result) {
	FILE *stream;

	stream = tmpfile();
	if (stream == NULL || !vaeg_causal_trace_start(stream, config)) {
		if (stream != NULL) {
			fclose(stream);
		}
		return 0;
	}
	if (vaeg_causal_trace_request_begin(VAEG_CAUSAL_SITE_MAIN_REQUEST_EMITTER) == 0) {
		vaeg_causal_trace_stop("request-allocation-failed");
		fclose(stream);
		return 0;
	}
	vaeg_causal_trace_mailbox_boundary(
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_REQUEST_ACCEPTED,
	    VAEG_CAUSAL_SITE_MAIN_REQUEST_EMITTER,
	    VAEG_CAUSAL_CONSUMER_REQUEST_ACCEPTOR,
	    VAEG_CAUSAL_CHANNEL_MAIN_TO_SUBSYSTEM,
	    0,
	    VAEG_CAUSAL_PREDICATE_TRUE,
	    VAEG_CAUSAL_MAILBOX_REASON_ACCEPTED);
	vaeg_causal_trace_mailbox_boundary(
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_ROUTE_SELECTED,
	    VAEG_CAUSAL_SITE_MAILBOX_ROUTE,
	    VAEG_CAUSAL_CONSUMER_MAILBOX_ROUTE,
	    VAEG_CAUSAL_CHANNEL_MAIN_TO_SUBSYSTEM,
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_REQUEST_ACCEPTED,
	    VAEG_CAUSAL_PREDICATE_TRUE,
	    VAEG_CAUSAL_MAILBOX_REASON_ROUTED);
	vaeg_causal_trace_mailbox_boundary(
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_ENQUEUE_ATTEMPTED,
	    VAEG_CAUSAL_SITE_MAILBOX_ENQUEUE,
	    VAEG_CAUSAL_CONSUMER_MAILBOX_ENQUEUE,
	    VAEG_CAUSAL_CHANNEL_MAIN_MAILBOX,
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_ROUTE_SELECTED,
	    VAEG_CAUSAL_PREDICATE_TRUE,
	    VAEG_CAUSAL_MAILBOX_REASON_ATTEMPTED);
	vaeg_causal_trace_mailbox_boundary(
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_ENQUEUE_COMMITTED,
	    VAEG_CAUSAL_SITE_MAILBOX_ENQUEUE,
	    VAEG_CAUSAL_CONSUMER_MAILBOX_STORAGE,
	    VAEG_CAUSAL_CHANNEL_MAIN_MAILBOX,
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_ENQUEUE_ATTEMPTED,
	    VAEG_CAUSAL_PREDICATE_TRUE,
	    VAEG_CAUSAL_MAILBOX_REASON_COMMITTED);
	vaeg_causal_trace_mailbox_boundary(
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_REQUEST_VISIBLE,
	    VAEG_CAUSAL_SITE_MAILBOX_VISIBILITY,
	    VAEG_CAUSAL_CONSUMER_MAILBOX_STORAGE,
	    VAEG_CAUSAL_CHANNEL_SUBSYSTEM_MAILBOX,
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_ENQUEUE_COMMITTED,
	    VAEG_CAUSAL_PREDICATE_TRUE,
	    VAEG_CAUSAL_MAILBOX_REASON_VISIBLE);
	vaeg_causal_trace_mailbox_boundary(
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_SUBSYSTEM_DISPATCHED,
	    VAEG_CAUSAL_SITE_SUBSYSTEM_DISPATCH,
	    VAEG_CAUSAL_CONSUMER_SUBSYSTEM_SCHEDULER,
	    VAEG_CAUSAL_CHANNEL_MAIN_TO_SUBSYSTEM,
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_REQUEST_VISIBLE,
	    VAEG_CAUSAL_PREDICATE_TRUE,
	    VAEG_CAUSAL_MAILBOX_REASON_DISPATCHED);
	vaeg_causal_trace_mailbox_boundary(
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_DEQUEUE_ATTEMPTED,
	    VAEG_CAUSAL_SITE_MAILBOX_DEQUEUE,
	    VAEG_CAUSAL_CONSUMER_MAILBOX_DEQUEUE,
	    VAEG_CAUSAL_CHANNEL_SUBSYSTEM_MAILBOX,
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_SUBSYSTEM_DISPATCHED,
	    VAEG_CAUSAL_PREDICATE_TRUE,
	    VAEG_CAUSAL_MAILBOX_REASON_DEQUEUE);
	vaeg_causal_trace_mailbox_boundary(
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_CALLBACK_ENTERED,
	    VAEG_CAUSAL_SITE_SUBSYSTEM_CALLBACK,
	    VAEG_CAUSAL_CONSUMER_SUBSYSTEM_CALLBACK,
	    VAEG_CAUSAL_CHANNEL_SUBSYSTEM_MAILBOX,
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_DEQUEUE_ATTEMPTED,
	    VAEG_CAUSAL_PREDICATE_TRUE,
	    VAEG_CAUSAL_MAILBOX_REASON_CALLBACK);
	vaeg_causal_trace_mailbox_boundary(
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_REQUEST_CONSUMED,
	    VAEG_CAUSAL_SITE_SUBSYSTEM_REQUEST_CONSUMER,
	    VAEG_CAUSAL_CONSUMER_REQUEST_STATE,
	    VAEG_CAUSAL_CHANNEL_SUBSYSTEM_MAILBOX,
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_CALLBACK_ENTERED,
	    VAEG_CAUSAL_PREDICATE_TRUE,
	    VAEG_CAUSAL_MAILBOX_REASON_CONSUMED);
	vaeg_causal_trace_mailbox_boundary(
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_RESPONSE_ELIGIBLE,
	    VAEG_CAUSAL_SITE_SUBSYSTEM_CALLBACK,
	    VAEG_CAUSAL_CONSUMER_RESPONSE_ELIGIBILITY,
	    VAEG_CAUSAL_CHANNEL_SUBSYSTEM_MAILBOX,
	    VAEG_CAUSAL_MAILBOX_BOUNDARY_REQUEST_CONSUMED,
	    VAEG_CAUSAL_PREDICATE_TRUE,
	    VAEG_CAUSAL_MAILBOX_REASON_ELIGIBLE);
	vaeg_causal_trace_io("write", "main-cpu", 0x1b4, 1, 1);
	vaeg_causal_trace_named("mailbox", "main-cpu", "fd-subsystem", "write",
	                       0xfd, 0x81, 1);
	vaeg_causal_trace_named("device_schedule", "scheduler", "fd-subsystem",
	                       "execute", 0, 0, 0);
	vaeg_causal_trace_state_transition(
	    VAEG_CAUSAL_COMPONENT_FD_SUBSYSTEM,
	    VAEG_CAUSAL_FIELD_HANDSHAKE_PHASE, 1, 2,
	    VAEG_CAUSAL_CAUSE_HANDSHAKE,
	    VAEG_CAUSAL_SITE_SUBSYSTEM_REQUEST_CONSUMER,
	    VAEG_CAUSAL_TRANSITION_REQUEST_CONSUMED,
	    VAEG_CAUSAL_PREDICATE_TRUE);
	vaeg_causal_trace_named("drive_state", "fdd", "drive", "ready", 0, 1, 1);
	vaeg_causal_trace_state_transition(
	    VAEG_CAUSAL_COMPONENT_DRIVE, VAEG_CAUSAL_FIELD_MOTOR_STATE, 1, 2,
	    VAEG_CAUSAL_CAUSE_TIMER, VAEG_CAUSAL_SITE_MOTOR_SETTLE,
	    VAEG_CAUSAL_TRANSITION_MOTOR_SETTLE_COMPLETED,
	    VAEG_CAUSAL_PREDICATE_TRUE);
	vaeg_causal_trace_state_transition(
	    VAEG_CAUSAL_COMPONENT_FD_SUBSYSTEM,
	    VAEG_CAUSAL_FIELD_RESPONSE_STATUS, 0, 1,
	    VAEG_CAUSAL_CAUSE_FDC_RESULT,
	    VAEG_CAUSAL_SITE_RESPONSE_STATUS,
	    VAEG_CAUSAL_TRANSITION_RESPONSE_STATUS_WRITTEN,
	    VAEG_CAUSAL_PREDICATE_TRUE);
	vaeg_causal_trace_state_transition(
	    VAEG_CAUSAL_COMPONENT_FDC, VAEG_CAUSAL_FIELD_COMMAND_QUEUE, 0, 1,
	    VAEG_CAUSAL_CAUSE_COMMAND, VAEG_CAUSAL_SITE_COMMAND_QUEUE,
	    VAEG_CAUSAL_TRANSITION_COMMAND_QUEUE_INSERTED,
	    VAEG_CAUSAL_PREDICATE_TRUE);
	vaeg_causal_trace_named("fdc_command", "fdc", "fdc", "issued", 0, 0x46, 1);
	vaeg_causal_trace_named("dma", "fdc", "dmac", "transfer", 0x200, 16, 0);
	vaeg_causal_trace_sector_transfer("complete", 0x200, 0x20f, 16, 0);
	vaeg_causal_trace_cpu_step(7, 0, 0x200, 0x205, 0x90, 1, 2, 3, 4, 5, 6, 7,
	                           8, 9, 10, 11, 0x202, 0);
	vaeg_causal_trace_stop("test-complete");
	*result = read_stream(stream);
	fclose(stream);
	return *result != NULL;
}

static int require_text(const char *text, const char *needle) {
	return text != NULL && strstr(text, needle) != NULL;
}

int main(void) {
	VAEG_CAUSAL_TRACE_CONFIG config = {100, 0, NULL, NULL, "all", "all", NULL};
	VAEG_CAUSAL_TRACE_CONFIG bounded = {2, 0, NULL, NULL, NULL, NULL, NULL};
	VAEG_CAUSAL_TRACE_CONFIG ring = {20, 3, NULL, NULL, NULL, NULL, NULL};
	char *first = NULL;
	char *second = NULL;
	char *limited = NULL;
	char *ringed = NULL;
	int result = EXIT_FAILURE;

	if (!run_chain(&config, &first) || !run_chain(&config, &second) ||
	    strcmp(first, second) != 0 || !require_text(first, "\"class\":\"mailbox\"") ||
	    !require_text(first, "\"class\":\"state_transition\"") ||
	    !require_text(first, "\"transition\":\"REQUEST_EMITTED\"") ||
	    !require_text(first, "\"transition\":\"COMMAND_QUEUE_INSERTED\"") ||
	    !require_text(first, "\"class\":\"mailbox_boundary\"") ||
	    !require_text(first, "\"boundary\":\"REQUEST_CONSUMED\"") ||
	    !require_text(first, "\"correlation\":1") ||
	    !require_text(first, "\"class\":\"sector_transfer\"") ||
	    !require_text(first, "\"class\":\"instruction_fetch_correlation\"") ||
	    !require_text(first, "\"class\":\"stop\"") ||
	    !require_text(first, "\"reason\":\"test-complete\"")) {
		fprintf(stderr, "causal chain fixture failed\n%s\n", first != NULL ? first : "(no output)");
		goto cleanup;
	}
	if (!run_chain(&bounded, &limited) || !require_text(limited, "\"reason\":\"event-limit\"")) {
		fprintf(stderr, "causal event limit fixture failed\n%s\n",
		        limited != NULL ? limited : "(no output)");
		goto cleanup;
	}
	if (!run_chain(&ring, &ringed) || !require_text(ringed, "\"class\":\"stop\"")) {
		fprintf(stderr, "causal ring fixture failed\n%s\n", ringed != NULL ? ringed : "(no output)");
		goto cleanup;
	}
	result = EXIT_SUCCESS;

cleanup:
	free(first);
	free(second);
	free(limited);
	free(ringed);
	return result;
}
