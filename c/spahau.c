/*-
 * Copyright (c) 2020  Peter Pentchev
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

/* For asprintf(3)... */
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/socket.h>

#include <err.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "spahau.h"
#include "sphhost.h"

#define VERSION_STRING	"0.1.0.dev2"

#define RBL_DOMAIN "zen.spamhaus.org"

/* 15 responses at the most + one element for the count. */
#define RESPONSE_SIZE	16

static bool		verbose;

struct selftest_item {
	const char * const address;
	const uint32_t result[RESPONSE_SIZE];
};

static struct selftest_item selftest_data[] = {
	{
		.address = "127.0.0.1",
		.result = {0},
	},
	{
		.address = "127.0.0.2",
		.result = {3, 0x7F000002, 0x7F000004, 0x7F00000A},
	},
};

#define SELFTEST_COUNT	(sizeof(selftest_data) / sizeof(selftest_data[0]))

#define IS_SPAMHAUS_ERROR(resp)	(((resp) & 0xFFFFFF00) == 0x7FFFFF00)

const char *rbl_domain = RBL_DOMAIN;

static void
usage(const bool _ferr)
{
	const char * const s =
	    "Usage:\tspahau [-HNv] [-d rbl.domain] address...\n"
	    "\tspahau [-v] [-d rbl.domain] -T address...\n"
	    "\tspahau -V | -h | --version | --help\n"
	    "\tspahau --features\n"
	    "\n"
	    "\t-d\tspecify the RBL domain to test against (default: "
	    RBL_DOMAIN ")\n"
	    "\t-H\tonly output the RBL hostnames, do not send queries\n"
	    "\t-h\tdisplay program usage information and exit\n"
	    "\t-T\trun a self test: try to obtain some expected responses\n"
	    "\t-V\tdisplay program version information and exit\n"
	    "\t-v\tverbose operation; display diagnostic output\n";

	fprintf(_ferr? stderr: stdout, "%s", s);
	if (_ferr)
		exit(1);
}

static void
version(void)
{
	puts("spahau " VERSION_STRING);
}

static void
features(void)
{
	puts("Features: spahau=" VERSION_STRING);
}

void
debug(const char * const fmt, ...)
{
	va_list v;

	va_start(v, fmt);
	if (verbose)
		vfprintf(stderr, fmt, v);
	va_end(v);
}

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

static uint32_t *
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

static const char *
response_string_desc(uint32_t response)
{
	debug("response_string() invoked for %08X\n", response);

	switch (response) {
		case 0x7F000002:
			return "SBL - Spamhaus SBL Data";

		case 0x7F000003:
			return "SBL - Spamhaus SBL CSS Data";

		case 0x7F000004:
			return "XBL - CBL Data";

		case 0x7F000009:
			return "SBL - Spamhaus DROP/EDROP Data";

		case 0x7F00000A:
			return "PBL - ISP Maintained";

		case 0x7F00000B:
			return "PBL - Spamhaus Maintained";

		/**************/

		case 0x7F000102:
			return "DBL - spam domain";

		case 0x7F000104:
			return "DBL - phish domain";

		case 0x7F000105:
			return "DBL - malware domain";

		case 0x7F000106:
			return "DBL - Internet C&C domain";

		case 0x7F000166:
			return "DBL - abused legit spam";

		case 0x7F000167:
			return "DBL - abused spammed redirector domain";

		case 0x7F000168:
			return "DBL - abused legit phish";

		case 0x7F000169:
			return "DBL - abused legit malware";

		case 0x7F00016A:
			return "DBL - abused legit botnet C&C";

		case 0x7F0001FF:
			return "DBL - IP queries prohibited!";

		/**************/

		case 0x7FFFFFFC:
			return "ERROR - Typing error in DNSBL name";

		case 0x7FFFFFFE:
			return
			    "ERROR - Anonymous query through public resolver";

		case 0x7FFFFFFF:
			return "ERROR - Excessive number of queries";

		default:
			/* good practices and stuff */
			break;
	}

	switch (response & 0xFFFFFF00) {
		case 0x7F000000:
			return "SBL - Spamhaus IP Blocklists";

		case 0x7F000100:
			return "DBL - Spamhaus Domain Blocklists";

		case 0x7F000200:
			return "ZRD - Spamhaus Zero Reputation Domains list";

		case 0x7FFFFF00:
			return "ERROR - could not obtain a Spamhaus response";

		default:
			/* good practices and stuff */
			break;
	}

	return "UNKNOWN - unexpected Spamhaus response";
}

