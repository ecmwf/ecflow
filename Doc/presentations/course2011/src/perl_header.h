#!/usr/bin/env perl
use strict;
my $ECF_PORT=^ECF_PORT:0^;
my ($svr, $xini, $xabo, $xcom, $xmet, $xeve, $xlab);
if ($ECF_PORT > 0) {
  $ENV{'ECF_PORT'} = "^ECF_PORT:0^"; # port
  $ENV{'ECF_HOST'} = "^ECF_HOST:0^"; # host
  $ENV{'ECF_NAME'} = "^ECF_NAME:0^"; # task path
  $ENV{'ECF_PASS'} = "^ECF_PASS:0^"; # password
  $ENV{'ECF_TRYNO'} = "^ECF_TRYNO:0^"; # job number
  ($svr, $xini, $xabo, $xcom, $xmet, $xeve, $xlab) = 
    ("ecflow_client", "--init", "--abort", "--complete", 
     "--meter", "--event", "--label");
} else {
  $ENV{'SMS_PROG'} = "^SMS_PROG:0^"; # Program Number
  $ENV{'SMSNODE'}  = "^SMSNODE:0^" ; # SMS host
  $ENV{'SMSNAME'}  = "^SMSNAME:0^" ; # task path
  $ENV{'SMSPASS'}  = "^SMSPASS:0^" ; # password
  $ENV{'SMSTRYNO'} = "^SMSTRYNO:0^"; # job number
  ($svr, $xini, $xabo, $xcom, $xmet, $xeve, $xlab) = 
    ("", "smsinit", "smsabort", "smscomplete", 
     "smsmeter", "smsevent", "smslabel"); }
sub xini() { system("$svr $xini $$"); }
sub xabo() { system("$svr $xabo $$"); }
sub xcom() { system("$svr $xcom $$"); }
sub xmet() { my ($n,$v)=@_;   system("$svr $xmet $n $v"); }
sub xeve($) { my $n=shift;    system("$svr $xeve $n"); }
sub xlab() {  my ($n, $v)=@_; system("$svr $xlab $n $v"); }
xini();
eval '
