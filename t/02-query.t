#!/usr/bin/perl

use v5.12;
use strict;
use warnings;

use Time::HiRes qw(usleep);

use Test::More;
use Test::Command;

my $prog = $ENV{TEST_PROG};
if (!defined $prog) {
	BAIL_OUT "No TEST_PROG in the environment";
}

plan tests => 41;

my @cmdstr = ($prog);
my $cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_isnt_num(0, "'@cmdstr' failed with no addresses specified");
$cmd->stdout_is_eq('', "'@cmdstr' did not produce any output");
$cmd->stderr_isnt_eq('', "'@cmdstr' output some error messages");
usleep(500000);

@cmdstr = ($prog, '127.0.0.1');
$cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_is_num(0, "'@cmdstr' completed successfully");
$cmd->stdout_isnt_eq('', "'@cmdstr' produced some output");
$cmd->stdout_like(
    qr{The IP address: 127\.0\.0\.1 is NOT found in the Spamhaus blacklists},
    "'@cmdstr' did not find 127.0.0.1");
$cmd->stdout_unlike(
    qr{The IP address: 127\.0\.0\.1 is found},
    "'@cmdstr' really did not find 127.0.0.1");
$cmd->stdout_unlike(
    qr{The IP address: 127\.0\.0\.2 is},
    "'@cmdstr' did not report anything about 127.0.0.2");
$cmd->stderr_is_eq('', "'@cmdstr' did not output any warnings or errrors");
usleep(500000);

@cmdstr = ($prog, '127.0.0.2');
$cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_is_num(0, "'@cmdstr' completed successfully");
$cmd->stdout_isnt_eq('', "'@cmdstr' produced some output");
$cmd->stdout_unlike(
    qr{The IP address: 127\.0\.0\.2 is NOT found in the Spamhaus blacklists},
    "'@cmdstr' did not NOT find 127.0.0.2");
$cmd->stdout_like(
    qr{The IP address: 127\.0\.0\.2 is found.*'127\.0\.0\.2 - SBL - Spamhaus SBL Data'},
    "'@cmdstr' found 127.0.0.2 in the Spamhaus SBL data list");
$cmd->stdout_like(
    qr{The IP address: 127\.0\.0\.2 is found.*'127\.0\.0\.4 - XBL - CBL Data'},
    "'@cmdstr' found 127.0.0.2 in the Spamhaus CBL data list");
$cmd->stdout_like(
    qr{The IP address: 127\.0\.0\.2 is found.*'127\.0\.0\.10 - PBL - ISP Maintained'},
    "'@cmdstr' found 127.0.0.2 in the ISP-maintained list");
$cmd->stdout_unlike(
    qr{The IP address: 127\.0\.0\.1 is},
    "'@cmdstr' did not report anything about 127.0.0.1");
$cmd->stderr_is_eq('', "'@cmdstr' did not output any warnings or errrors");
usleep(500000);

@cmdstr = ($prog, '127.0.0.1', '127.0.0.2');
$cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_is_num(0, "'@cmdstr' completed successfully");
$cmd->stdout_isnt_eq('', "'@cmdstr' produced some output");
$cmd->stdout_like(
    qr{The IP address: 127\.0\.0\.1 is NOT found in the Spamhaus blacklists},
    "'@cmdstr' did not find 127.0.0.1");
$cmd->stdout_unlike(
    qr{The IP address: 127\.0\.0\.1 is found},
    "'@cmdstr' really did not find 127.0.0.1");
$cmd->stdout_unlike(
    qr{The IP address: 127\.0\.0\.2 is NOT found in the Spamhaus blacklists},
    "'@cmdstr' did not NOT find 127.0.0.2");
$cmd->stdout_like(
    qr{The IP address: 127\.0\.0\.2 is found.*'127\.0\.0\.2 - SBL - Spamhaus SBL Data'},
    "'@cmdstr' found 127.0.0.2 in the Spamhaus SBL data list");
$cmd->stdout_like(
    qr{The IP address: 127\.0\.0\.2 is found.*'127\.0\.0\.4 - XBL - CBL Data'},
    "'@cmdstr' found 127.0.0.2 in the Spamhaus CBL data list");
$cmd->stdout_like(
    qr{The IP address: 127\.0\.0\.2 is found.*'127\.0\.0\.10 - PBL - ISP Maintained'},
    "'@cmdstr' found 127.0.0.2 in the ISP-maintained list");
$cmd->stderr_is_eq('', "'@cmdstr' did not output any warnings or errrors");
usleep(500000);

@cmdstr = ($prog, '-v', '127.0.0.1', '127.0.0.2');
$cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_is_num(0, "'@cmdstr' completed successfully");
$cmd->stdout_isnt_eq('', "'@cmdstr' produced some output");
$cmd->stdout_like(
    qr{The IP address: 127\.0\.0\.1 is NOT found in the Spamhaus blacklists},
    "'@cmdstr' did not find 127.0.0.1");
$cmd->stdout_unlike(
    qr{The IP address: 127\.0\.0\.1 is found},
    "'@cmdstr' really did not find 127.0.0.1");
$cmd->stdout_unlike(
    qr{The IP address: 127\.0\.0\.2 is NOT found in the Spamhaus blacklists},
    "'@cmdstr' did not NOT find 127.0.0.2");
$cmd->stdout_like(
    qr{The IP address: 127\.0\.0\.2 is found.*'127\.0\.0\.2 - SBL - Spamhaus SBL Data'},
    "'@cmdstr' found 127.0.0.2 in the Spamhaus SBL data list");
$cmd->stdout_like(
    qr{The IP address: 127\.0\.0\.2 is found.*'127\.0\.0\.4 - XBL - CBL Data'},
    "'@cmdstr' found 127.0.0.2 in the Spamhaus CBL data list");
$cmd->stdout_like(
    qr{The IP address: 127\.0\.0\.2 is found.*'127\.0\.0\.10 - PBL - ISP Maintained'},
    "'@cmdstr' found 127.0.0.2 in the ISP-maintained list");
$cmd->stderr_isnt_eq('', "'@cmdstr' produced some diagnostic output");
usleep(500000);

@cmdstr = ($prog, '-d', 'nosuchsbl.ringlet.net', '127.0.0.2');
$cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_is_num(0, "'@cmdstr' completed successfully");
$cmd->stdout_isnt_eq('', "'@cmdstr' produced some output");
$cmd->stdout_like(
    qr{The IP address: 127\.0\.0\.2 is NOT found in the Spamhaus blacklists},
    "'@cmdstr' did not find 127.0.0.1");
$cmd->stdout_unlike(
    qr{The IP address: 127\.0\.0\.2 is found},
    "'@cmdstr' really did not find 127.0.0.2");
$cmd->stdout_unlike(
    qr{The IP address: 127\.0\.0\.1 is},
    "'@cmdstr' did not report anything about 127.0.0.1");
$cmd->stderr_is_eq('', "'@cmdstr' did not output any warnings or errrors");
usleep(500000);
