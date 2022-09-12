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

# try to load sha1 module
my $hasSha1 = try_load_module("Digest::SHA1 qw(sha1_hex)");

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

print "Digest::SHA1 available: ${hasSha1}\n\n";

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

	my @params;
	my ($action,$path) = split(" ",$request,2);
	if ($action eq "delta") {
		my $p1; my $p2, my $p3;
		($p1, $p2, $p3, $path) = split(" ",$path,4);
		push(@params,$p1,$p2,$p3); 
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
		$action->($path, @params);
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

sub do_getf {
	my ($path) = @_;

	print "getf $path\n";
	validate($path);

	open(IN,"<$path") || die "$path: $!";
	my $buf;
	my $size = 64*1024;
	my $read;

	# add a "header" to the front
	my @md;
	(@md) = &meta($path);
	my $mdSize = @md;
	if ($mdSize == 3)
	{
		print $client "0:$md[1]:$md[2]:";
		# print "meta=0:$md[1]:$md[2]:\n";
	} else {
		print $client "0:::";
	}

	# send data
	while( ($read = sysread(IN,$buf,$size)) > 0)
	{
		syswrite($client,$buf,$size);
	}

	close(IN);
}

sub do_delta {
	my ($path, $pos, $mtime, $chksum) = @_;

	print "delta $pos $mtime $chksum $path\n";
	validate($path);

	# get metadat from file
	my @md;
	(@md) = &meta($path);
	my $mdSize = @md;
	my $aSize="";
	my $aMtime="";
	my $aChksum="";

	print "md=@md\n";

	my $all=1;
	if ($mdSize == 3)
	{
		$aSize=$md[0];
		$aMtime=$md[1];
		$aChksum=$md[2];

		if ($aSize == $pos)
		{
			# nothing changed
			if($aMtime == $mtime && ($chksum == "x" || $aChksum == $chksum)) 
			{
				print $client "1";
				return;
			} 
		} 
		elsif ($aSize > $pos && $aChksum == $chksum)
		{
			$all=0;
		}
	}
	
	print "all=$all\n";

	open(IN,"<$path") || die "$path: $!";
	my $buf;
	my $size = 64*1024;
	my $read;

	# try to send delta
	if ($all == 0)
	{
		if (seek(IN, $pos, 0) ==  1) 
		{
			# send "header"
			print $client "1:$aMtime:$aChksum:";
			# send data
			while( ($read = sysread(IN,$buf,$size)) > 0)
			{
				syswrite($client,$buf,$size);
			}
			close(IN);
			return;
		}
	} 
	
	# if we are here the whole file will be sent
	if (seek(IN, 0, 0) == 1)
	{
		# send "header"
		print $client "0:$aMtime:$aChksum:";
		# send data
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

sub meta {
	my ($path) = @_;
	my @x = stat "$path";
	if(@x) 
	{	
		my $v="x";
		# the sha1 module might not be available
		if ($hasSha1) 
		{
			# the chaksum is the sha1 of the 1024 bytes
			my $fh;
			if (open($fh, "<", $path))
			{
				my $buf;
				my $maxSize=1024;
				read($fh, $buf, $maxSize);
				$v = sha1_hex($buf);
			}
			close($fh);
		} 		
		
		# size, modtime, checksum
		return ($x[7], $x[9], $v);
	}
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

sub try_load_module {
  my $mod = shift;
  eval("use $mod");
  if ($@) {
    return(0);
  } else {
    return(1);
  }
}
