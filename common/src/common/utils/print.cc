#include <common/utils/print.hh>

std::ostream & operator<< (std::ostream & stream, const std::vector <char> & vec) {
    stream << "[";
    for (int i = 0 ; i < vec.size () ;i++) {
	if (i != 0) stream << ", ";
	stream << vec[i] << '[' << (int) (vec [i]) << ']';
    }
    stream << "]";
    return stream;
}


namespace common::utils {

    std::string strip_string (const std::string & str) {
	std::string s = str;
    
	auto pos = s.find_first_not_of (" \n\r\t");    
	if (pos != std::string::npos) s = s.substr (pos);

	pos = s.find_last_not_of(" \n\r\t");
	if (pos != std::string::npos) s.erase(pos + 1);		

	return s;
    }

    std::string findAndReplaceAll(const std::string & input, const std::string & toSearch, const std::string & replaceStr)  {
	std::string data = input;
	
	size_t pos = data.find(toSearch);
	while( pos != std::string::npos) {
	    data.replace(pos, toSearch.size(), replaceStr);
	    pos = data.find(toSearch, pos + replaceStr.size());
	}
	
	return data;
    }


    std::vector <std::string> split_string (const std::string & in, const std::string & splitter) {	
	size_t pos = 0;
	std::vector <std::string> result ;
	std::string s = in;
	while ((pos = s.find(splitter)) != std::string::npos) {
	    auto token = s.substr (0, pos);
	    if (token != splitter && token.size () != 0) {
		result.push_back (token);
	    }
	    
	    s.erase(0, pos + splitter.length());
	}

	if (s.size () != 0) {
	    result.push_back (strip_string (s));
	}
	
	return result;
    }


    bool is_prefix (const std::string & word, const std::string & str) {
	if (word.size () > str.size ()) return false;
	return std::equal (word.begin (), word.end (), str.begin ());
    }
    
    
}
