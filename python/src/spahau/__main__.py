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
"""Main program: parse command-line arguments, do what is requested."""

import argparse
import sys

from typing import Callable, Dict, List, Tuple  # noqa: H301

from spahau import defs
from spahau import query
from spahau import response


ConfigHandler = Callable[[defs.Config, defs.IPAddress], None]


SELFTEST_DATA_DEFS: Dict[str, List[str]] = {
    "127.0.0.1": [],
    "127.0.0.2": ["127.0.0.2", "127.0.0.4", "127.0.0.10"],
}

SELFTEST_DATA = {
    defs.IPAddress.parse(name): [defs.IPAddress.parse(item) for item in value]
    for name, value in SELFTEST_DATA_DEFS.items()
}


def cmd_describe(cfg: defs.Config, address: defs.IPAddress) -> None:
    """Describe the specified RBL return codes."""
    print(response.response_string(cfg, address))


def cmd_show_hostname(cfg: defs.Config, address: defs.IPAddress) -> None:
    """Build and display the hostname for the query."""
    print(query.get_hostname(cfg, address))


def cmd_selftest(cfg: defs.Config, address: defs.IPAddress) -> None:
    """Run a self-test."""
    expected = SELFTEST_DATA.get(address)
    if expected is None:
        sys.exit("Unknown selftest address '{address}'")

    stringified = " ".join(
        f"'{response.response_string(cfg, resp)}'" for resp in expected
    )
    print(
        f"Querying '{address}', expecting {len(expected)} "
        f"responses: {stringified}"
    )

    responses = query.query(cfg, address)
    stringified = " ".join(
        f"'{response.response_string(cfg, resp)}'" for resp in responses
    )
    print(f"...got {len(responses)} responses: {stringified}")
    if responses != expected:
        sys.exit(f"Mismatch for '{address}'")


def cmd_test(cfg: defs.Config, address: defs.IPAddress) -> None:
    """Describe the specified RBL return codes."""
    responses = query.query(cfg, address)
    if not responses:
        print(
            f"The IP address: {address} is NOT found in "
            f"the Spamhaus blacklists."
        )
        return

    if responses[0].is_spamhaus_error:
        print(
            f"Could not obtain a response for {address}: "
            + response.response_string(cfg, responses[0])
        )
        return

    stringified = " ".join(
        f"'{response.response_string(cfg, resp)}'" for resp in responses
    )
    print(
        f"The IP address: {address} is found in the following "
        f"Spamhaus public IP zone: {stringified}"
    )


def parse_arguments() -> Tuple[defs.Config, ConfigHandler]:
    """Parse the command-line arguments."""
    parser = argparse.ArgumentParser(prog="spahau")
    parser.add_argument(
        "--describe",
        "-D",
        action="store_true",
        help="describe the specified RBL return codes/addresses",
    )
    parser.add_argument(
        "--domain",
        "-d",
        type=str,
        default=defs.RBL_DOMAIN,
        help="specify the RBL domain to test against",
    )
    parser.add_argument(
        "--hostname",
        "-H",
        action="store_true",
        help="only output the RBL hostnames, do not send queries",
    )
    parser.add_argument(
        "--selftest",
        "-T",
        action="store_true",
        help="run a self test: try to obtain some expected responses",
    )
    parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        help="verbose operation; display diagnostic output",
    )
    parser.add_argument(
        "addresses",
        type=str,
        nargs="+",
        help="the addresses to query or describe",
    )

    args = parser.parse_args()

    key = (
        ("D" if args.describe else "")
        + ("H" if args.hostname else "")
        + ("T" if args.selftest else "")
    )
    if len(key) > 1:
        sys.exit("At most one of -D, -H, or -T may be specified")
    handler = {
        "D": cmd_describe,
        "H": cmd_show_hostname,
        "T": cmd_selftest,
        "": cmd_test,
    }[key]

    return (
        defs.Config(
            addresses=[defs.IPAddress.parse(item) for item in args.addresses],
            domain=str(args.domain),
            verbose=bool(args.verbose),
        ),
        handler,
    )


def main() -> None:
    """Parse command-line arguments, do cri... err, things."""
    cfg, func = parse_arguments()
    for address in cfg.addresses:
        func(cfg, address)


if __name__ == "__main__":
    main()
