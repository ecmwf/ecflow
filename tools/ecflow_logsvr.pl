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

	print("request=",$request,"\n");

	my ($action,$path) = split(" ",$request,2);
	my $param; 
	if ($action eq "delta") {
		($param, $path) = split(" ",$path,2);
	} 

	# map path
	if ($path ne "") {
		foreach my $k ( keys %map ) {
			if($path =~ m,^$k,) {
				$path =~ s,^$k,$map{$k},;
				last;
			}
		}
	}
	
	$action = "do_$action";	
	eval {
		no strict;
		$action->($path, $param);
	};
	#print $client $@ if($@);
	warn "$@" if $@;
	exit;

}
print "$$ eof: $!\n";

sub do_version {
	print $client "2";
}

sub do_get {
	my ($path) = @_;

	print "get $path\n";
	validate($path);

	open(IN,"<$path") || die "$path: $!";
	my $buf;
	my $size = 64*1024;
	my $read;

	while( ($read = sysread(IN,$buf,$size)) > 0)
	{
		syswrite($client,$buf,$size);
	}

	close(IN);
}

sub do_delta {
	my ($path, $pos) = @_;

	print "delta $pos $path\n";
	validate($path);

	open(IN,"<$path") || die "$path: $!";
	my $buf;
	my $size = 64*1024;
	my $read;

	if (seek(IN, $pos, 0) ==  1) 
	{
		while( ($read = sysread(IN,$buf,$size)) > 0)
		{
			syswrite($client,$buf,$size);
		}
	}

	close(IN);
}

sub do_list {
	my ($path) = @_;

	print "list $path\n";
	validate($path);

	my ($name,$dir) = fileparse($path);

	$name =~ s/\..*//;

	opendir(DIR,$dir) || die "$dir: $!";
	while(my $d = readdir(DIR))
	{
	  next unless( $d =~ /^$name\./);	
	  my @x = stat "$dir$d";
	  if(@x)
	    {
	      print $client "$x[2] $x[4] $x[5] $x[7] $x[8] $x[9] $x[10] $dir$d\n";
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

