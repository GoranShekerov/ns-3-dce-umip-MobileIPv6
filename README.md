ns-3-dce-umip-MobileIPv6
========================

I've put these codes to be a starting point for someone who want to work in the field of Mobile IPv6 simulations in ns-3.
 
Modified ns-3 codes and additional modules for simulating MIPv6 in ns-3 

This codes are aimed to simulate MIPv6 in ns-3 ussing dce-umip 1.2 on Ubuntu 12.04 x64


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
