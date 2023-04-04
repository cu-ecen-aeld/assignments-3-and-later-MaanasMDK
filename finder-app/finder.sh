#!/bin/sh

# Author: Maanas Makam Dileep Kumar
# Description: Shell script to perform finder function.

#check number of arguments
if [ $# -ne 2 ]
then
	echo "Two arguments required!!"
	exit 1
fi

#extract the arguments
filesdir=$1
searchstr=$2

#checking if file directory is valid
if [ -d "$filesdir" ]
then
	filecount=$(find "$filesdir" -type f | wc -l)
	line_count=$(grep -roh $searchstr $filesdir | wc -l)
	echo "The number of files are $filecount and the number of matching lines are $line_count"
	exit 0
else
	echo "Directory not present!!"
	exit 1
fi
