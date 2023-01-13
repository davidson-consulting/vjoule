from influxdb import InfluxDBClient
import inotify.adapters
import os
import matplotlib.pylab as plt
from datetime import datetime
import signal
import time
import argparse


VJOULE_PATH = "/etc/vjoule/simple_formula/"

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
def readConsumption (i, db, cpuConso, ramConso, cgroup, formulaPath) :
    cpu = 0.0
    mem = 0.0
    with open (cgroup + "/package", 'r') as file :
        cpu = float (file.read().rstrip())

    if (os.path.isfile (cgroup + "/memory")) :
        with open (cgroup + "/memory", 'r') as file :
            mem = float (file.read().rstrip())

    cgroupName = cgroup[len (formulaPath):]

    lastCpu = 0.0
    lastMem = 0.0
    if (cgroupName in cpuConso) :
        lastCpu = cpuConso[cgroupName]
        lastMem = ramConso[cgroupName]
        
    
        json_body = [
            {
                "measurement" : "cpu",
                "tags" : {
                    "target" : cgroupName
                },
                "time": i,
                "fields": {
                    "value": cpu - lastCpu           
                }
            },
            {
                "measurement" : "memory",
                "tags" : {
                    "target" : cgroupName
                },
                "time": i,
                "fields": {
                    "value": mem - lastMem    
                }
            }
        ]
        db.write_points (json_body)
        
    cpuConso[cgroupName] = cpu
    ramConso[cgroupName] = mem

    
    

    
#
# Read the tree of cgroup written by the formula
# @returns:
#     - cpuConso: insert a new entry for each monitored cgroup 
#     - ramConso: insert a new entry for each monitored cgroup 
# @params:
#     - path: the path of the formula result dir
# 
def readCgroupTree (i, db, cpuConso, ramConso, path) :
    for root, dirs, files in os.walk (path) :
        if ("package" in files) :
            # this is a cgroup consumption result
            print ("Reading : " + str(root))
            readConsumption (i, db, cpuConso, ramConso, root, path)


#
# Main loop reading result and inserting them in influxdb
# 
#
def main (args) :
    formulaPath = "/etc/vjoule/" + args.formula + "_formula"
    
    i = 0
    client = InfluxDBClient ('localhost', 8086, 'root', 'root', 'vjoule-' + args.formula)
    #client.drop_database ('vjoule-' + args.formula)
    #client.create_database ('vjoule-' + args.formula)

    while True : 
        waitFormulaSignal (formulaPath) # wait for a new report

        now = datetime.now()
        current_time = now.strftime("%H:%M:%S.%f")
        print (current_time)
        readCgroupTree (current_time, client, cpuConso, ramConso, formulaPath)

    
if __name__ == '__main__':
    parser = argparse.ArgumentParser (description="Puller for influxdb")
    parser.add_argument ('formula', default="simple");

    args = parser.parse_args ()
    main(args)
    
