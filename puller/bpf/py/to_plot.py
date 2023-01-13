import os
import matplotlib.pylab as plt
from datetime import datetime
import signal
import time
import argparse

conso = {}

#
# Read the consumption of a cgroup
# @returns:
#     - conso: insert a new entry
# @params:
#     - cgroup: the path of the cgroup result to read
#     - bpfPath: the path of the bpf result root 
#
def readConsumption (i, conso, cgroup, bpfPath) :
    cpu = 0.0
    with open (cgroup + "/v_joule", 'r') as file :
        c = file.read().rstrip()
        if (c != "") : 
            cpu = float (c)
        else :
            return None

    cgroupName = cgroup[len (bpfPath):]
    if (cgroupName in conso) :
        conso[cgroupName][i] = cpu        
    else :
        conso[cgroupName] = {i : cpu}


#
# Read the consumption of RAPL
# @returns:
#     - conso: insert a new entry
# @params:
#     - cgroup: the path of the cgroup result to read
#     - bpfPath: the path of the bpf result root 
#
def readRaplConso (i, conso, formulaPath) :
    cpu = 0.0
    if os.path.exists (formulaPath + "/RAPL") : 
        with open (formulaPath + "/RAPL", 'r') as file :
            c = file.read().rstrip()
            if (c != "") : 
                cpu = float (c)
            else :
                return None
        
            cpu = float (c)

        if ("#RAPL" in conso) :
            conso['#RAPL'][i] = cpu        
        else :
            conso['#RAPL'] = {i : cpu}


#
# Read the tree of cgroup written by the formula
# @returns:
#     - conso: insert a new entry for each monitored cgroup 
# @params:
#     - path: the path of the formula result dir
# 
def readCgroupTree (i, conso, path) :
    for root, dirs, files in os.walk (path) :
        if ("v_joule" in files) :
            # this is a cgroup consumption result
            print ("Reading : " + str(root))
            readConsumption (i, conso, root, path)


#
# Plot the result of the pulling
# @params:
#    - conso: the result of consumption read
#
def plotResults (conso, label) :
    for cgroup in conso :
        if (len (conso[cgroup]) > 1) : 
            lists = sorted (conso[cgroup].items ())

            x, y = zip (*lists)
            for i in range (1, len (y)) : # we update the consumptions  
                conso[cgroup][x[len (y) - i]] = y[len (y) - i] - y[(len (y) - i) - 1]
        
            lists = sorted (conso[cgroup].items ())
            x, y = zip (*(lists[1:]))
            plt.plot (x, y, label=label + ":" + cgroup)
    

            #
# Main loop reading result until Ctrl-C is pressed
# 
#
def main () :
    formulaPath = "/etc/vjoule/bpf_sensor/"
    
    i = 0    
    while True : 
        now = datetime.now()
        current_time = now.strftime("%H:%M:%S/") + str (i)
        print (current_time)
        readCgroupTree (current_time, conso, formulaPath)
        readRaplConso (current_time, conso, formulaPath)
        time.sleep (1)


        #
# When Ctrl-C is pressed, export the result into out.jpg
#
#
def handler(signum, frame):
    my_dpi = 96
    plt.figure(figsize=(800/my_dpi, 800/my_dpi))

    plotResults (conso, "conso")
    
    plt.legend()
    plt.xticks(rotation=40)
    
    plt.savefig ("out.jpg", dpi=my_dpi)
    #plt.show ()
    print ("Figure exported to out.jpg")
    exit(0)
    
if __name__ == '__main__':
    signal.signal(signal.SIGINT, handler)
    
    main()
