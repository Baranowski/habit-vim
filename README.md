# Requirements

1. Linux operating system
2. yaml-cpp (can be obtained from here: http://code.google.com/p/yaml-cpp/)

This project will build only if yaml-cpp is installed globally on your system. However, modifying Makefile to fix that should not be hard.

# Installation

1. Run `make` to build the wrapper and analyze tools.
2. Add the `bin/` subdir to your `PATH` environment variable
   or create a symlink to the `bin/hvim` file and put it
   somewhere in your `PATH`.

# Usage

Simply use the `hvim` command just as you would use `vim`. This will log every keystroke to a log file, by default in `$HOME/.vim/hvim-log`.

Then create the config file (the `conf/main.yaml` file is a good starting point and should probably work if you haven't done something weird with your .vimrc). You may run the following to debug it:

    # Adjust those variables if needed
    HVIM_DIR="./"
    LOG_PATH="$HOME/.vim/hvim-log"
    CONF_PATH="$HVIM_DIR/conf/main.yaml"

    $HVIM_DIR/bin/analyze $CONF_PATH --mode-log < $LOG_PATH > debug_output

This will create a `mode-<Mode Name>.log` file for every mode defined in your config. Each of those files will contain all the keys pressed in the corresponding mode. In addition the program will merge those logs and write them to the standard output (the `debug_output` file in the above example).

If you are happy with the result (i.e. all the keystrokes
have been identified as belonging to the correct mode), you may use the program to find the most common keystroke patterns. To do so, run:

    $HVIM_DIR/bin/analyze $CONF_PATH < $LOG_PATH > summary

This will write to the standard output (the `summary` file in this example) the most often used key combinations, ordered from the most to the least common one.

# Configuration

Edit the `bin/hvim` file. It's pretty straightforward.

# Bugs

See the TODO file.

# Credits

This sofware uses and includes the 'sarray' package developed
by Sean Quinlan and Sean Doward. See the lib/sarray/sarray.c
file for the license.
