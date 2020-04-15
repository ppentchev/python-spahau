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
import dataclasses
import json
import sys

from typing import Any, Callable, Dict, List, Tuple, Union  # noqa: H301

from spahau import defs
from spahau import query
from spahau import response


Result = Union[str, response.Response, List[response.Response]]

ConfigHandler = Callable[[defs.Config, defs.IPAddress], Result]


SELFTEST_DATA_DEFS: Dict[str, List[str]] = {
    "127.0.0.1": [],
    "127.0.0.2": ["127.0.0.2", "127.0.0.4", "127.0.0.10"],
}

SELFTEST_DATA = {
    defs.IPAddress.parse(name): [
        response.response_desc(defs.IPAddress.parse(item)) for item in value
    ]
    for name, value in SELFTEST_DATA_DEFS.items()
}


def cmd_describe(_cfg: defs.Config, address: defs.IPAddress) -> Result:
    """Describe the specified RBL return codes."""
    return response.response_desc(address)


def cmd_show_hostname(cfg: defs.Config, address: defs.IPAddress) -> Result:
    """Build and display the hostname for the query."""
    return query.get_hostname(cfg, address)


def cmd_selftest(cfg: defs.Config, address: defs.IPAddress) -> Result:
    """Run a self-test."""
    expected = SELFTEST_DATA.get(address)
    if expected is None:
        sys.exit(f"Unknown selftest address '{address}'")

    if not cfg.json:
        print(
            f"Querying '{address}', expecting {len(expected)} responses: "
            + " ".join(f"'{resp}'" for resp in expected)
        )

    responses = query.query(cfg, address)
    if not cfg.json:
        print(
            f"...got {len(responses)} responses: "
            + " ".join(f"'{resp}'" for resp in responses)
        )

    if responses != expected:
        sys.exit(f"Mismatch for '{address}'")

    return responses


def cmd_test(cfg: defs.Config, address: defs.IPAddress) -> Result:
    """Describe the specified RBL return codes."""
    return query.query(cfg, address)


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
        "--json", "-j", action="store_true", help="display JSON output"
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
            json=bool(args.json),
            verbose=bool(args.verbose),
        ),
        handler,
    )


def main() -> None:
    """Parse command-line arguments, do cri... err, things."""
    cfg, func = parse_arguments()

    data: Dict[str, Any] = {}
    for address in cfg.addresses:
        value = func(cfg, address)
        cfg.diag(f"{address}: got {value}")
        if cfg.json:
            if not value or isinstance(value, str):
                data[address.text] = value
            elif isinstance(value, response.Response):
                data[address.text] = dataclasses.asdict(value)
            else:
                data[address.text] = [
                    dataclasses.asdict(item) for item in value
                ]
            continue

        if isinstance(value, (str, response.Response)):
            print(value)
            continue

        assert isinstance(value, list)
        if not value:
            print(
                f"The IP address: {address} is NOT found in "
                f"the Spamhaus blacklists."
            )
            continue

        assert all(isinstance(item, response.Response) for item in value)
        if value and value[0].tag == "ERROR":
            print(f"Could not obtain a response for {address}: {value[0]}")
            continue

        stringified = " ".join(f"'{resp}'" for resp in value)
        print(
            f"The IP address: {address} is found in the following "
            f"Spamhaus public IP zone: {stringified}"
        )

    if cfg.json:
        print(json.dumps(data, indent=2))


if __name__ == "__main__":
    main()
