import sys
from os import path
import getopt
from ConfigParser import ConfigParser
from neuros import Neuros
import neuros
from cmd_add import cmd_add

import ports

version = "Xiph.org Positron version 0.1"

def usage():
    print version
    print
    print "positron add: Add files to the Neuros, copying as needed"


def set_config_defaults(config):
    config.add_section("general")
    config.set("general", "musicdir", "MUSIC")

def main(argv):
    options = "c:hm:v"
    long_options = ("config=", "help", "mount-point=", "version")

    # parse global options
    try:
        opts, remaining = getopt.getopt(argv[1:], options, long_options)
    except getopt.GetoptError, e:
        print "Error:", e
        usage()
        sys.exit()

    config_file = None
    mountpoint = None
    for o,a in opts:
        if o in ("-v", "--version"):
            print version
            sys.exit()
        elif o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-c", "--config"):
            config_file = a
        elif o in ("-m", "--mount-point"):
            mountpoint = a

    # Open config file
    config = ConfigParser()
    set_config_defaults(config)
    if config_file != None:
        config.read(config_file)
    else:
        config.read([ports.site_config_file_path(), ports.user_config_file_path()])

    # Override config file settings with command line options
    if mountpoint != None:
        config.set("general","mountpoint",mountpoint)

    # Sanity check
    if not config.has_option("general","mountpoint"):
        print "Error: Neuros mountpoint not set with -m and not present in config file."

    try:
        myNeuros = Neuros(config.get("general", "mountpoint"))
        
        # now select major command
        if len(remaining) == 0:
            usage()
        elif remaining[0] == "add":
            cmd_add(config, myNeuros, remaining[1:])

        exit_value = 0
    except neuros.Error, e:
        print "Error:", e
        exit_val = 1

    sys.exit(exit_value)

    
if __name__ == "__main__":
    main(sys.argv)
