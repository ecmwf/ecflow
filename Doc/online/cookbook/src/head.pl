#!/usr/bin/env perl
# head.pl
use strict;

my $ECF_PORT=^ECF_PORT:0^;
$ENV{'ECF_PORT'} = "^ECF_PORT:0^"; # port
$ENV{'ECF_NODE'} = "^ECF_NODE:0^"; # host
$ENV{'ECF_NAME'} = "^ECF_NAME:0^"; # task path
$ENV{'ECF_PASS'} = "^ECF_PASS:0^"; # password
$ENV{'ECF_TRYNO'} = "^ECF_TRYNO:0^"; # job number

sub xinit() { system("ecflow_client --init $$"); }
sub xabort() { system("ecflow_client --abort $$"); }
sub xcomplete() { system("ecflow_client --complete $$"); }
sub xmeter($$) { my $name=shift; my $value=shift; 
		 system("ecflow_client --meter $name $value"); }
sub xevent($)  { my $n=shift;    system("ecflow_client --event $n"); }
sub xlabel($$) {  my $name=shift; my $value=shift; 
		  system("ecflow_client --label $name $value"); }
xinit();
eval '
