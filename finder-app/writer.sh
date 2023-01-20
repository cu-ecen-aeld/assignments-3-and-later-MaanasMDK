#!/bin/bash

# Author: Maanas Makam Dileep Kumar
# Description: Shell script to perform writer function.

#check number of arguments
if [ $# -ne 2 ]
then
	echo "Two arguments required!!"
	exit 1
fi

#extract arguments
file_path=$1
text=$2

dir=$( dirname "$file_path" )

#check if directory path exists
if [ -d "$dir" ]
then
	echo "$text" > $file_path
else
	mkdir -p "$dir" && echo "$text" > $file_path
fi

#check for successful creation of file
if [ ! -f "$file_path" ]
then
    echo "Failed to create file!!"
    exit 1
fi

exit 0
