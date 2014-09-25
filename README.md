ns-3-dce-umip-MobileIPv6
========================

I've put these codes as a starting point, help, compare results, for someone who want to work in the field of Mobile IPv6 simulations in ns-3. 

I did this in june 2014 using dce-umip 1.2 so there was not support for IPv6 sockets in DCE. In dce 1.3 dce-creadle has support for IPv6 sockets so I suppose that the aplication packets can be analyzed in the aplication sinks of the nodes.

In order to analize some network parametars as delay, jitter, throughput.. every application packet betwean CN and MN (or other control packets) must be uniquely identified (time and numerically). The ns3-tagging funcionality does not work with DCE so to accomplish analisis of the network parametars the folowing changes in the ns-3.20 and dce 1.2 code and additional module are implemented:

- the function that creates the packets in the On-Off aplication (VoIP is implemented with On-Off) is modified to include generated time and numerical identification of the packet. In the simulation script, sinks in CN and MN nodes get this packets and send a copy to the "tpa" module.

- tpa module (short from traffic performance analyzer) is aimed to calculate the statistics for the network parametars in the sistem.

- The quagga helper class in dce is modified to allow configuration of the RA interval from the simulation script.

- The umip helper class in dce is modified in order to configure MIPv6 in CN for simulating Route Optimization scenario.

- I've also created GUI aplication named "Ns3 script Runner" to automate the proces of executing the simulation script and generating data for ploting with gnuplot, acheaving 95% confidence level or at most 100 executions. 

This codes are aimed to simulate MIPv6 in ns-3 ussing dce-umip 1.2, in my case on Ubuntu 12.04 x64


Just replace the files in the corespondent location and recompile the ns-3-dce project.

Example
************ recompile dce

cd /root/workspace/bake
./bake.py configure -e dce-linux-1.2 -e dce-umip-1.2
./bake.py build -vvv

or ./bake.py build -vvv -o dce-linux-1.2 -o ns-3.20 //to recompile only dce and ns-3.20 modules

Feel free to ask question on ns-3 google group.
link for umip topic: https://groups.google.com/forum/?fromgroups#!searchin/ns-3-users/umip/ns-3-users/SJl_9bl-Cm0/6CfjuxJFon4J%5B1-25%5D

You can also contact me on g_sekerov@yahoo.com 
