This is the initial attempt at making Grade-Cast which will be used for Graded-VSS.

Both the dealer and the Grade-Cast will try to read the IP and port from hosts.txt\
The dealer is a separated entity and it can be run first or after the other processes.\
For simple simulation if the process id is a multiplier of 3 then it will count as a "bad" process and
 it will send a different message.

Brief:\
The dealer will distribute the secret to all other processes and then quit.\
The other processes will receive the secret and then try to pass it to all other processes.\
Each process will count the times it received the same message (tally) from all other processes and print an output struct.\
Each processes counts itself in the tally from the start since zmq doesn't really support sending messages to yourself
 unless you make a new port which would make things trickier for now.
