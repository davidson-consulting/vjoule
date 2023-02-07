import os
from vjoule_api_py.vjoule_api import *

def computePi(prec):
    res = 0
    for i in range(prec):
        res += (4.0 / prec) / (1.0 + ((i - 0.5) * (1.0 / prec)) * ((i - 0.5) * (1.0 / prec)))
    return res


def evalPi () : 
    api = VJouleAPI ()
    pg = api.getGroup ("this")

    m_beg = api.getCurrentMachineConsumption ()
    p_beg = pg.getCurrentConsumption ()

    computePi (10000000)

    m_end = api.getCurrentMachineConsumption ()
    p_end = pg.getCurrentConsumption ()

    m_diff = m_end - m_beg
    p_diff = p_end - p_beg

    print (p_diff)
    print (m_diff)
    print (p_diff % m_diff)

    api.dispose ()

    
def evalExtern (pids):

    api = VJouleAPI ()
    pg = api.createGroup ("extern", pids)
    
    m_beg = api.getCurrentMachineConsumption ()
    p_beg = pg.getCurrentConsumption ()

    time.sleep (5)

    m_end = api.getCurrentMachineConsumption ()
    p_end = pg.getCurrentConsumption ()

    m_diff = m_end - m_beg
    p_diff = p_end - p_beg

    print (p_diff)
    print (m_diff)
    print (p_diff % m_diff)

    api.dispose ()    



if __name__ == "__main__":
    #evalPi ()
    print ("===")
    evalExtern ([281717, 281718])
