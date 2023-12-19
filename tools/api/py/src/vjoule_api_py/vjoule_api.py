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

        if (os.path.isfile (self._VJOULE_DIR + "results/pdu_energy")) :
            i = open (self._VJOULE_DIR + "results/pdu_energy", "r")
            ret.pdu = float (i.read ())
            i.close ()

        if (os.path.isfile (self._VJOULE_DIR + "results/pdu_power")) :
            with open (self._VJOULE_DIR + "results/pdu_power", "r") as f:
                x = f.read ()
                ret.pdu_watts = float (x)

        return ret

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
    

class VJouleConsumptionStamp :
    """
    A consumption stamp is the consumption read from the vJoule service at a given timestamp
    It contains four elements : 
        - time: the timestamp
        - pdu: the consumption in joule since the start of the monitoring
        - cpu: the consumption in joule since the start of the monitoring
        - ram: the consumption in joule since the start of the monitoring
        - gpu: the consumption in joule since the start of the monitoring
    """
    
    def __init__ (self, time) :
        self.time = time
        self.pdu_watts = 0
        self.pdu = 0
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
            self.pdu - other.pdu,
            self.cpu - other.cpu,
            self.ram - other.ram,
            self.gpu - other.gpu
        )

    def __str__ (self):
        return "stamp (time: {:.2f}, pdu: {:.2f}J, cpu: {:.2f}J, ram: {:.2f}J, gpu: {:.2f}J)".format (self.time, self.pdu, self.cpu, self.ram, self.gpu)



class VJouleConsumptionDiff :
    """
    A consumption diff is the difference between two consumption stamp
    It contains four elements : 
        - time: the duration between the two stamps
        - pdu: the consumption in joule between the two stamps
        - cpu: the consumption in joule between the two stamps
        - ram: the consumption in joule between the two stamps 
        - gpu: the consumption in joule between the two stamps 
    """ 
    
    def __init__ (self, time, pdu, cpu, ram, gpu):
        self.time = time
        self.pdu = pdu
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
            self.pdu / max (1, other.pdu) * 100,
            self.cpu / max (1, other.cpu) * 100,
            self.ram / max (1, other.ram) * 100,
            self.gpu / max (1, other.gpu) * 100
        )            
        
    def __str__ (self):
        return "diff (time: {:.2f}s, pdu: {:.2f}J, cpu: {:.2f}J, ram{:.2f}J, gpu: {:.2f}J)".format (self.time, self.pdu, self.cpu, self.ram, self.gpu)


class VJouleConsumptionPerc :
    """
    A consumption perc is the percentage between two consumption diffs
    It contains four elements : 
        - time: the percentage of duration 
        - cpu: the percentage of cpu consumption
        - ram: the percentage of ram consumption
        - gpu: the percentage of gpu consumption
    """ 
    
    def __init__ (self, time, pdu, cpu, ram, gpu):
        self.time = time
        self.pdu = pdu
        self.cpu = cpu
        self.ram = ram
        self.gpu = gpu

        
    def __str__ (self):
        return "diff (time: {:.2f}%, pdu: {:.2f}%, cpu: {:.2f}%, ram{:.2f}%, gpu: {:.2f}%)".format (self.time, self.pdu, self.cpu, self.ram, self.gpu)
