#
# ~/.login - Setup user login shell environment for mcsh (Modern C Shell).
#
# This file is sourced by mcsh for login shells after ~/.mcshrc (or its
# backward-compat fallbacks ~/.tcshrc / ~/.cshrc).  See mcsh(1) and
# environ(7) for details.
#

setenv	EDITOR	vi
setenv	PAGER	less

# umask 077

# set path=( ~/.local/bin $path:q )
