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
#include <stdio.h>

#include "spahau.h"
#include "sphhost.h"

char *sph_get_hostname(const char *address)
{
	debug("About to convert '%s' to an RBL hostname for '%s'\n",
	    address, rbl_domain);
	unsigned char ads[4];
	const int pnres = inet_pton(AF_INET, address, &ads);
	if (pnres == 0) {
		warnx("Invalid address '%s'", address);
		return NULL;
	} else if (pnres != 1) {
		warnx("Internal inet_pton() error for '%s'", address);
		return NULL;
	}
	/* 'ads' is in network byte order, so we can do this... */
	debug("- converted it to %d.%d.%d.%d\n",
	    ads[0], ads[1], ads[2], ads[3]);

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
