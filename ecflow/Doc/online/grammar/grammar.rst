.. index::
   single: Grammar
   
.. _grammer:
   
============================
**Definition file Grammar**
============================
 
===== =======
Token Meaning
===== =======
'*'   Zero or more
'!'   Zero or one
'+'   One or more
'>>'  Sequence/concatenation
'|'   alternate
'-'   not
===== =======

.. productionlist::
   defs: *( `nextline` | `extern` ) >> +`suite` >> EOL
   extern: "extern" >> ( `absolutepath`  >> !( ":" >> `identifier` ) >> +`nextline` 
   suite: "suite" >> suiteName >> *( `variable` | `inlimit` | `defstatus`  | `limit` | `late` | `clock` | `repeat` | `autocancel` | `zombie` ) >> *( `family` | `task` ) >> `endsuite`
   family: "family" >> `familyName` >> !`leaf_ecf` >> *(`task` | `family`) >> `endfamily` 
   task: "task"  >> `taskName`  >> `leaf_ecf`  >> !`endtask`
   leaf_ecf: *( `variable` | `trigger` | `time` | `today` | `date` | `day` | `defstatus` | `complete` | `inlimit` | `label` | `event` | `late` | `limit` | `meter` | `repeat` | `cron` | `autocancel` | `zombie` )
   clock: "clock" >> ( "real"| "hybrid" ) >> ( ( `clock_date` >> !(`hh_mm` | int ) ) | (`hh_mm` | int )) >> +`nextline`  
   trigger: "trigger"  >> `expression` >> +`nextline` 
   complete: "complete" >> `expression` >> +`nextline` 
   variable: "edit" >> `identifier` >> `varvalue` >> +`nextline`
   label : "label" >> `identifier` >> `quotedstring`  >> +`nextline` 
   time: "time" >> !'+ >> (`timeseries` | `two_int_p` >> “:” `two_int_p`) >> +`nextline`
   today: "today" >> !'+ >> (`timeseries` | `two_int_p` >> “:” `two_int_p` ) >> +`nextline`
   day: "day" >> (“monday” | “tuesday” | “wednesday” | “thursday” | “friday” | “saturday” | “sunday”) + `nextline`
   date: "date"  >> !’+’ >> (  `two_int_p` | ‘*’  )  >> "." >>  ( `two_int_p` | ‘*’) >> "." >> (`two_int_p` | ‘*’)   
   autocancel: "autocancel"  >> !’+’ >> (( `two_int_p` >> ‘:’ >> `two_int_p`) | unsigned integer) >> +`nextline`
   limit: "limit" >> ( `identifier` >> unsigned int ) >> +`nextline` 
   inlimit: "inlimit" >>( (`nodePath` >> ":“ >> `identifier`) | `identifier` )) >> ! unsigned int >> +`nextline`
   event: `eventcontent2` | `eventcontent1`;
   eventcontent1: "event”  >> ( `eventnumber`  |  `eventname`  ) >> +`nextline` 
   eventcontent2: "event”  >> ( `eventnumber`  >> `eventname`  ) >> +`nextline` 
   meter: "meter" >> `identifier` >> ( int >> int >> !unsigned int) >> +`nextline`
   defstatus: "defstatus" >> `dstate`  >> +`nextline` 
   endsuite: "endsuite" >> *`nextline` 
   endfamily: "endfamily" >> +`nextline` 
   endtask: "endtask" >> +`nextline` 
   zombie: "zombie" >> `zombie_type` >> ":" >> !(`client_side_action` | `server_side_action`) >> ":" >> *`child` >> ":" >> !`zombie_life_time` 
   zombie_type: "user" | "ecf" | "path"  
   child: "init" | "event" | "meter" | "label" | "wait" | "abort" | "complete" 
   client_side_action: "fob" | "fail" | "block" 
   server_side_action: "adopt" | "delete" 
   zombie_life_time: unsigned integer  ( default:  user(300), ecf(3600), path(900)  )
   late: "late" >> `late_option` >> !`late_option`  >> !`late_option` >> +`nextline` 
   late_option: "-c" >> `hh_mm` | ("-s" >> `hh_mm` ) | "-a" >> `hh_mm` 
   repeat: "repeat" >> `repeat_type` >> +`nextline`
   repeat_type: `repeat_date` | `repeat_day` | `repeat_month` | `repeat_year` | `repeat_integer` | `repeat_enumerated` | `repeat_string` 
   repeat_day: "day" >> unsigned integer >> !`ymd`
   repeat_month: “month" >> unsigned integer >> !`ymd`
   repeat_year: "year" >> unsigned integer >> !`ymd`
   repeat_integer: "integer" >> `identifier` >> integer >> " " >> integer >> " " >> integer
   repeat_enumerated: "enumerated" >> `identifier` >> +`identifier`
   repeat_string: "string" >> `identifier` >> +`identifier`
   repeat_date: "date" >> `identifier` >> `ymd` >> `ymd` >> unsigned integer
   varvalue: `tickquotedstring` | `quotedstring` | `identifier`    
   suiteName: `node_name` >> +`nextline` 
   familyName: `node_name` >> +`nextline` 
   taskName: `node_name` >> +`nextline` 
   nodestate: "complete” | "unknown” | "queued" | "aborted" | “active”  
   dstate: "complete” | "unknown” | "queued" | "aborted" | “active” | “suspended”  
   eventnumber: unsigned integer 
   eventname: `identifier` 
   hh_mm: !'+' >> `two_int_p` >> ":" >> `two_int_p`   
   clock_date: (  `two_int_p` | ‘*’  )  >> "." >>  (  `two_int_p` | ‘*’  ) >> "." >> (`two_int_p` | ‘*’)   
   cron: "cron" >> ((‘-w >> +int) | (‘-d’ >> +int) | (‘-m’ >> +int)) >> `timeseries` >> +`nextline`
   node_name: (alpha_numeric | ‘_’ )  >> *(alpha_numeric | ‘_’ | ‘.’ )
   comment: ’#’ >> printable chars > `newline`
   nextline: `newline` | `comment`  
   timeseries: `two_int_p` >> “:” >> `two_int_p` >>  `two_int_p` >> “:” >> `two_int_p` >> `two_int_p` >> “:” >> `two_int_p`
   quotedstring: ’”‘ >> *(printable chars) >> ‘”’
   tickquotedstring: ’'’ >> *(print_p - `nextline`) ]
   absolutepath: !’/’ >> `identifier` >> *( ‘/’ >> `identifier` )
   dotdotpath: ".."  >> +( ‘/’ >> `identifier` )  
   dotpath: ‘.’  >> +( ‘/’  >> `identifier` )
   identifier: (alpha_numeric | ‘_’)  >> *(alpha_numeric | ‘_’)
   nodePath: `absolutepath` | `dotdotpath` | `dotpath`    
   expression: printable chars >> !’\’ >> `nextline`
   two_int_p: 2 digit integer
   theYear: 4 digit integer
   ymd: 8 digit integer 
   newline: \n
 
