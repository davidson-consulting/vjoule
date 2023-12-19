import os
from vjoule_api_py.vjoule_api import *

def computePi(prec):
    res = 0
    for i in range(prec):
        res += (4.0 / prec) / (1.0 + ((i - 0.5) * (1.0 / prec)) * ((i - 0.5) * (1.0 / prec)))
    return res

def evalPi () : 
    api = VJouleAPI ()

    m_beg = api.getCurrentMachineConsumption ()

    pi = computePi (10000000)

    m_end = api.getCurrentMachineConsumption ()

    m_diff = m_end - m_beg

    print ("PI : ", pi)
    print (m_diff)

    
if __name__ == "__main__":
    evalPi ()
