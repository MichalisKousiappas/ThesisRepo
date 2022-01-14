This is the attempt at making Graded-VSS.

At this point, we need libzmq and libgsl to work.
For debian based systems its simple, sudo apt libzmq && sudo apt install libgsl27
Otherwise read online on how to install them on your system.

A few other things to note, Grade-Cast has been slightly changed.
Instead of only the delear distributing 1 secret, the delear distributes a secret
and each node then distributes his own secret.


Note:
This code block is supposed to implement the two way verification but doesn't work.
The reason is that every so often a process will move slightly faster than some other process.
take for this scenario for example:
Distributor is n = 6 and distributor is 2. all processes received the secret and the ok message.
process 3 manages to run faster than process 5 and it sends both the common string Z and the follow up since the message was valid.
process 5 did not manage to send any before 3 send both but at the end it sends both.
process 4 received both messages from 3 in the first check but nothing from 5.
in the 2nd check process 4 receives both messages from 5. since the common string is the same in both messages all is good.

now imagine that 3 is a bad process...it convinces process 4 that the message was not good thus process 4 will distribute nothing
in the 2nd phase meaning that ultimately the messages from 5 got lost.

The reason this happens is that when process 4 acts as a server to receive messages, it doesn't check who sends it only what it got.
on the other hand if it did check and discarded the 2nd message from 3 it would solve the issue since it would wait for the message from process 5.
but then the issue will be that process 4 will wait for a message in phase 2 that will never come.
