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
"""Type and constant definitions for the Spamhaus RBL client."""

import dataclasses
import re
import sys

from typing import List, Tuple, Type  # noqa: H301


RBL_DOMAIN = "zen.spamhaus.org"

RE_IPV4 = re.compile(
    r""" ^
    (?:
        (?: 0 | [1-9][0-9]* ) \.
    ){3}
    (?: 0 | [1-9][0-9]* )
    $ """,
    re.X,
)


@dataclasses.dataclass(frozen=True)
class IPAddress:
    """An IP address in three different representations."""

    text: str
    text_rev: str
    octets: Tuple[int, int, int, int]
    value: int
    is_spamhaus_error: bool

    @classmethod
    def parse(cls: Type["IPAddress"], text: str) -> "IPAddress":
        """Parse a string into an address."""

        try:
            if not RE_IPV4.match(text):
                raise ValueError()

            octets = list(map(int, text.split(".")))
            if len(octets) != 4:
                raise ValueError()
            if any(value < 0 or value > 255 for value in octets):
                raise ValueError()

        except ValueError as err:
            raise ValueError(f"Not a dotted quad: {text}") from err

        value = (
            (octets[0] << 24)
            + (octets[1] << 16)
            + (octets[2] << 8)
            + octets[3]
        )
        return cls(
            text=text,
            text_rev=f"{octets[3]}.{octets[2]}.{octets[1]}.{octets[0]}",
            octets=(octets[0], octets[1], octets[2], octets[3]),
            value=value,
            is_spamhaus_error=octets[:3] == [127, 255, 255],
        )

    def __str__(self) -> str:
        """Provide a human-readable representation."""
        return self.text


@dataclasses.dataclass(frozen=True)
class Config:
    """Configuration for the main program."""

    addresses: List[IPAddress]
    domain: str
    json: bool
    verbose: bool

    def diag(self, msg: str) -> None:
        """Output a diagnostic message if requested."""
        if self.verbose:
            print(msg, file=sys.stderr)
