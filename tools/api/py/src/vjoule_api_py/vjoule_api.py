import inotify.adapters
import os
import subprocess
import sys
import time  


class VJouleError (Exception):

    def __init__ (self, msg) :
        self.msg = msg


class VJouleAPI:

    def __init__ (self):
        self._VJOULE_DIR = "/etc/vjoule/"

        
        ret = subprocess.run (["systemctl", "is-active", "--quiet", "vjoule_service.service"])
        if (ret.returncode != 0) :
            raise VJouleError ("vJoule service is not started. sudo systemctl start vjoule_service.service")

        self._inotifFd = inotify.adapters.Inotify ()
        self._inotifFdW = self._inotifFd.add_watch (self._VJOULE_DIR + "results")

    
        create = subprocess.run (["vjoule_cgutils", "add", "vjoule_api.slice/this_" + str (os.getpid ())])
        if (create.returncode != 0) :
            raise VJouleError ("failed to create cgroup. make sure the current user is in group 'vjoule'.")

        attach = subprocess.run (["vjoule_cgutils", "attach", "vjoule_api.slice/this_" + str (os.getpid ()), str (os.getpid ())])
        if (attach.returncode != 0):
            raise VJouleError ("failed to attach process to cgroup. make sure the current user is in group 'vjoule'.")

        group = VJouleProcessGroup (self, "this_" + str (os.getpid ()))
        self._groups = {"this" : group, "this_" + str (os.getpid ()) :  group}
        
        i = open(self._VJOULE_DIR + "cgroups", 'r')
        found = False
        for l in i.readlines () :
            if (l == "vjoule_api.slice/*") :
                found = True
                break

        if not found :
            o = open (self._VJOULE_DIR + "cgroups", "a+")
            o.write ("vjoule_api.slice/*\n")

        self.__forceSig ()


    def getCurrentMachineConsumption (self) :
        ret = VJouleConsumptionStamp (time.time ())
        if (os.path.isfile (self._VJOULE_DIR + "results/cpu")) :
            i = open (self._VJOULE_DIR + "results/cpu", "r")
            ret.cpu = float (i.read ())
            i.close ()

        if (os.path.isfile (self._VJOULE_DIR + "results/ram")) :
            i = open (self._VJOULE_DIR + "results/ram", "r")
            ret.ram = float (i.read ())
            i.close ()

        if (os.path.isfile (self._VJOULE_DIR + "results/gpu")) :
            i = open (self._VJOULE_DIR + "results/gpu", "r")
            ret.gpu = float (i.read ())
            i.close ()

        return ret


    def getGroup (self, name):
        if (name in self._groups) :
            return self._groups[name]
        else :
            raise VJouleError ("no process group named '" + name + "' is monitored.")


    def createGroup (self, name, pids) :
        cmd = ["vjoule_cgutils", "attach", "vjoule_api.slice/" + name]
        for i in pids :
            cmd = cmd + [str (i)]

        attach = subprocess.run (cmd)
        if (attach.returncode != 0) :
            raise VJouleError ("failed to attach pid to cgroup. make sure the current user is in group 'vjoule'.")
        
        self._groups[name] = VJouleProcessGroup (self, name)
        self.__forceSig ()
        return self._groups[name]

    def __removeGroup (self, name) :
        del self._groups[name]
        
    def __forceSig (self):
        for i in range (2) :
            f = open (self._VJOULE_DIR + "signal", "w")
            f.write ("1\n");
            f.close ()
            self.__waitIteration ()

    def __waitIteration (self):
        while True :
            for event in self._inotifFd.event_gen (yield_nones=False):
                (_,type_names,path,filename) = event
                if (filename == "cpu") :
                    return
    
    def dispose (self):
        for i in self._groups :
            self._groups[i].dispose ()
        
        pass

class VJouleProcessGroup:

    def __init__ (self, context, name) :
        self._context = context
        self._name = name

    def isMonitored (self):
        return os.path.isfile (self._context._VJOULE_DIR + "results/" + self._name + "/cpu")

    def getName (self):
        return self._name
    
    def getCurrentConsumption (self) :
        ret = VJouleConsumptionStamp (time.time ())
        if (os.path.isfile (self._context._VJOULE_DIR + "results/vjoule_api.slice/" + self._name + "/cpu")) :
            i = open (self._context._VJOULE_DIR + "results/vjoule_api.slice/" + self._name + "/cpu", "r")
            ret.cpu = float (i.read ())
            i.close ()

        if (os.path.isfile (self._context._VJOULE_DIR + "results/vjoule_api.slice/" + self._name + "/ram")) :
            i = open (self._context._VJOULE_DIR + "results/vjoule_api.slice/" + self._name + "/ram", "r")
            ret.ram = float (i.read ())
            i.close ()

        if (os.path.isfile (self._context._VJOULE_DIR + "results/vjoule_api.slice/" + self._name + "/gpu")) :
            i = open (self._context._VJOULE_DIR + "results/vjoule_api.slice/" + self._name + "/gpu", "r")
            ret.gpu = float (i.read ())
            i.close ()
            
        return ret
    
    def dispose (self) :
        ret = subprocess.run (["vjoule_cgutils", "del", ("vjoule_api.slice/" + self._name)])
        if (ret.returncode != 0) :
            raise VJouleError ("failed to delete cgroup. make sure the current user is in group 'vjoule'.")

class VJouleConsumptionStamp :

    def __init__ (self, time) :
        self.time = time
        self.cpu = 0
        self.ram = 0
        self.gpu = 0

    def __sub__ (self, other) :
        return VJouleConsumptionDiff (
            self.time - other.time,
            self.cpu - other.cpu,
            self.ram - other.ram,
            self.gpu - other.gpu
        )

    def __str__ (self):
        return "stamp (time: {:.2f}, cpu: {:.2f}J, ram: {:.2f}J, gpu: {:.2f}J)".format (self.time, self.cpu, self.ram, self.gpu)



class VJouleConsumptionDiff :

    def __init__ (self, time, cpu, ram, gpu):
        self.time = time
        self.cpu = cpu
        self.ram = ram
        self.gpu = gpu

    def __mod__ (self, other) :
        return VJouleConsumptionPerc (
            self.time / other.time * 100,
            self.cpu / max (1, other.cpu) * 100,
            self.ram / max (1, other.ram) * 100,
            self.gpu / max (1, other.gpu) * 100
        )            
        
    def __str__ (self):
        return "diff (time: {:.2f}s, cpu: {:.2f}J, ram{:.2f}J, gpu: {:.2f}J)".format (self.time, self.cpu, self.ram, self.gpu)


class VJouleConsumptionPerc :

    def __init__ (self, time, cpu, ram, gpu):
        self.time = time
        self.cpu = cpu
        self.ram = ram
        self.gpu = gpu

        
    def __str__ (self):
        return "diff (time: {:.2f}%, cpu: {:.2f}%, ram{:.2f}%, gpu: {:.2f}%)".format (self.time, self.cpu, self.ram, self.gpu)


    
