#!/usr/bin/perl -w
if ($#ARGV < 0) {
	print "No Parameters Passed In\n";
	exit;
}
foreach my $arg(@ARGV) {
	print "Parameter: $arg\n";
}
my $d=0;
for (my $i=0; $i < 10000000; $i++) {
	$d++;
}
print "Sleeping for 1 second...\n";
sleep 1;
print "Done\n";
