#pragma once

namespace common::utils {

    /**
     * Run the sudo command so the process become a root process
     */
    void becomeSudo ();

    /**
     * Verify that the user has the correct sudo privileges       
     */
    void authenticateSudo ();
    
}
