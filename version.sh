#!/bin/sh

if [ -x $(which git) ] && [ -d ".git" ]
then
	VERSION=$(git describe --dirty --always --tags)
	echo ${VERSION} > VERSION
elif [ -f VERSION ]
then
	VERSION=$(cat VERSION)
else
	exit 1
fi

echo ${VERSION} | tr -d '\n'
