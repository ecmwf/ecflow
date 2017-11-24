# tail.pl
';
if ($@){
    print "caught signal: $@\n";
    xabort();
    exit;
  }
print "the job is now complete\n";
xcomplete();
exit;
