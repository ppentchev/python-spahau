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
"""Query the Spamhaus RBL for the specified address."""

import socket

from typing import List

from spahau import defs
from spahau import response


def get_hostname(cfg: defs.Config, address: defs.IPAddress) -> str:
    """Build the hostname to query."""
    cfg.diag(f"Building the hostname for {address} and {cfg.domain}")
    return address.text_rev + "." + cfg.domain


def query(
    cfg: defs.Config, address: defs.IPAddress
) -> List[response.Response]:
    """Send a query, parse the responses."""
    cfg.diag(f"Query for {address}")
    hostname = get_hostname(cfg, address)
    try:
        resp = socket.getaddrinfo(hostname, None, family=socket.AF_INET)
    except socket.gaierror as err:
        if err.errno != socket.EAI_NONAME:
            raise

        return []

    cfg.diag(f"Response: {[(data[0], data[4]) for data in resp]}")
    result = [defs.IPAddress.parse(data[4][0]) for data in resp]
    errors = [addr for addr in result if addr.is_spamhaus_error]
    if errors:
        return [response.response_desc(errors[0])]

    return [
        response.response_desc(resp)
        for resp in sorted(set(result), key=lambda addr: addr.octets)
    ]
