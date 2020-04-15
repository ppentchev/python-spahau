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

#define _GNU_SOURCE

#include <sys/types.h>

#include <arpa/inet.h>

#include <err.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "spahau.h"
#include "sphhost.h"

static bool
address_to_bytes(const char * const address, uint8_t * const bytes)
{
	const int pnres = inet_pton(AF_INET, address, bytes);
	if (pnres == 0) {
		warnx("Invalid address '%s'", address);
		return false;
	} else if (pnres != 1) {
		warnx("Internal inet_pton() error for '%s'", address);
		return false;
	}
	/* 'bytes' is in network byte order, so we can do this... */
	debug("- converted it to %d.%d.%d.%d\n",
	    bytes[0], bytes[1], bytes[2], bytes[3]);
	return true;
}

bool
sph_pton(const char * const address, uint32_t * const result)
{
	debug("About to convert '%s' into a network-byte-order value\n",
	    address);
	uint8_t ads[4];
	if (!address_to_bytes(address, ads))
		return false;

	*result = (ads[0] << 24) | (ads[1] << 16) | (ads[2] << 8) | ads[3];
	debug("- got %08X\n", *result);
	return true;
}

char *sph_get_hostname(const char *address)
{
	debug("About to convert '%s' to an RBL hostname for '%s'\n",
	    address, rbl_domain);
	uint8_t ads[4];
	if (!address_to_bytes(address, ads))
		return NULL;

	/* We don't really need to reverse it, just build the hostname. */
	char *hostname;
	if (asprintf(&hostname, "%d.%d.%d.%d.%s",
	    ads[3], ads[2], ads[1], ads[0], rbl_domain) == -1) {
		warn("Could not allocate memory for the hostname");
		return NULL;
	}
	debug("built RBL hostname '%s'\n", hostname);
	return hostname;
}
