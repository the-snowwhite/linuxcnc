#!/bin/sh
rm -rf autom4te.cache
AUTOGEN_TARGET=${AUTOGEN_TARGET-configure:config.h.in}
set -e
case :$AUTOGEN_TARGET: in
*:configure:*) autoconf; touch configure ;;
esac
case :$AUTOGEN_TARGET: in
*:config.h.in:*) autoheader; touch config.h.in ;;
esac
