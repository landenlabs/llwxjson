#!/bin/tcsh

set prog=./DerivedData/Build/Products/Release/llwxjson
set prog=llwxjson


echo "input file=$1"
$prog -dump -noHttpdPrefix $1 >! /tmp/wxnew.json

ls -al $1 /tmp/wxnew.json
jd $1 /tmp/wxnew.json

jq --sort-keys "." /tmp/wxnew.json >!  /tmp/wxNewSort.json
jq --sort-keys "." $1              >!  /tmp/wxOrgSort.json
meld /tmp/wxNewSort.json /tmp/wxOrgSort.json
   
   