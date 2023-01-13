import inotify.adapters
import os
import matplotlib.pylab as plt
from datetime import datetime
import signal
import time
import argparse


cpuConso = {}
ramConso = {}

#
# Wait for an signal from the formula
# The signal is sent using inotify, and is emitted each time new values are written by the formula
# This is the correct way of waiting formula information as it is a passive wait
# 
def waitFormulaSignal (formulaPath):
    i = inotify.adapters.Inotify()
    i.add_watch(formulaPath)

    while True :
        for event in i.event_gen(yield_nones=False):
            (_, type_names, path, filename) = event
            if (filename == "formula.signal") :
                return


#
# Read the consumption of a cgroup
# @returns:
#     - cpuConso: insert a new entry
#     - ramConso: insert a new entry
# @params:
#     - cgroup: the path of the cgroup result to read
#
def readConsumption (i, cpuConso, ramConso, cgroup, formulaPath) :
    cpu = 0.0
    mem = 0.0
    with open (cgroup + "/package", 'r') as file :
        cpu = float (file.read().rstrip())

    if (os.path.isfile (cgroup + "/memory")) :
        with open (cgroup + "/memory", 'r') as file :
            mem = float (file.read().rstrip())

    cgroupName = cgroup[len (formulaPath):]
    if (cgroupName in cpuConso) :
        cpuConso[cgroupName][i] = cpu
        ramConso[cgroupName][i] = mem
    else :
        cpuConso[cgroupName] = {i : cpu}
        ramConso[cgroupName] = {i : mem}
    
#
# Read the tree of cgroup written by the formula
# @returns:
#     - cpuConso: insert a new entry for each monitored cgroup 
#     - ramConso: insert a new entry for each monitored cgroup 
# @params:
#     - path: the path of the formula result dir
# 
def readCgroupTree (i, cpuConso, ramConso, path) :
    for root, dirs, files in os.walk (path) :
        if ("package" in files) :
            # this is a cgroup consumption result
            print ("Reading : " + str(root))
            readConsumption (i, cpuConso, ramConso, root, path)

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
def main (args) :
    formulaPath = "/etc/vjoule/" + args.formula + "_formula"
    
    i = 0    
    while True : 
        waitFormulaSignal (formulaPath) # wait for a new report

        now = datetime.now()
        current_time = now.strftime("%H:%M:%S/") + str (i)
        print (current_time)
        readCgroupTree (current_time, cpuConso, ramConso, formulaPath)

#
# When Ctrl-C is pressed, export the result into out.jpg
#
#
def handler(signum, frame):
    my_dpi = 96
    plt.figure(figsize=(800/my_dpi, 800/my_dpi))

    plotResults (cpuConso, "cpu")
    plotResults (ramConso, "ram")
    #plt.legend()
    plt.xticks(rotation=40)
    
    plt.savefig ("out.jpg", dpi=my_dpi)
    #plt.show ()
    print ("Figure exported to out.jpg")
    exit(0)
    
if __name__ == '__main__':
    signal.signal(signal.SIGINT, handler)
    parser = argparse.ArgumentParser (description="Puller for influxdb")
    parser.add_argument ('formula', default="simple");

    args = parser.parse_args ()
    
    main(args)
