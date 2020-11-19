#!/bin/bash
#borrowing from Cameron. Thanks

echo "  ///////////////////"
echo " ////  IPC IDs  ////"
echo "///////////////////"
ipcs | grep -Eo '[0-9]+\W+kong'

printf "%s\n" ""
echo "  /////////////////////////"
echo " ////  Processes IDs  ////"
echo "/////////////////////////"
ps -U kong | grep oss

for y in $(ps -U kong | grep oss | awk '{print $1}'); do kill $y; done > /dev/null 2>&1
for x in $(ipcs | grep kong| awk '{print $2}'); do ipcrm -m $x || ipcrm -s $x || ipcrm -q $x; done > /dev/null 2>&1

printf "%s\n" ""
echo "  /////////////////////////////"
echo " ////  Remaining IPC IDs  ////"
echo "/////////////////////////////"
ipcs | grep -Eo '[0-9]+\W+kong'

printf "%s\n" ""
echo "  ////////////////////////////////"
echo " ////  Remiaing Process IDs  ////"
echo "////////////////////////////////"
ps -U kong | grep oss
printf "%s\n" ""
