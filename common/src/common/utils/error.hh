#pragma once

#include <iostream>

namespace common::utils {

  class CommonError {

    // The error message of the error
    std::string _msg;

  public:

    /**
     * Create an error with a specific message
     */
    CommonError(const std::string& msg);

    /**
     * @returns: the error message of the error
     */
    const std::string& getMessage() const;

    virtual ~CommonError();

  };

}


