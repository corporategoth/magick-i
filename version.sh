#!/bin/sh
#
# Increment Magick build number

VERSION="$1"

if [ -f version.h ] ; then
	BUILD=`fgrep '#define BUILD' version.h | sed 's/^#define BUILD.*"\([0-9]*\)".*$/\1/'`
	BUILD=`expr $BUILD + 1`
else
	BUILD=1
fi
cat >version.h <<EOF
/* Version information for Magick.
 *
 * Services IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#define BUILD	"$BUILD"

const char version_number[] = "$VERSION";
const char version_build[] =
	"build #" BUILD ", compiled " __DATE__ " " __TIME__;
EOF
