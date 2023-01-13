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
def readConsumption (conso, cgroup, bpfPath) :
    x = []
    y = []
    with open (cgroup + "/v_joule", 'r') as file :
        lines = file.readlines()
        lastX = 0
        lastY = 0
        for line in lines :
            l = line.split ()
            x = x + [float (l[0]) - lastX]
            y = y + [float (l[1]) - lastY]
            #lastX = float (l[0])
            #lastY = float (l[1])
            
    cgroupName = cgroup[len (bpfPath):]
    conso[cgroupName] = (x, y)
    

#
# Read the consumption of RAPL
# @returns:
#     - conso: insert a new entry
# @params:
#     - cgroup: the path of the cgroup result to read
#     - bpfPath: the path of the bpf result root 
#
def readRaplConso (conso, formulaPath) :
    x = []
    y = []
    if os.path.exists (formulaPath + "/cgroup/RAPL") :        
        with open (formulaPath + "/cgroup/RAPL", 'r') as file :
            lines = file.readlines()
            lastX = 0
            lastY = 0
            for line in lines :
                l = line.split ()
                x = x + [float (l[0]) - lastX]
                y = y + [float (l[1]) - lastY]
                #lastX = float (l[0])
                #lastY = float (l[1])
                
        conso["#RAPL"] = (x, y)


#
# Read the tree of cgroup written by the formula
# @returns:
#     - conso: insert a new entry for each monitored cgroup 
# @params:
#     - path: the path of the formula result dir
# 
def readCgroupTree (conso, path) :
    for root, dirs, files in os.walk (path) :
        if ("v_joule" in files) :
            # this is a cgroup consumption result
            print ("Reading : " + str(root))
            readConsumption (conso, root, path)


def smooth (plots, length) :
    res = []
    for i in range (len (plots)) :
        all = 0
        nb = 0
        for j in range (i - length, i + length) :
            if j > 0 and j < len (plots):
                all = all + plots[j]
                nb = nb + 1
        res = res + [all / nb]
    return res


def cumulate (x, y) :
    current = 0
    cumul = 0
    res_y = []
    for i in range (1, len (x)):
        current += x[i]
        cumul += y[i]
        while current >= 1:
            res_y = res_y + [cumul * (1.0 / current)]
            cumul = cumul - (cumul * (1.0 / current))
            current = current - 1.0
            
    return res_y
            

            
#
# Plot the result of the pulling
# @params:
#    - conso: the result of consumption read
#
def plotResults (conso, label) :
    for cgroup in conso :
        x, y = conso[cgroup]

        #y = cumulate (x, y)
        plt.plot (y, label=label + ":" + cgroup)
        plt.legend ()
        plt.savefig ("out.jpg")
    
            #
# Main loop reading result until Ctrl-C is pressed
# 
#
def main () :
    formulaPath = "/etc/vjoule/bpf_sensor/"
    readCgroupTree (conso, formulaPath)
    readRaplConso (conso, formulaPath)
    plotResults (conso, "conso")


if __name__ == '__main__':
    
    main()
