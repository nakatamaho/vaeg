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
#include "compiler.h"
#include "timemng.h"
#include <string.h>

static BOOL seed_active;
static _SYSTIME calendar_seed;

BOOL timemng_parse_seed(const char *text, _SYSTIME *systime) {
	static const UINT offsets[6] = {0, 5, 8, 11, 14, 17};
	static const UINT lengths[6] = {4, 2, 2, 2, 2, 2};
	static const UINT month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	static const UINT weekday_offsets[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
	UINT fields[6] = {0, 0, 0, 0, 0, 0};
	UINT i, j, year, limit;
	_SYSTIME candidate;

	if ((text == NULL) || (systime == NULL) || (strlen(text) != 19) || (text[4] != '-') ||
	    (text[7] != '-') || (text[10] != 'T') || (text[13] != ':') || (text[16] != ':')) {
		return (FAILURE);
	}
	for (i = 0; i < 6; i++) {
		for (j = 0; j < lengths[i]; j++) {
			char digit = text[offsets[i] + j];
			if ((digit < '0') || (digit > '9')) {
				return (FAILURE);
			}
			fields[i] = fields[i] * 10 + (UINT)(digit - '0');
		}
	}
	if ((fields[0] < 1980) || (fields[0] > 2079) || (fields[1] < 1) || (fields[1] > 12) ||
	    (fields[2] < 1) || (fields[3] > 23) || (fields[4] > 59) || (fields[5] > 59)) {
		return (FAILURE);
	}
	year = fields[0];
	limit = month_days[fields[1] - 1];
	if ((fields[1] == 2) && ((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0))) {
		limit++;
	}
	if (fields[2] > limit) {
		return (FAILURE);
	}
	memset(&candidate, 0, sizeof(candidate));
	candidate.year = (UINT16)fields[0];
	candidate.month = (UINT16)fields[1];
	candidate.day = (UINT16)fields[2];
	candidate.hour = (UINT16)fields[3];
	candidate.minute = (UINT16)fields[4];
	candidate.second = (UINT16)fields[5];
	if (fields[1] < 3) {
		year--;
	}
	candidate.week = (UINT16)((year + year / 4 - year / 100 + year / 400 +
	                           weekday_offsets[fields[1] - 1] + fields[2]) %
	                          7);
	*systime = candidate;
	return (SUCCESS);
}

BOOL timemng_set_seed(const char *text) {
	_SYSTIME candidate;
	if (text == NULL) {
		seed_active = FALSE;
		return (SUCCESS);
	}
	if (timemng_parse_seed(text, &candidate) != SUCCESS) {
		return (FAILURE);
	}
	calendar_seed = candidate;
	seed_active = TRUE;
	return (SUCCESS);
}

BOOL timemng_seed_active(void) {
	return (seed_active);
}

BOOL timemng_gettime(_SYSTIME *systime) {
	time_t long_time;
	struct tm *now_time;

	if (seed_active) {
		*systime = calendar_seed;
		return (SUCCESS);
	}
	time(&long_time);
	now_time = localtime(&long_time);
	if (now_time != NULL) {
		systime->year = now_time->tm_year + 1900;
		systime->month = now_time->tm_mon + 1;
		systime->week = now_time->tm_wday;
		systime->day = now_time->tm_mday;
		systime->hour = now_time->tm_hour;
		systime->minute = now_time->tm_min;
		systime->second = now_time->tm_sec;
		systime->milli = 0;
		return (SUCCESS);
	}
	return (FAILURE);
}
