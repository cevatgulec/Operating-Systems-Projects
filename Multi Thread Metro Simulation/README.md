you can compile my code as follows gcc metro.c -o metro
you can runn m my code as follows ./metro -s 60 -p 0.5 where -s parameter is simulation time and -p is probability
my code doesn't print anything so after you run it you need to wait until the simulation ends code wil terminate automatically
then you can check the log files
I implemented all parts evertyhing works well you can see the control_center log file and the train log file
I also well commented my code so if you look at it you will see detailed explanation
Simply I have 4 section threads each of them generates trains from A, B, E and F every second for the given probability
I have only 1 queue which is TunnelQueue I keep the trains in this queue
Also I implemented queue.c file to manipulate the queue
and Metro control center thread controls the tunnel if tunnel is available dequeue the train also system overload and breakdown parts are working correctly.

