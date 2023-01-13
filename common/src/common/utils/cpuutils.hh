#pragma once

namespace sensor::perf {

    /**
     * @returns: the cpu maximal frequency
     */
    float getMaxFrequency ();
    
    /**
     * @returns: the cpu minimal frequency
     */
    float getMinFrequency ();

    /**
     * @returns: the cpu base frequency
     */
    float getBaseFrequency ();
    

}
