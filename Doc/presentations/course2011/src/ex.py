family(PROJECT)
  def_fam()
  families = '00 12'
  var('ENSEMBLES', 50)
  trigger("make==complete")
  for fam in qw(families):
      family(fam)
      if fam == "00":
          var('DELTA_DAY', 1)
      else:
          var('DELTA_DAY', 0)
      var('EMOS_BASE', fam)
      call_skull(PROJECT)

      family("pop")
      oncl()
      trigger("./"+PROJECT+"==complete")
      call_pop_skull()
      endfamily("pop")

      family("ws")
      onws()
      trigger("./"+PROJECT+"==complete")
      call_pop_skull()
      endfamily("ws")
      endfamily(fam)
endfamily(PROJECT)
