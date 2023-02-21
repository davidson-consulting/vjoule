import inotify.adapters
import os
import subprocess
import sys
import time  

class VJouleError (Exception):
    """
    exception thrown when an error occurs in the vJoule API
    """
    
    def __init__ (self, msg) :
        self.msg = msg


class VJouleAPI:
    """
    Main class of the vjoule api, managing the group creation and deletion
    This is also used to retreive the consumption of the whole machine
    
    Examples
    ---------
    api = VJouleAPI ()
    
    # Find the process group containing the current pid
    pg = api.getGroup ("this")     

    # Vjoule API modifies the system in which the py app is running
    # So it is really important to clean everything the API has created
    # api.dispose ()
    """
    
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
            if (l == "vjoule_api.slice/*\n") :
                found = True
                break

        if not found :
            o = open (self._VJOULE_DIR + "cgroups", "a+")
            o.write ("vjoule_api.slice/*\n")
            o.close ()

        self.__forceSig ()


    def getCurrentMachineConsumption (self) :
        """
        Communicate with the vJoule service to perform a service iteration and get the energy consumption of the whole machine

        Returns
        -------        
        The machine consumption since the start of the vjoule service               
        """
        
        self.__forceSig ()
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
        """
        Find a registered group, or a cgroup that already exist in the system
        
        Returns
        -------
        A VJouleProcessGroup

        Raise
        -----
        VJouleError, if the group does not exist
        """
        
        if (name in self._groups) :
            return self._groups[name]
        else :
            i = open (self._VJOULE_DIR + "cgroups", 'r')
            found = False
            for l in i.readlines ():
                if (l == name + "\n") :
                    found = True
                    break
            
            if not found :
                o = open (self._VJOULE_DIR + "cgroups", 'a+')
                o.write (name + "\n")
                o.close ()
                
            self.__forceSig ()
            
            for i in range (2) :
                if (os.path.isfile (self._VJOULE_DIR + "results/" + name + "/cpu")) :
                    ret = VJouleProcessGroup (self, name, noSlice = True, didAdd = not found)
                    self._groups[name] = ret
                    return ret
                time.sleep (0.01)
            
            raise VJouleError ("no process group named '" + name + "' is monitored.")


    def createGroup (self, name, pids) :
        """
        Create a process group from a list of Pids
        
        Warning
        --------
        Uses the system cgroup to register the newly created group, all pids that were previously in another cgroup are moved
        To avoid that, you can still use getGroup using the name of the cgroup you want to watch
        
        Returns
        -------
        A VJouleProcessGroup

        Parameters
        ----------
        name : str, the name of the process group (will be located in the cgroup vjoule_api.slice/${name}
        pids : list, the list of pids
        
        Raises
        ------
        VJouleError, if the creation failed
        """
        
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
        """
        Forget a group 
        """
        
        del self._groups[name]
        
    def __forceSig (self):
        """
        Force the service to perform an iteration

        """
        
        for i in range (2) :
            f = open (self._VJOULE_DIR + "signal", "w")
            f.write ("1\n");
            f.close ()
            self.__waitIteration ()

    def __waitIteration (self):
        """
        Wait for an iteration in the service
        """
        
        while True :
            for event in self._inotifFd.event_gen (yield_nones=False):
                (_,type_names,path,filename) = event
                if (filename == "cpu") :
                    return
    
    def dispose (self):
        """
        Clean the API, and all the element created by the API
        """
        
        for i in self._groups :
            self._groups[i].dispose ()
        
        pass

