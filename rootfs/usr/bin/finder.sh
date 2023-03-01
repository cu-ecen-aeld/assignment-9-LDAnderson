#!/bin/sh
# Leif Anderson U Colorado
# Usage finder FILESDIR SEARCHSTR
# searches for lines matching SEARCHSTR in directory FILESDIR

# check # of args
if [ $# -lt 2 ]
then
    echo "Invalid arguments"
    exit 1
fi


# valid directory?
if [ ! -d $1 ]
then
    echo "Invalid directory"
    exit 1
fi

cd $1
num_matches=$(grep -R $2 $1 | wc -l)
num_files=$(ls $1 | wc -l)

echo The number of files are $num_files and the number of matching lines are $num_matches
