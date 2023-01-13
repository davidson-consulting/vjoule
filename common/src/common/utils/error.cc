#include <common/utils/error.hh>

namespace common::utils {

  CommonError::CommonError(const std::string& msg) : _msg(msg) 
  {}

  const std::string & CommonError::getMessage() const {
    return this->_msg;
  }

  CommonError::~CommonError () {}
			
}


