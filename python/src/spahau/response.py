# Copyright (c) 2020  Peter Pentchev <roam@ringlet.net>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
"""Build a response string out of an IPAddress response."""

import dataclasses

from typing import Optional

from spahau import defs


@dataclasses.dataclass(frozen=True)
class Response:
    """A decoded response from the Spamhaus RBL."""

    tag: str
    reason: str
    address: Optional[defs.IPAddress] = None

    def __str__(self) -> str:
        """Provide a human-readable representation."""
        return f"{self.address} - {self.tag} - {self.reason}"


EXACT = {
    "127.0.0.2": Response(tag="SBL", reason="Spamhaus SBL Data"),
    "127.0.0.3": Response(tag="SBL", reason="Spamhaus SBL CSS Data"),
    "127.0.0.4": Response(tag="XBL", reason="CBL Data"),
    "127.0.0.9": Response(tag="SBL", reason="Spamhaus DROP/EDROP Data"),
    "127.0.0.10": Response(tag="PBL", reason="ISP Maintained"),
    "127.0.0.11": Response(tag="PBL", reason="Spamhaus Maintained"),
    "127.0.1.2": Response(tag="DBL", reason="spam domain"),
    "127.0.1.4": Response(tag="DBL", reason="phish domain"),
    "127.0.1.5": Response(tag="DBL", reason="malware domain"),
    "127.0.1.6": Response(tag="DBL", reason="Internet C&C domain"),
    "127.0.1.102": Response(tag="DBL", reason="abused legit spam"),
    "127.0.1.103": Response(
        tag="DBL", reason="abused spammed redirector domain"
    ),
    "127.0.1.104": Response(tag="DBL", reason="abused legit phish"),
    "127.0.1.105": Response(tag="DBL", reason="abused legit malware"),
    "127.0.1.106": Response(tag="DBL", reason="abused legit botnet C&C"),
    "127.0.1.255": Response(tag="DBL", reason="IP queries prohibited!"),
    "127.255.255.252": Response(
        tag="ERROR", reason="Typing error in DNSBL name"
    ),
    "127.255.255.254": Response(
        tag="ERROR", reason="Anonymous query through public resolver"
    ),
    "127.255.255.255": Response(
        tag="ERROR", reason="Excessive number of queries"
    ),
}

DOMAINS = {
    "127.0.0.0": Response(tag="SBL", reason="Spamhaus IP Blocklists"),
    "127.0.1.0": Response(tag="DBL", reason="Spamhaus Domain Blocklists"),
    "127.0.2.0": Response(
        tag="ZRD", reason="Spamhaus Zero Reputation Domains list"
    ),
    "127.255.255.0": Response(
        tag="ERROR", reason="could not obtain a Spamhaus response"
    ),
}


def response_desc(address: defs.IPAddress) -> Response:
    """Return the Spamhaus description of the address."""
    match = EXACT.get(address.text)
    if match is not None:
        return dataclasses.replace(match, address=address)

    octets = address.octets
    match = DOMAINS.get(f"{octets[0]}.{octets[1]}.{octets[2]}.0")
    if match is not None:
        return dataclasses.replace(match, address=address)

    return Response(
        tag="UNKNOWN", reason="unexpected Spamhaus response", address=address
    )