class VJouleProcessGroup:
    """
    A vjoule process group is a system cgroup that is watched by the vjoule_service
    
    """

    def __init__ (self, context, name, noSlice = False, didAdd = False) :
        """
        Warning
        --------
        the constructor should be called only by the VJouleAPI class
        
        Parameters
        -----------
        context: the VJouleAPI that created the process group
        name: the name of the process group
        noSlice: False iif the group is located in vjoule_api.slice
        didAdd: True iif the VJouleAPI told to the vjoule_service to monitor the group
        """
        
        self._VJOULE_DIR = "/etc/vjoule/"
        self._context = context
        self._name = name
        self._noSlice = noSlice
        self._didAdd = didAdd
        self._resultLoc = ("results/" + self._name) if (self._noSlice) else ("results/vjoule_api.slice/" + self._name)

    def isMonitored (self):
        """
        Returns
        -------
        Should be True, checking that the vjoule_service is watching the process group
        """
        return os.path.isfile (self._context._VJOULE_DIR + self._resultLoc + "/cpu")

    def getName (self):
        """
        Returns
        -------
        The name of the process group
        """
        return self._name
    
    def getCurrentConsumption (self) :
        """
        Communicate with the vJoule service to perform a service iteration and get the energy consumption of the process group 

        Returns
        -------        
        The process consumption estimation since the start of the monitoring               
        """
        
        self._context._VJouleAPI__forceSig ()
        ret = VJouleConsumptionStamp (time.time ())
        if (os.path.isfile (self._context._VJOULE_DIR + self._resultLoc + "/cpu")) :
            i = open (self._context._VJOULE_DIR + self._resultLoc + "/cpu", "r")
            ret.cpu = float (i.read ())
            i.close ()

        if (os.path.isfile (self._context._VJOULE_DIR + self._resultLoc + "/ram")) :
            i = open (self._context._VJOULE_DIR + self._resultLoc + "/ram", "r")
            ret.ram = float (i.read ())
            i.close ()

        if (os.path.isfile (self._context._VJOULE_DIR + self._resultLoc + "/gpu")) :
            i = open (self._context._VJOULE_DIR + self._resultLoc + "/gpu", "r")
            ret.gpu = float (i.read ())
            i.close ()
            
        return ret
    
    def dispose (self) :
        """
        Clean the process group and all the element that were created for it
        
        Warning
        -------
        This function should be called only by the VJouleAPI class
        """
        
        if (not self._noSlice) : 
            ret = subprocess.run (["vjoule_cgutils", "del", ("vjoule_api.slice/" + self._name)])
            if (ret.returncode != 0) :
                raise VJouleError ("failed to delete cgroup. make sure the current user is in group 'vjoule'.")
        elif self._didAdd:
            i = open (self._VJOULE_DIR + "cgroups", 'r')
            add = ""
            for l in i.readlines ():
                if (l != (self._name + "\n")) :                    
                    add = add + l

            o = open (self._VJOULE_DIR + "cgroups", "w")
            o.write (add)
            o.close ()

class VJouleConsumptionStamp :
    """
    A consumption stamp is the consumption read from the vJoule service at a given timestamp
    It contains four elements : 
        - time: the timestamp
        - cpu: the consumption in joule since the start of the monitoring
        - ram: the consumption in joule since the start of the monitoring
        - gpu: the consumption in joule since the start of the monitoring
    """
    
    def __init__ (self, time) :
        self.time = time
        self.cpu = 0
        self.ram = 0
        self.gpu = 0

    def __sub__ (self, other) :
        """
        Returns
        -------
        A VJouleConsumptionDiff
        """
        
        return VJouleConsumptionDiff (
            self.time - other.time,
            self.cpu - other.cpu,
            self.ram - other.ram,
            self.gpu - other.gpu
        )

    def __str__ (self):
        return "stamp (time: {:.2f}, cpu: {:.2f}J, ram: {:.2f}J, gpu: {:.2f}J)".format (self.time, self.cpu, self.ram, self.gpu)



class VJouleConsumptionDiff :
    """
    A consumption diff is the difference between two consumption stamp
    It contains four elements : 
        - time: the duration between the two stamps
        - cpu: the consumption in joule between the two stamps
        - ram: the consumption in joule between the two stamps 
        - gpu: the consumption in joule between the two stamps 
    """ 
    
    def __init__ (self, time, cpu, ram, gpu):
        self.time = time
        self.cpu = cpu
        self.ram = ram
        self.gpu = gpu

    def __mod__ (self, other) :
        """
        Parameters
        ----------
        other: VJouleConsumptionDiff
        
        Returns
        -------
        The percentage of 'self' over 'other'
        """
        
        return VJouleConsumptionPerc (
            self.time / other.time * 100,
            self.cpu / max (1, other.cpu) * 100,
            self.ram / max (1, other.ram) * 100,
            self.gpu / max (1, other.gpu) * 100
        )            
        
    def __str__ (self):
        return "diff (time: {:.2f}s, cpu: {:.2f}J, ram{:.2f}J, gpu: {:.2f}J)".format (self.time, self.cpu, self.ram, self.gpu)


class VJouleConsumptionPerc :
    """
    A consumption perc is the percentage between two consumption diffs
    It contains four elements : 
        - time: the percentage of duration 
        - cpu: the percentage of cpu consumption
        - ram: the percentage of ram consumption
        - gpu: the percentage of gpu consumption
    """ 
    
    def __init__ (self, time, cpu, ram, gpu):
        self.time = time
        self.cpu = cpu
        self.ram = ram
        self.gpu = gpu

        
    def __str__ (self):
        return "diff (time: {:.2f}%, cpu: {:.2f}%, ram{:.2f}%, gpu: {:.2f}%)".format (self.time, self.cpu, self.ram, self.gpu)


    
