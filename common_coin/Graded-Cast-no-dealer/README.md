This is the attempt at making Grade-Cast which will be used for Graded-VSS.

The dealer in this scenario is not a separated entity but is given as an argument for all processes.\
The process choocen to be the dealer will act as the dealer at the start and then continue on as a regural process.\
For simple simulation if the process id is a multiplier of 3 then it will count as a "bad" process and
 it will send a different message.

Brief:\
The dealer will distribute the secret to all other processes and then quit.\
The other processes will receive the secret and then try to pass it to all other processes.\
Each process will count the times it received the same message (tally) from all other processes and print an output struct.\
Each processes counts itself in the tally from the start since zmq doesn't really support sending messages to yourself
 unless you make a new port which would make things trickier for now.
