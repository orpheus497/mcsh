#!/bin/tcsh
set TERM = "xterm-truecolor"
if ( "$TERM" =~ "*truecolor*" ) then
    echo "quoted works"
else
    echo "quoted failed"
endif
