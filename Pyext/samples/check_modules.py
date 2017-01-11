#!/usr/bin/env python2.7
"""Check if expected modules are loaded.

Usage:
 ./check_modules.py --m "module load git module swap PrgEnv-cray PrgEnv-gnu " --f module_list_file
 ./check_modules.py --m "module swap PrgEnv-cray PrgEnv-gnu" --f module_list_file
 ./check_modules.py --m "module unload grib_api" --f module_list_file
 ./check_modules.py --m "module load gnu/4.8.1 module load python/2.7.8-01" --f module_list_file
 ./check_modules.py --m "module swap PrgEnv-cray PrgEnv-gnu module load cdt/16.03" --f module_list_file
 ./check_modules.py --m "module swap craype-network-aries craype-network-none module switch fftw/3.3.4.7" --f module_list_file

module_list_file is the output of module list e.g.

Currently Loaded Modulefiles:
  1) version/3.2.10(default)             7) ecfs/2.2.1-rc2(new:prodn:default)
  2) verbose/true(default)               8) metview/4.6.4(default)
  3) mode/64(default)                    9) ecaccess/4.0.2(default)
  4) gnu/4.8.1(default)                 10) fftw/3.3.4(default)
  5) python/2.7.8-01(default)           11) emos/437(default)
  6) sms/4.4.14(default)                12) ecmwf/1.0(default)
"""

import argparse
import re
import sys

def main():
    PARSER = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    PARSER.add_argument('--m', help="Space separated list of module commands")
    PARSER.add_argument('--f', help="Path to a file that is the output of module list")
    PARSER.add_argument('--remote_host', help="remote host")
    ARGS = PARSER.parse_args()

    delimter = "module "
    module_cmds = [e.strip() for e in ARGS.m.split(delimter) if e.strip() != ""]
    print "module_cmds ",module_cmds

    module_actions = ("load", "add", "rm", "unload", "swap", "switch")
    modules_required = dict()
    modules_missing = set()

    def set_version(s):
        print " set_version ",s
        try:
            mod, version = s.split('/')
        except ValueError:
            mod = s
            version = None
        modules_required[mod] = version
        if mod in modules_missing:
             modules_missing.remove(mod)

    def unset_version(s):
        # Unloading a specific version has no effect if a different version is loaded!
        print " unset_version ",s
        try:
            mod, version = s.split('/')
            if mod in modules_required and modules_required[mod] == version:
                modules_required.pop(mod)
            modules_missing.add(s)
        except ValueError:
            if s in modules_required:
                modules_required.pop(s)
            modules_missing.add(s)

    print "========================================================="
    print "Read input module list->",module_cmds
    print "========================================================="
    for mod in module_cmds:
        module_cmd = mod.split()
        print "module_cmd ",module_cmd
        action = module_cmd[0]
        if action not in module_actions:
            print "Error: Module command ", module_cmd, " is not correctly formed, second arg must be one of", ', '.join(module_actions)
            return 1
        if action in ('load', 'add'):
            if len(module_cmd) != 2:
                print "Error: invalid module command '%s'" % ' '.join(module_cmd)
            set_version(module_cmd[1])
        if action in ('swap', 'switch'):
            # Swap takes either 1 or 2 arguments
            # module swap pkg/version <-- unloads pkg, loads pkg/version
            # module swap pkg1/v1 pkg2/v2 <-- unloads pkg1/v1, loads pkg2/v2
            if len(module_cmd) > 3:
                print "Error: invalid module command '%s'" % ' '.join(module_cmd)
            if len(module_cmd) == 3:
                unset_version(module_cmd[1])
            set_version(module_cmd[-1])
        if action in ('rm', 'unload'):
            if len(module_cmd) != 2:
                print "Error: invalid module command '%s'" % ' '.join(module_cmd)
            unset_version(module_cmd[1])

    print "========================================================="
    print "Read module list file->",ARGS.f
    print "========================================================="
    modules_loaded = dict()
    module_list = list()
    with open(ARGS.f) as module_list_file:
        for line in module_list_file:
            #print line
            line = line.rstrip()
            if line.find("Currently Loaded") == 0: continue;
            tokens = re.split(r'\s|\d+\)',line)
            while '' in tokens: tokens.remove('')
            #print "tokens",tokens
            for tok in tokens:
                #print "tok",tok
                count = tok.count('/')
                if count == 1:
                   mod,version = tok.split('/')
                   version = version.split("(")[0]
                   print "->(",mod,",",version,")"
                   modules_loaded[mod] = version
                else:
                   print "->(",tok,",None)"
                   modules_loaded[tok] = None

    print "========================================================="
    print "Checking"
    print "========================================================="
    print "->module_missing:   ",modules_missing
    print "->modules_required: ",modules_required
    print "->module list:      ",modules_loaded
    failed = 0
    for mod, version in modules_required.items():
        if version and modules_loaded.get(mod) != version or version is None and mod not in modules_loaded:
            print "Error: expected module %s/%s not found in module list" % (mod, version)
            failed = 1
    for module in modules_missing:
        print "modules_missing - ", module
        if '/' in module:
            mod, version = module.split('/')
            if modules_loaded.get(mod) == version:
                print "Error: unexpected module %s/%s found in module list" % (mod, version)
                failed = 1
        else:
            if module in modules_loaded:
                print "Error: unexpected module %s found in module list" % module
                failed = 1
    return failed

if __name__ == "__main__":
    sys.exit(main())
