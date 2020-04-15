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

plan tests => 12;

my @cmdstr = ($prog, '-H');
my $cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_isnt_num(0, "'@cmdstr' failed with no addresses specified");
$cmd->stdout_is_eq('', "'@cmdstr' did not produce any output");
$cmd->stderr_isnt_eq('', "'@cmdstr' output some error messages");

@cmdstr = ($prog, '-H', '127.0.0.1', '8.8.4.4');
$cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_is_num(0, "'@cmdstr' completed successfully");
$cmd->stdout_is_eq(
    "1.0.0.127.zen.spamhaus.org\n4.4.8.8.zen.spamhaus.org\n",
    "'@cmdstr' output the correct hostnames");
$cmd->stderr_is_eq('', "'@cmdstr' did not output any warnings or errrors");

@cmdstr = ($prog, '-v', '-H', '127.0.0.1', '8.8.4.4');
$cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_is_num(0, "'@cmdstr' completed successfully");
$cmd->stdout_is_eq(
    "1.0.0.127.zen.spamhaus.org\n4.4.8.8.zen.spamhaus.org\n",
    "'@cmdstr' output the correct hostnames");
$cmd->stderr_isnt_eq('', "'@cmdstr' produced some diagnostic output");

@cmdstr = ($prog, '-H', '-d', 'nosuchrbl.ringlet.net', '127.0.0.1', '8.8.4.4');
$cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_is_num(0, "'@cmdstr' completed successfully");
$cmd->stdout_is_eq(
    "1.0.0.127.nosuchrbl.ringlet.net\n4.4.8.8.nosuchrbl.ringlet.net\n",
    "'@cmdstr' output the correct hostnames");
$cmd->stderr_is_eq('', "'@cmdstr' did not output any warnings or errrors");
