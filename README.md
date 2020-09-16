# ts	

Torus Simulator: simulator of traffic within multidimensional torus interconnect


Background:
-----------

1. d-dimensional torus of size k;
2. von Neuman neighborhood;
3. local packet switching rules;
4. using shortest paths only;
5. random load balancing;
6. exponential distribution of time between packets;
7. store-and-forward mode;
8. limitations of node internal buffer (queue) length;
9. node packet queue extraction: the first suitable;
10. separate tracts for sending/receiving packets for each port.


Description:
------------

Multidimensional torus interconnect is de facto standard for high performance
low latence network topology for supercomputers, clusters, and networks-on-chip.

d-dimensional lattice of size k is simulated, its nodes indexed with d-tuples 
having components' range from 0 to k-1. A lattice node represents a computing
and packed switching device. A hypertorus is obtained from a hypercube via 
closing (connecting) opposite facets in each dimension.

In von-Neumann's neighborhood, neighboring cells are situated at Manhattan 
distance equal to 1: only one coordinate changes, the difference belong to {-1,1}.
Neighbors are connected via facets which are (d-1)-dimension hypercubes.
For a hypertorus cell, there are 2*d neighbors. For connection with each its 
neighbor, a device has a separate port. A port is specifies by a tuple (m,r),
where m is the number of dimension and r is the number of direction: -1 to origin, 
1 to (plus) infinity. 

Each node generates packets for a random destination node. A packet is delivered
to the destination based on local switching rules of nodes. Statistical information
is collected, processed, and printed. 

Based on the coordinate difference represented as subtraction of the current node
address from the destination node address, we define the following packet switching rules: 

 a) first coordinate with nonzero difference;

 b) random coordinate among coordinates with nonzero difference;

 c) random coordinate among coordinates with nonzero difference, 
    choice probability is proportional to the coordinate difference absolute value; 

 d-f) similar to a)-c) for free ports only (take into consideration the node state). 

Computing the coordinate difference difference, we choose the shortes path among 
two directions: clockwise - represented by a positive number, counterclockwise - 
represented by a negative number. When we switch a packet to the corresponding port (m,r), 
m equals to the absolute value of the difference and r equals to its sign.


Command line format:
--------------------

>ts [options]


Options (keys):
---------------

* --d=dimension       lattice dimension,
* --k=size            lattice size,	
* --r=rule            packet switching rule: a-f,
* --cht=channel-time  time of a packet transmission within a channel,
* --bl=buffer-length  length of device (node) enternal buffer,
* --lambda=node-traffic-intensity (exponential distribution),
* --maxst=halt-simulation-time,
* --dbg=debug-level, = 0,1,2...

Defaults: ts --d=3 --k=4 --r=a --cht=100 --bl=10000 --maxst=1000000 --dbg=0


Output:
-------

ts outputs input information and statistical information of simulation; 
in debug mode (with dbg>0), detailed information on the simulation process 
is provided. 


An example:
-----------

```
./ts --r=c --lambda=0.01 --d=4
***** Input information *****
torus dimensions d=4, size k=4
lambda=1.000000e-02, cht=100, bl=1000
switching rule c

simulating...

***** Simulation Statistics *****
simulation time: 1000001 (mtu)
generated packets: 2572820
delevered packets: 2571241
torus performanse: 2.571238e+00 (pkt/mtu)
torus load: 5.043945e+01 (%)
average hops per packet: 4.016371e+00
average packet channel time: 1.474520e+02 (mtu)
```

References:
-----------

Zaitsev, D.A., Tymchenko, S.I., Shtefan, N.Z.
Switching vs Routing within Multidimensional Torus Interconnect, 
PIC&ST2020, October 6-9, 2020, Kharkiv, Ukraine.

Zaitsev, D.A., Shmeleva, T.R., and Groote, J.F. 
Verification of Hypertorus Communication Grids by Infinite Petri Nets 
and Process Algebra, IEEE/CAA Journal of Automatica Sinica, 6(3), 2019, 733-742. 
http://dx.doi.org/10.1109/JAS.2019.1911486/

Zaitsev, D.A., Shmeleva, T.R., Retschitzegger, W., Proull, B. 
Security of grid structures under disguised traffic attacks, 
Cluster Computing, 19(3) 2016, 1183-1200. 
http://dx.doi.org/10.1007/s10586-016-0582-9/

Zaitsev, D.A. A generalized neighborhood for cellular automata,
Theoretical Computer Science, 666 (2017), 21-35, 
http://dx.doi.org/10.1016/j.tcs.2016.11.002/

---------------------------
http://member.acm.org/~daze
---------------------------