static char *
response_string(const uint32_t response)
{
	char *desc;
	const int res = asprintf(&desc, "%u.%u.%u.%u - %s",
	    response >> 24, (response >> 16) & 0xFF,
	    (response >> 8) & 0xFF, response & 0xFF,
	    response_string_desc(response));
	if (res == -1) {
		warn("Could not allocate memory");
		return NULL;
	}
	return desc;
}

static void
test(const char * const address)
{
	debug("About to check %s\n", address);
	uint32_t * const responses = query(address);
	if (responses == NULL) {
		warnx("Could not obtain a result for '%s'", address);
		return;
	}
	if (responses[0] == 0) {
		printf(
		    "The IP address: %s is NOT found in "
		    "the Spamhaus blacklists.\n", address);
		free(responses);
		return;
	}
	if (responses[0] == 1 && IS_SPAMHAUS_ERROR(responses[1])) {
		char * const resp = response_string(responses[1]);
		printf("Spamhaus returned an error code for %s: %s\n",
		    address, resp);
		free(resp);
		free(responses);
		return;
	}

	printf(
	    "The IP address: %s is found in the following "
	    "Spamhaus public IP zone:",
	    address);
	for (size_t pos = 1; pos <= responses[0]; pos++) {
		char * const resp = response_string(responses[pos]);
		printf(" '%s'", resp);
		free(resp);
	}
	printf("\n");
	free(responses);
}

static void
selftest(const char * const address)
{
	for (size_t idx = 0; idx < SELFTEST_COUNT; idx++) {
		const struct selftest_item * const item = &selftest_data[idx];
		if (strcmp(address, item->address) != 0)
			continue;

		const uint32_t expected_count = item->result[0];
		if (expected_count >= RESPONSE_SIZE)
			errx(1,
			    "Internal error: selftest_data[%zu]: bad count %u",
			    idx, expected_count);
		printf("Querying '%s', expecting %u responses%s",
		    item->address, expected_count,
		    expected_count == 0 ? "" : ":");
		for (size_t resp_idx = 1;
		    resp_idx <= expected_count;
		    resp_idx++) {
			char * const resp =
			    response_string(item->result[resp_idx]);
			printf(" '%s'", resp);
			free(resp);
		}
		printf("\n");

		uint32_t *responses = query(item->address);
		if (responses == NULL)
			errx(1, "Unexpected problem querying '%s'",
			    item->address);
		const uint32_t recv_count = responses[0];
		if (recv_count >= RESPONSE_SIZE)
			errx(1, "Unexpected response count for '%s': %u",
			    item->address, recv_count);
		printf("...got %u responses%s",
		    recv_count, recv_count == 0 ? "" : ":");

		bool mismatch = recv_count != expected_count;
		for (size_t resp_idx = 1; resp_idx <= recv_count; resp_idx++) {
			char * const resp =
			    response_string(responses[resp_idx]);
			printf(" '%s'", resp);
			free(resp);

			if (responses[resp_idx] != item->result[resp_idx])
				mismatch = true;
		}
		printf("\n");
		free(responses);

		if (mismatch)
			errx(1, "Mismatch for %s", item->address);

		return;
	}

	errx(1, "No selftest definition for address '%s'", address);
}

static void
show_hostname(const char * const address)
{
	char * const host = sph_get_hostname(address);
	if (host == NULL)
		return;

	printf("%s\n", host);
	free(host);
}

int
main(int argc, char * const argv[])
{
	bool hflag = false, Vflag = false, show_features = false;
	int ch;
	void (*testfunc)(const char *) = test;

	while (ch = getopt(argc, argv, "d:HhTVv-:"), ch != -1)
		switch (ch) {
			case 'd':
				rbl_domain = optarg;
				break;

			case 'H':
				testfunc = show_hostname;
				break;

			case 'h':
				hflag = true;
				break;

			case 'T':
				testfunc = selftest;
				break;

			case 'V':
				Vflag = true;
				break;

			case 'v':
				verbose = true;
				break;

			case '-':
				if (strcmp(optarg, "help") == 0)
					hflag = true;
				else if (strcmp(optarg, "version") == 0)
					Vflag = true;
				else if (strcmp(optarg, "features") == 0)
					show_features = true;
				else {
					warnx("Invalid long option '%s' specified", optarg);
					usage(true);
				}
				break;

			default:
				usage(1);
				/* NOTREACHED */
		}
	if (Vflag)
		version();
	if (hflag)
		usage(false);
	if (show_features)
		features();
	if (Vflag || hflag || show_features)
		return (0);

	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage(true);

	for (size_t i = 0; i < (size_t)argc; i++)
		testfunc(argv[i]);
	return (0);
}
