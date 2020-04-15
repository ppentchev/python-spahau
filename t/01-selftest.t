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

plan tests => 12;

my @cmdstr = ($prog, '-T');
my $cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_isnt_num(0, "'@cmdstr' failed with no addresses specified");
$cmd->stdout_is_eq('', "'@cmdstr' did not produce any output");
$cmd->stderr_isnt_eq('', "'@cmdstr' output some error messages");
usleep(500000);

@cmdstr = ($prog, '-T', '8.8.8.8');
$cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_isnt_num(0, "'@cmdstr' failed with an unknown test address");
$cmd->stdout_is_eq('', "'@cmdstr' did not produce any output");
$cmd->stderr_isnt_eq('', "'@cmdstr' output some error messages");
usleep(500000);

@cmdstr = ($prog, '-T', '127.0.0.1', '127.0.0.2');
$cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_is_num(0, "'@cmdstr' completed successfully");
$cmd->stdout_isnt_eq('', "'@cmdstr' produced some output");
$cmd->stderr_is_eq('', "'@cmdstr' did not output any warnings or errrors");
usleep(500000);

@cmdstr = ($prog, '-v', '-T', '127.0.0.1', '127.0.0.2');
$cmd = Test::Command->new(cmd => \@cmdstr);
$cmd->exit_is_num(0, "'@cmdstr' completed successfully");
$cmd->stdout_isnt_eq('', "'@cmdstr' produced some output");
$cmd->stderr_isnt_eq('', "'@cmdstr' produced some diagnostic output");
usleep(500000);
