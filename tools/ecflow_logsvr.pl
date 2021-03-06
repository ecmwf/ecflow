#!/usr/bin/env perl
## Copyright 2009-2017 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

use IO::Socket;
use strict;
use POSIX qw(:sys_wait_h);
use File::Basename;
use Cwd;

my $port = $ENV{LOGPORT};
my $path = $ENV{LOGPATH};
my $map  = $ENV{LOGMAP};

$port = 19999 unless($port);
$path = cwd   unless($path);

my @path = split(":",$path);

# output autoflush
$|=1; # omt

print "ecFlow log server running on port $port\n";
print "Serving files from:\n";


foreach my $p ( @path )
{
	print "   $p\n";
}

print "\n";

print "Directory mapping:\n";
my %map = (split(":", $map ));
foreach my $p ( sort keys %map )
{
	print "   $p maps to $map{$p}\n";
}

print "\n";



sub REAPER {
	print "$$: SIG{CHLD}\n";
	1 until(-1 == waitpid(-1,WNOHANG));
	$SIG{CHLD} = \&REAPER;
}

$SIG{CHLD} = \&REAPER;

my $server = IO::Socket::INET->new(
	LocalPort => $port,
	Reuse     => 1,
	Listen    => 10,
	Type      => SOCK_STREAM
	);

my $client = 0;
my $pid;

$SIG{PIPE} = "IGNORE";

for(;;)
{
	$SIG{CHLD} = "IGNORE";
	$client = $server->accept();
	$SIG{CHLD} = \&REAPER;
	
	last unless(defined $client);

	my $pid = fork;

	if($pid)
	{
		close($client);
		next;
	}

	unless(defined $pid)
	{
	#	print $client "ERROR: fork: $!\n";
		close($client);
		warn "fork: $!";
		next;
	}	

	close($server);

	my $request = <$client>;
	chomp($request);

	my ($action,$param) = split(" ",$request,2);

	foreach my $k ( keys %map )
	{
		if($param =~ m,^$k,)
		{
			$param =~ s,^$k,$map{$k},;
			last;
		}
	}

	$action = "do_$action";	
	eval {
		no strict;
		$action->($param);
	};
	#print $client $@ if($@);
	warn "$@" if $@;
	exit;

}
print "$$ eof: $!\n";

sub do_get {
	my ($param) = @_;

	print "get $param\n";
	validate($param);

	open(IN,"<$param") || die "$param: $!";
	my $buf;
	my $size = 64*1024;
	my $read;

	while( ($read = sysread(IN,$buf,$size)) > 0)
	{
                $buf =~ s/\x00//eg;
                $buf =~ s/  //eg;
                $buf =~ s/ //eg;
				$buf =~ s/FDB; WARNING;  No directory entry matches:(.*)$//eg;
		syswrite($client,$buf,$size);
	}

	close(IN);
}

sub do_list {
	my ($param) = @_;

	print "list $param\n";
	validate($param);

	my ($name,$path) = fileparse($param);

	$name =~ s/\..*//;

	opendir(DIR,$path) || die "$path: $!";
	while(my $d = readdir(DIR))
	{
	  next unless( $d =~ /^$name\./);	
	  my @x = stat "$path$d";
	  if(@x)
	    {
	      print $client "$x[2] $x[4] $x[5] $x[7] $x[8] $x[9] $x[10] $path$d\n";
	    }
	}
	close(DIR);

}

sub validate {
	my ($file) = @_;

	my @f = grep { $_ ne '..' } split("/",$file);
	$file = join("/",@f);

	foreach my $p ( @path )
	{
		return if($file =~ /^$p/);
	}
	die "Invalid file requested $file\n";
}

