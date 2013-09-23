';
if ($@){
    print "caught signal: $@\n";
    xabo(); # xabort();
    exit;
  }
print "the job is now complete\n";
xcom(); # xcomplete();
exit;
