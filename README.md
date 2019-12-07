# DistributedAlgorithms2019

## Authors
- Stefano Huber
- Orazio Rillo

## General Structure:
The code is intended to implement a *Localized Causal Order Broadcast* instance.
In order to achieve so, we have implemented 5 incremental abstractions
that eventually will allow us to implement it.

- Link
- Best Effort Broadcast
- Uniform Reliable Broadcast
- Fifo Broadcast
- Localized Causal Order Broadcast

These abstractions communicate among themselves in a concurrent
fashion: the lower (writer) and the higher (reader) abstractions share a 
common global queue (pipe) which is accessible only when a lock is
 acquired, so that read and write can't happen simultaneously. 

A further complication need to be solved in this kind of communication: 
as the reader is always eager to read, but sometimes there is just nothing
to read, the reader must **block** and wait until the pipe is non-empty again.

In order to perform it, two strategies can be used: the easier one is the 
busy waiting strategy, which is actually very resource consumptive and therefore 
it has been avoided. The other one uses interrupts, so that the reader
waiting for the queue to be filled can sleep and be notified by the 
writer itself. The latter option has been chosen, and implemented with 
c++ condition variables. 

### Link
The link abstraction emulates TCP. 
TODO: Further description

1)  *Validity*: If pi and pj are correct, then every message sent by pi to pj is
    eventually delivered by pj
2) *No duplication*: No message is
   delivered (to a process) more than once
3) *No creation*: No message is
   delivered unless it was sent

### Best Effort Broadcast
*Best Effort Broadcast* need these three properties in order to be correct:
1) *validity*: If pi and pj are correct, then every message broadcast by pi is
             eventually delivered by pj
2) *No duplication*: No message is delivered more than once
3) *No creation*: No message is delivered unless it was broadcast

The duplication and no creation properties are assured by the underlying
perfect link abstraction. The validity, instead relies on the perfect link
property to eventually validity. Therefore, once implemented Perfect Links,
in order to implement Beb:

- BeBroadcast: for every process, send a perfect link message
- BeDeliver: every process delivered by perfect link gets delivered by Beb.

### Uniform Reliable Broadcast:
*Uniform Reliable Broadcast* needs these properties to be implemented:
1) all Beb properties
2) *Uniform Agreement*: For any
   message m, if a process delivers m, then
   every correct process delivers m   
   
We implement Uniform Reliable Broadcast uniform agreement without a 
*Perfect Failure Detector*, given the assumption that at most a minority
of processes can crash (written in the project description).

In order to overcome this issue, we can assume that before 
urb delivering a message, instead of waiting for an ack from all processes 
that are still alive (we can't judge it as we don't have a failure detector), 
 we just wait for an ack from the majority of the total processes.
 
In fact, as soon as we have received the majority of the acks, we are 
sure that at least one of them comes from a correct process, which will be 
responsible to broadcast it (using *best effort broadcast*) to all correct 
processes. Therefore we can be sure that if we have received the majority 
of the acks, then we can deliver the message without breaking the *Uniform 
Agreement* property.

### Fifo Broadcast:
*Fifo Broadcast* requires (apart from the properties of *URB*) that all
 processes receive the messages coming 
from the same sender in the order they have been broadcasted.

We can assure this property by simply storing the messages delivered by urb
 which are not in the correct order in set, and retrieving them out of 
 the set as soon as their dependencies are fulfilled.
 
An additional improvement has been done: since we know that sequence 
numbers are ordered from 1 to n, as soon as we urb deliver a message 
out of order,
we can assume that all messages before the one received have been broadcasted.
Therefore, we can deliver them even if we haven't strictly speaking 
received them.


 





