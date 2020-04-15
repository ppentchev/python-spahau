#!/usr/bin/perl

use v5.12;
use strict;
use warnings;

use Test::More;
use Test::Command;

my $prog = $ENV{TEST_PROG};
if (!defined $prog) {
	BAIL_OUT "No TEST_PROG in the environment";
}

plan tests => 9;

my @cmdstr = ($prog, '-D');
my $cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_isnt_num(0, "'@cmdstr' failed with no addresses specified");
$cmd->stdout_is_eq('', "'@cmdstr' did not produce any output");
$cmd->stderr_isnt_eq('', "'@cmdstr' output some error messages");

@cmdstr = ($prog, '-D', '127.0.0.2', '127.0.1.102', '8.8.4.4');
$cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_is_num(0, "'@cmdstr' completed successfully");
$cmd->stdout_is_eq(
    "127.0.0.2 - SBL - Spamhaus SBL Data\n".
    "127.0.1.102 - DBL - abused legit spam\n".
    "8.8.4.4 - UNKNOWN - unexpected Spamhaus response\n",
    "'@cmdstr' output the correct hostnames");
$cmd->stderr_is_eq('', "'@cmdstr' did not output any warnings or errrors");

@cmdstr = ($prog, '-D', '-v', '127.0.0.242', '127.0.2.100', '127.255.255.252');
$cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_is_num(0, "'@cmdstr' completed successfully");
$cmd->stdout_is_eq(
    "127.0.0.242 - SBL - Spamhaus IP Blocklists\n".
    "127.0.2.100 - ZRD - Spamhaus Zero Reputation Domains list\n".
    "127.255.255.252 - ERROR - Typing error in DNSBL name\n",
    "'@cmdstr' output the correct hostnames");
$cmd->stderr_isnt_eq('', "'@cmdstr' produced some diagnostic output");
