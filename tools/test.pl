#!/usr/bin/env perl

# my $request = "A B";

# my ($action, $param) = split(" ",$request, 2);
# print("action=",$action,"\n");
# print("param=", $param,"\n");
# @params = split(" ", $param, 2);
# print("params[0]=", $params[0],"\n");
# print("params[1]=", $params[1],"\n");


my $request = "delta B CSS";
chomp($request);

my ($action,$vals) = split(" ",$request,2);

print("action=",$action, "\n");
print("vals=",$vals, "\n");

my $path = $vals;
my $param; 
if ($action eq "delta") {
	($param, $path) = split(" ",$vals,2);
}

print("path=",$path, "\n");

$action = "do_$action";	
eval {
	no strict;
	$action->($path, $param);
};

sub do_get {
	my ($param) = @_;

	print "get $param\n";
}

sub do_delta {
	my ($path, $pos) = @_;

	print "delta $pos $path\n";
}