# spahau - the trivial Spamhaus RBL client

The `spahau` tool queries the Spamhaus RBL to figure out whether
the specified addresses are listed or not and, if they are, for
what reason. It does that by making DNS type A (IPv4 address)
queries and interpreting the "magic" addresses that the RBL
returns as reasons for blocking the queried-about address.

## Operation

### Figuring out what RBL hostname to query

The `spahau` tool parses the specified address in the dotted-quad
format: a sequence of four numbers separated by dots. It then
reverses the order of these octets and builds a hostname by
appending the RBL domain name. Thus, for `127.0.0.2` specified as
the address to query for, `spahau` will look up an address record
for the 2.0.0.127.zen.spamhaus.org hostname.

### Making a DNS query

The `spahau` tool then proceeds to use the `getaddrinfo()` function
available from most languages' standard libraries. It explicitly
specifies `AF_INET` as the address family, but specifies nothing else
besides the hostname - no port number or service name, no flags.
As a result the `getaddrinfo()` function will make a DNS query for
an "A" record for the Spamhaus RBL hostname corresponding to
the IP address specified.

The result of this query will either be a "not found" error, signalled
in different ways in the different languages' libraries (a return code
of `EAI_NONAME` in C, a `socket.gaierror` exception with its `errno`
field set to `socket.EAI_NONAME` in Python, etc.), or a list of
response structures. The `spahau` tool parses this list and extracts
the IP addresses returned.

### Interpreting the results

The `spahau` tool then proceeds to map the returned IP addresses either
to exact values taken from the Spamhaus documentation, or - if the address
is not among the ones explicitly listed in the documentation - to address
ranges also specified there. It then reports the results on its standard
output stream.

### Handling Spamhaus RBL error responses

Apart from "no name" and address lists, the Spamhaus RBL may also refuse
to answer the query for various reasons. If the `spahau` tool encounters
an address within the special 127.255.255.0/24 range, it will immediately
stop interpreting any other responses returned for the queried address and
reports a failure to obtain a response, since the Spamhaus documentation
explicitly states that such an error must not be interpreted as
an indication that the original address is blacklisted.

## Invoking the tool in other modes

The `spahau` tool may also be invoked with the following command-line
arguments; in all cases it expects one or more IP addresses to be specified
in addition to these flags:

- `-H`: only output the `*.zen.spamhaus.org` hostname to be queried, do not
  actually send any DNS queries

- `-D`: treat the specified address as a response from Spamhaus and display
  the corresponding human-readable description (usually a reason for
  blacklisting the address)

- `-T`: send a query about the specified address, either `127.0.0.1` or
  `127.0.0.2`, to the Spamhaus RBL and check that the response will be
  interpreted correctly - no records returned for `127.0.0.1` and three
  well-known records returned for `127.0.0.2`

In addition to these, the `-v` command-line argument makes `spahau` be
much more verbose and output lots of diagnostic messages to its standard
error stream; this does not affect the text sent to the standard output
stream, so it may still be parsed as usual.

## Running the tests

There are three kinds of tests for the `spahau` utility:

### Self-tests within the `spahau` implementation

Both the C and Python implementations of `spahau` contain a built-in
self-test that queries the Spamhaus RBL for some well-known fictitious
entries meant for testing and verifies the obtained results. These tests
may be run by invoking the `spahau` utility (or the `python/run_spahau.sh`
shell script) with the `-T` command-line option.

### TAP tests

In the source tree, there is a TAP-compatible set of tests written in
Perl in the t/ directory. The `prove` command (usually installed as part
of Perl) and the Perl Test::Command module are needed to run it.

It may be run in two ways:

- for a single implementation: set the `TEST_PROG` environment variable to
  the full path of a single executable file that implements the `spahau`
  utility's command-line interface and run `prove t`

- for all the implementations: run `make test` from the top-level source
  directory; this will build the C implementation, package up the Python one
  just to make sure that some files' syntax is correct, and then run
  the TAP suite for both `c/spahau` and `python/run_spahau.sh`

### Syntax and type checks for the Python implementation

The Python implementation has a `tox.ini` file containing definitions for
testing environments that will run several syntax and type checkers on
the Python source code. This test suite is invoked by running the `tox`
utility (usually available in a package called `tox` or `python3-tox` or
similar) in the `python/` subdirectory where the `tox.ini` file resides.
