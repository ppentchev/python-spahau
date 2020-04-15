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

#include <sys/types.h>

#include <err.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "spahau.h"
#include "sphhost.h"
#include "sphresponse.h"
#include "sphquery.h"

#define VERSION_STRING	"0.1.0.dev2"

#define RBL_DOMAIN "zen.spamhaus.org"

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

const char *rbl_domain = RBL_DOMAIN;

static void
usage(const bool _ferr)
{
	const char * const s =
	    "Usage:\tspahau [-DHNv] [-d rbl.domain] address...\n"
	    "\tspahau [-v] [-d rbl.domain] -T address...\n"
	    "\tspahau -V | -h | --version | --help\n"
	    "\tspahau --features\n"
	    "\n"
	    "\t-D\tdescribe the specified RBL return codes/addresses\n"
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

static void
show_response(const char * const address)
{
	uint32_t result;
	if (!sph_pton(address, &result)) {
		warnx("Could not parse '%s'", address);
		return;
	}

	char * const resp = response_string(result);
	if (resp == NULL)
		return;

	puts(resp);
	free(resp);
}

int
main(int argc, char * const argv[])
{
	bool hflag = false, Vflag = false, show_features = false;
	int ch;
	void (*testfunc)(const char *) = test;

	while (ch = getopt(argc, argv, "Dd:HhTVv-:"), ch != -1)
		switch (ch) {
			case 'D':
				testfunc = show_response;
				break;

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
