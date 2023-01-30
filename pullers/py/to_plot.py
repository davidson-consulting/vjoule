import inotify.adapters
import os
import matplotlib.pylab as plt
from datetime import datetime
import signal
import time
import argparse


cpuConso = {}
ramConso = {}
gpuConso = {}

#
# Wait for an signal from the service
# The signal is sent using inotify, and is emitted each time new values are written by the service
# This is the correct way of waiting formula information as it is a passive wait
# 
def waitServiceIteration (servicePath):
    i = inotify.adapters.Inotify()
    i.add_watch(servicePath)

    while True :
        for event in i.event_gen(yield_nones=False):
            (_, type_names, path, filename) = event
            if (filename == "cpu") :
                return


#
# Read the consumption of a cgroup
# @returns:
#     - cpuConso: insert a new entry
#     - ramConso: insert a new entry
# @params:
#     - cgroup: the path of the cgroup result to read
#
def readConsumption (i, cpuConso, ramConso, gpuConso, cgroup, servicePath) :
    cpu = 0.0
    mem = 0.0
    gpu = 0.0
    with open (cgroup + "/cpu", 'r') as file :
        cpu = float (file.read().rstrip())

    if (os.path.isfile (cgroup + "/ram")) :
        with open (cgroup + "/ram", 'r') as file :
            mem = float (file.read().rstrip())

    if (os.path.isfile (cgroup + "/gpu")) :
        with open (cgroup + "/gpu", 'r') as file :
            gpu = float (file.read().rstrip())

    cgroupName = cgroup[len (servicePath):]
    if (cgroupName in cpuConso) :
        cpuConso[cgroupName][i] = cpu
        ramConso[cgroupName][i] = mem
        gpuConso[cgroupName][i] = gpu
    else :
        cpuConso[cgroupName] = {i : cpu}
        ramConso[cgroupName] = {i : mem}
        gpuConso[cgroupName] = {i : gpu}
    
#
# Read the tree of cgroup written by the formula
# @returns:
#     - cpuConso: insert a new entry for each monitored cgroup 
#     - ramConso: insert a new entry for each monitored cgroup 
# @params:
#     - path: the path of the formula result dir
# 
def readCgroupTree (i, cpuConso, ramConso, gpuConso, path) :
    for root, dirs, files in os.walk (path) :
        if ("cpu" in files) :
            # this is a cgroup consumption result
            print ("Reading : " + str(root))
            readConsumption (i, cpuConso, ramConso, gpuConso, root, path)

#
# Plot the result of the pulling
# @params:
#    - cpuConso: the result of cpu consumption read
#    - ramConso: the result of ram consumption read
#
def plotResults (cpuConso, label) :
    for cgroup in cpuConso :
        if (len (cpuConso[cgroup]) > 1) : 
            lists = sorted (cpuConso[cgroup].items ())

            x, y = zip (*lists)
            for i in range (1, len (y)) : # we update the consumptions  
                cpuConso[cgroup][x[len (y) - i]] = y[len (y) - i] - y[(len (y) - i) - 1]
        
            lists = sorted (cpuConso[cgroup].items ())
            x, y = zip (*(lists[1:]))
            plt.plot (x, y, label=label + ":" + cgroup)

#
# Main loop reading result until Ctrl-C is pressed
# 
#
def main () :
    servicePath = "/etc/vjoule/results/"
    
    i = 0    
    while True : 
        waitServiceIteration (servicePath) # wait for a new report

        now = datetime.now()
        current_time = now.strftime("%H:%M:%S/") + str (i)
        print (current_time)
        readCgroupTree (current_time, cpuConso, ramConso, gpuConso, servicePath)

#
# When Ctrl-C is pressed, export the result into out.jpg
#
#
def handler(signum, frame):
    my_dpi = 96
    plt.figure(figsize=(800/my_dpi, 800/my_dpi))

    plotResults (cpuConso, "cpu")
    plotResults (ramConso, "ram")
    plotResults (gpuConso, "gpu")
    
    plt.legend()
    plt.xticks(rotation=40)
    
    plt.savefig ("out.jpg", dpi=my_dpi)
    #plt.show ()
    print ("Figure exported to out.jpg")
    exit(0)
    
if __name__ == '__main__':
    signal.signal(signal.SIGINT, handler)
    
    main()
