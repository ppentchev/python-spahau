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

#include <err.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "spahau.h"
#include "sphhost.h"
#include "sphresponse.h"

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

char *
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
