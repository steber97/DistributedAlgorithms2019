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
out of order (lets call this sequence number *k*, and the last sequence
number we have received is *l*, with *k > l*),
we can assume that all messages before *k* have been broadcasted.
Therefore, we can deliver them even if we haven't strictly speaking 
received them. (from *l+1* to *k*).

### Local Causal Order Broadcast
*Local Causal Order Broadcast* is based on the properties of local order:
For any two messages (m1, m2), we say that *m1 -> m2* (m1 precedes m2) 
1) FIFO: Some process *pi*
         broadcasts *m1* before broadcasting *m2*
2) Local order: Some process *pi* delivers *m1* and then broadcasts *m2*
3) Transitivity: There is a message *m3* such that *m1 -> m3* and *m3 - > m2*

This means that every message *m1* sent by any process *p1*, before being
delivered by any other process, needs to have all its dependencies satisfied.
For instance, if process *p1* before having broadcasted *m1* has delivered 
some message *m2* from a process *p2* that is a causal dependency of *p1*,
then all other process must deliver *m2* before delivering *p1*.

We have faced this challenge using an approach based on vector clocks:
it means that every message contains a vector of so called "clocks", which 
carries information related to the dependencies of that message for every other process.

So, if for instance *m1* has a vector clock `[0, 1, 2, 0]`, it means that
before delivering *m1* every process must have delivered up to message 1 
of process 2, and up to message 2 of process 3 (we start counting processes 
by 1, as the project description does).

It is trivial to satisfy the FIFO property: either we build *LCOB* on top of
*FIFO broadcast*, either we say that every message (with sequence number `s1`) 
carries in its own vector clock a dependency with the process itself up to the
sequence number `s1 - 1`. We opted for the latter, which allowed us
to build *LCOB* on top of *UrBroadcast* instead of *Fifo*, which is 
better for performance, being *UrBroadcast* a lower level abstraction.

Regarding the other two properties, we say that *Local Order* is satisfied
 by the fact that every time a process *p1* delivers a message *m1* from any 
 other process *p2* and then broadcasts *m2*, it increases
 its local vector clock by 1 for *p2*. It means that every other 
 process needs to deliver *m1* before being able to deliver *m2*, as *m2*
 carries in its vector clock the dependency with *m1*.
 
Of course, only in case *p2* is a local dependency of *p1*.

*Transitivity* instead is satisfied by the fact that whenever there 
are three messages such that m1 -> m2 -> m3, then we are assured that 
m1 -> m3, in fact m3 has among its dependencies (in the vector clock) the
dependency with m2, and in return m2 has too the dependency in its vc with
m1. Therefore, whenever some process receives m3, it needs to wait for m2
which needs to wait for m1, therefore transitivity is respected.

The algorithm then stores every incoming message (from the *UrB* level)
into a *pending messages* data structure: every time a new message can be 
delivered (which means that the local vector clock of the process is 
bigger or equal in every entry than the vector clock of the incoming message) 
then it is delivered. The process is repeated until no more process can be 
delivered.

In order to speed up the search for messages that can be delivered, we 
implemented a graph data structure which allows faster lookup for messages
that can be delivered: the graph stores one node for every message received.
Every time we receive a new message *m1*, we add one edge from each message 
*mi* that is a dependency for *m1* to *m1* itself. A counter `unmet_dependencies`
stores the number of dependencies that the message is waiting for in order
to be able to be delivered. 

Every time we deliver a message *m2*, we follow all the edges starting from
*m2*, and subtract 1 to the `unmet_dependencies` counter of every neighbour node.
Whenever any of the neighbour can be delivered (`unmet_dependencies == 0`) then
recursively we can deliver it too.

In this way, instead of keeping a long list of pending messages that has to be iterated
over every time we receive a new message *m1*, we only inspect the messages that are 
influenced by the delivery of *m1*. The memory consumption is *O(m * p)*, where *m*
 is the number of messages and *p* the number of processes, as every node represents 
 a message, and from every node there are at most p outgoing edges.

  

 


 





