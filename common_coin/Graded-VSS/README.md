This is the attempt at making Graded-VSS.

At this point, we need libzmq and libgsl to work.
For debian based systems its simple, sudo apt libzmq && sudo apt install libgsl27
Otherwise read online on how to install them on your system.

A few other things to note, Grade-Cast has been slightly changed.
Instead of only the delear distributing 1 secret, the delear distributes a secret
and each node then distributes his own secret.
