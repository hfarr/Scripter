#!/bin/bash

# When this is invoked by C we get a little oopsie
# and $1 is two.sh, and $2 is the actual input we want to send.
# so. If I wasn't middlemanning this with shell scripts it wouldn't be so
# bad..
python3 handler.py "$2"
