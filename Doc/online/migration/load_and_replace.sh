
  ecflow_client --load multi1.def
  ecflow_client --replace /multi/main/00/test/fc_complete/test2 multi2.def
  ecflow_client --replace /test20121123 multi3.def

  python TestBench.py multi1.def
  python TestBench.py  multi3.def
