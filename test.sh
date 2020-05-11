#!/bin/bash

#pocet cisel bud zadam nebo 10 :)
if [ $# -lt 1 ];then 
    numbers=10;
else
    numbers=$1;
fi;

#round numbers to next power of 2
y=`echo "x=l($numbers)/l(2); scale=0; 2^((x+0.5)/1)" | bc -l;`
if [ "$numbers" -gt "$y" ]; then
    y=`echo "$y*2" | bc -l`
fi
#echo $y

treeheight=`echo "(l($y)/l(2))/1; scale=0;" | bc -l`
treeheight=`echo "$treeheight/1" | bc -l`
treeheight=`echo ${treeheight%.*} | bc -l`
#echo $treeheight
numleafs=`echo "2^($treeheight - 1)" | bc -l`
numprocs=`echo "($numleafs * 2) - 1; scale=0;" | bc -l`
numprocs=`echo "$numprocs/1" | bc -l`
numprocs=`echo ${numprocs%.*} | bc -l`
#echo $numprocs

#numleafs=`echo "x=l($y)/l(2);  scale=0; 2^((x+0.5)/1)" | bc -l`
#numleafs=`echo ${numleafs%.*} | bc -l`
#echo $numleafs

#preklad cpp zdrojaku
mpic++ --prefix /usr/local/share/OpenMPI -o bs bks.cpp

#vyrobeni souboru s random cisly
dd if=/dev/random bs=1 count=$numbers of=numbers

#spusteni
mpirun --prefix /usr/local/share/OpenMPI -np $numprocs bs $numbers $y $numprocs $numleafs

#uklid
rm -f bs numbers
