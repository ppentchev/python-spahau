/**
 * Copyright (c) 2020  Peter Pentchev <roam@ringlet.net>
 * All rights reserved.
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

#include <sys/types.h>
#include <sys/socket.h>

#include <err.h>
#include <inttypes.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#include "spahau.h"
#include "sphhost.h"
#include "sphquery.h"

static int
compare_uint32(const void * const a, const void * const b)
{
	const uint32_t va = *(const uint32_t *)a;
	const uint32_t vb = *(const uint32_t *)b;

	/* Yes, this can be written as (va > vb) - (va < vb)... */
	if (va > vb)
		return 1;
	if (va < vb)
		return -1;
	return 0;
}

static void
sort_uniq(uint32_t * const response)
{
	const uint32_t start_count = response[0];
	qsort(&response[1], start_count, sizeof(response[1]), compare_uint32);

	uint32_t next_pos = 2;
	uint32_t last_value = response[1];
	for (uint32_t idx = 2; idx < response[0]; idx++) {
		if (response[idx] == last_value)
			continue;
		last_value = response[idx];
		response[next_pos] = last_value;
		next_pos++;
	}
	response[0] = next_pos - 1;
}

uint32_t *
query(const char * const address)
{
	debug("About to query %s\n", address);
	char * const hostname = sph_get_hostname(address);

	uint32_t * const response = malloc(RESPONSE_SIZE * sizeof(*response));
	if (response == NULL) {
		warn("Could not allocate memory for the response");
		free(hostname);
		return NULL;
	}

	struct addrinfo hints = { 0 };
	hints.ai_family = AF_INET;
	struct addrinfo *resp;
	const int res = getaddrinfo(hostname, NULL, &hints, &resp);
	if (res == EAI_NONAME) {
		response[0] = 0;
		free(hostname);
		return response;
	}
	if (res != 0) {
		warnx("Could not query '%s': %s", hostname, gai_strerror(res));
		free(hostname);
		return NULL;
	}
	free(hostname);

	size_t pos;
	for (pos = 1; pos < RESPONSE_SIZE && resp != NULL;
	    pos++, resp = resp->ai_next) {
		if (resp->ai_family != AF_INET) {
			warnx(
			    "getaddrinfo() returned a record with "
			    "address family %d instead of %d",
			    resp->ai_family, AF_INET);
			response[0] = 0;
			return response;
		}
		if (resp->ai_addrlen != sizeof(struct sockaddr_in)) {
			warnx(
			    "getaddrinfo() returned a record with "
			    "address length %zu, expected %zu",
			    (size_t)resp->ai_addrlen,
			    sizeof(struct sockaddr_in));
			response[0] = 0;
			return response;
		}
		const uint8_t * const sinaddr = (const uint8_t *)&(((const struct sockaddr_in *)(resp->ai_addr))->sin_addr);
		response[pos] = (sinaddr[0] << 24) | (sinaddr[1] << 16) | (sinaddr[2] << 8) | sinaddr[3];
		debug("- got %08X\n", response[pos]);

		if (IS_SPAMHAUS_ERROR(response[pos])) {
			response[1] = response[pos];
			response[0] = 1;
			debug("only returning the error code");
			return response;
		}
	}
	debug("out of the loop with pos %zu\n", pos);
	response[0] = pos - 1;

	if (response[0] > 1)
		sort_uniq(response);
	return response;
}
