#!/bin/bash

if ( test $# -gt 1 )
then
	echo This script requires one or zero arguments
	exit 1
fi

prefix="$1"

if ( test -z "$prefix" )
then
	filecount=$(find . -maxdepth 1 -type f | wc -l | tr -d ' ')
else
	filecount=$(find . -maxdepth 1 -type f -name "${prefix}*" | wc -l | tr -d ' ')
fi

echo "Number of files found: $filecount"

sleep 2

if ( test "$filecount" -gt 0 )
then exit 0
else exit 1
fi
