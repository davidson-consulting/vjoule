#include <common/utils/config.hh>
#include <sstream>
#include <istream>
#include <fstream>
#include <common/utils/files.hh>

namespace common::utils {

    using namespace std;


    namespace config {


	/***
	 * ========================================================================
	 * ========================================================================
	 * =========================          array           =====================
	 * ========================================================================
	 * ========================================================================
	 */

	array::array() {}

	array::array(const array& other) {
	    other.clone_content(this->_types, this->_content);
	}

	const array& array::operator= (const array& other) {
	    this->clear();
	    other.clone_content(this->_types, this->_content);
	    return other;
	}

	int array::size() const {
	    return this->_content.size();
	}

	int array::get_int(unsigned int i) const {
	    if (i >= this->_content.size()) {
		std::stringstream ss;
		ss << "Out of array : " << i << " > " << this->_content.size();
		throw config_error(0, ss.str());
	    }
	    else if (this->_types[i] == typeid (float).name()) {
		return static_cast <int> (*(reinterpret_cast <float*> (this->_content[i])));
	    }
	    else if (this->_types[i] != typeid (int).name()) {
		throw config_error(0, "Incompatible types : " + this->_types[i] + "  and " + typeid (int).name());
	    }
	    return *(reinterpret_cast <int*> (this->_content[i]));
	}

	float array::get_float(unsigned int i) const {
	    if (i >= this->_content.size()) {
		std::stringstream ss;
		ss << "Out of array : " << i << " > " << this->_content.size();
		throw config_error(0, ss.str());
	    }
	    else if (this->_types[i] == typeid (int).name()) {
		return static_cast <float> (*(reinterpret_cast <int*> (this->_content[i])));
	    }
	    else if (this->_types[i] != typeid (float).name()) {
		throw config_error(0, "Incompatible types : " + this->_types[i] + "  and " + typeid (float).name());
	    }
	    return *(reinterpret_cast <float*> (this->_content[i]));
	}


	void array::print(std::ostream& stream) const {
	    stream << "[";
	    for (auto i : utils::range(0, this->_content.size())) {
		if (i != 0)
		stream << ", ";
		if (this->_types[i] == typeid (float).name()) {
		    stream << "float(" << *(reinterpret_cast<float*> (this->_content[i])) << ")";
		}
		else if (this->_types[i] == typeid (int).name()) {
		    stream << "int(" << *(reinterpret_cast<int*> (this->_content[i])) << ")";
		}
		else if (this->_types[i] == typeid (std::string).name()) {
		    stream << "str(" << *(reinterpret_cast<std::string*> (this->_content[i])) << ")";
		}
		else if (this->_types[i] == typeid (bool).name()) {
		    stream << "bool(" << *(reinterpret_cast<bool*> (this->_content[i])) << ")";
		}
		else if (this->_types[i] == typeid (array).name()) {
		    (reinterpret_cast<array*> (this->_content[i]))->print(stream);
		}
		else if (this->_types[i] == typeid (dict).name()) {
		    (reinterpret_cast<dict*> (this->_content[i]))->print(stream);
		}
		else {
		    stream << "unmanaged";
		}
	    }
	    stream << "]";
	}

	void array::format (std::ostream& stream) const {
	    stream << "[";
	    for (auto i : utils::range(0, this->_content.size())) {
		if (i != 0)
		stream << ", ";
		if (this->_types[i] == typeid (float).name()) {
		    stream << *(reinterpret_cast<float*> (this->_content[i]));
		}
		else if (this->_types[i] == typeid (int).name()) {
		    stream << *(reinterpret_cast<int*> (this->_content[i]));
		}
		else if (this->_types[i] == typeid (std::string).name()) {
		    stream << "\"" << *(reinterpret_cast<std::string*> (this->_content[i])) << "\"";
		}
		else if (this->_types[i] == typeid (bool).name()) {
		    if (*(reinterpret_cast<bool*> (this->_content[i]))) {
			stream << "true" ;
		    } else stream << "false";
		}
		else if (this->_types[i] == typeid (array).name()) {
		    (reinterpret_cast<array*> (this->_content[i]))->format(stream);
		}
		else if (this->_types[i] == typeid (dict).name()) {
		    (reinterpret_cast<dict*> (this->_content[i]))->format(stream, 2);
		}
	    }
	    stream << "]";

	}

	void array::push(void* value, const std::string& type) {
	    this->_types.push_back(type);
	    this->_content.push_back(value);
	}

	void array::clone_content(std::vector <std::string>& types, std::vector <void*>& content) const {
	    content.reserve(this->_content.size());
	    types.reserve(this->_content.size());
	    for (auto i : utils::range(0, this->_content.size())) {
		types.push_back(this->_types[i]);
		if (this->_types[i] == typeid (float).name()) {
		    content.push_back(new float(*(reinterpret_cast<float*> (this->_content[i]))));
		}
		else if (this->_types[i] == typeid (int).name()) {
		    content.push_back(new int(*(reinterpret_cast<int*> (this->_content[i]))));
		}
		else if (this->_types[i] == typeid (std::string).name()) {
		    content.push_back(new std::string(*(reinterpret_cast<std::string*> (this->_content[i]))));
		}
		else if (this->_types[i] == typeid (bool).name()) {
		    content.push_back(new bool(*(reinterpret_cast<bool*> (this->_content[i]))));
		}
		else if (this->_types[i] == typeid (array).name()) {
		    content.push_back(new array(*(reinterpret_cast<array*> (this->_content[i]))));
		}
		else if (this->_types[i] == typeid (dict).name()) {
		    content.push_back(new dict(*(reinterpret_cast<dict*> (this->_content[i]))));
		}
	    }
	}

	void array::clear() {
	    for (auto i : utils::range(0, this->_content.size())) {
		if (this->_types[i] == typeid (std::string).name()) {
		    (reinterpret_cast<std::string*> (this->_content[i]))->~string();
		}
		else if (this->_types[i] == typeid (array).name()) {
		    (reinterpret_cast<array*> (this->_content[i]))->clear();
		}
		else if (this->_types[i] == typeid (dict).name()) {
		    (reinterpret_cast<dict*> (this->_content[i]))->clear();
		}

		delete this->_content[i];
	    }

	    this->_types.clear();
	    this->_content.clear();
	}


	array::~array() {
	    this->clear();
	}


	/***
	 * ========================================================================
	 * ========================================================================
	 * =========================          dict            =====================
	 * ========================================================================
	 * ========================================================================
	 */

	dict::dict() {
	}

	dict::dict(const dict& other) {
	    other.clone_content(this->_types, this->_content);
	}

	const dict& dict::operator= (const dict& other) {
	    this->clear();
	    other.clone_content(this->_types, this->_content);

	    return other;
	}

	std::vector <std::string> dict::keys() const {
	    std::vector <std::string> keys;
	    for (auto& it : this->_types) {
		keys.push_back(it.first);
	    }
	    return keys;
	}

	void dict::insert(const std::string& name, void* value, const std::string& type) {
	    this-> erase (name);
	    
	    this->_types[name] = type;
	    this->_content[name] = value;
	}

	int dict::get_int(const std::string& name) const {
	    auto value = this->_content.find(name);
	    if (value == this->_content.end()) {
		std::stringstream ss;
		ss << "Name : [" << name << "] not found in dictionnary ";
		this->print(ss);
		throw config_error(0, ss.str());
	    }
	    else if (this->_types.find(name)->second == typeid (float).name()) {
		return static_cast<int> (*(reinterpret_cast<float*> (value->second)));
	    }
	    else if (this->_types.find(name)->second != typeid (int).name()) {
		throw config_error(0, "Incompatible types : " + this->_types.find(name)->second + " and " + std::string(typeid (int).name()) + " for index [" + name + "]");
	    }
	    else {
		return *(reinterpret_cast<int*> (value->second));
	    }
	}

	float dict::get_float(const std::string& name) const {
	    auto value = this->_content.find(name);
	    if (value == this->_content.end()) {
		std::stringstream ss;
		ss << "Name : [" << name << "] not found in dictionnary ";
		this->print(ss);
		throw config_error(0, ss.str());
	    }
	    else if (this->_types.find(name)->second == typeid (int).name()) {
		return static_cast<float> (*(reinterpret_cast<int*> (value->second)));
	    }
	    else if (this->_types.find(name)->second != typeid (float).name()) {
		throw config_error(0, "Incompatible types : " + this->_types.find(name)->second + " and " + std::string(typeid (int).name()) + " for index [" + name + "]");
	    }
	    else {
		return *(reinterpret_cast<float*> (value->second));
	    }
	}

	bool dict::has_int(const std::string& name) const {
	    auto value = this->_content.find(name);
	    if (value == this->_content.end()) return false;
	    return (this->_types.find(name)->second == typeid (int).name() || this->_types.find(name)->second == typeid (float).name());
	}

	bool dict::has_float(const std::string& name) const {
	    auto value = this->_content.find(name);
	    if (value == this->_content.end()) return false;
	    return (this->_types.find(name)->second == typeid (int).name() || this->_types.find(name)->second == typeid (float).name());
	}

	int dict::getOr_int(const std::string& name, int orVal) const {
	    auto value = this->_content.find(name);
	    if (value == this->_content.end()) return orVal;
	    else if (this->_types.find(name)->second == typeid (float).name()) {
		return static_cast<int> (*(reinterpret_cast<float*> (value->second)));
	    }
	    else if (this->_types.find(name)->second != typeid (int).name()) {
		throw config_error(0, "Incompatible types : " + this->_types.find(name)->second + " and " + std::string(typeid (int).name()) + " for index [" + name + "]");
	    }
	    else {
		return *(reinterpret_cast<int*> (value->second));
	    }
	}

	float dict::getOr_float(const std::string& name, float orVal) const {
	    auto value = this->_content.find(name);
	    if (value == this->_content.end()) return orVal;
	    else if (this->_types.find(name)->second == typeid (int).name()) {
		return static_cast<float> (*(reinterpret_cast<int*> (value->second)));
	    }
	    else if (this->_types.find(name)->second != typeid (float).name()) {
		throw config_error(0, "Incompatible types : " + this->_types.find(name)->second + " and " + std::string(typeid (float).name()) + " for index [" + name + "]");
	    }
	    else {
		return *(reinterpret_cast<float*> (value->second));
	    }
	}



	void dict::print(std::ostream& stream) const {
	    stream << "{";
	    int j = 0;
	    for (auto i : this->_content) {
		if (j != 0)
		stream << ", ";
		j += 1;
		if (this->_types.find(i.first)->second == typeid (float).name()) {
		    stream << i.first << "=> float(" << *(reinterpret_cast<float*> (i.second)) << ")";
		}
		else if (this->_types.find(i.first)->second == typeid (int).name()) {
		    stream << i.first << "=> int(" << *(reinterpret_cast<int*> (i.second)) << ")";
		}
		else if (this->_types.find(i.first)->second == typeid (std::string).name()) {
		    stream << i.first << "=> str(" << *(reinterpret_cast<std::string*> (i.second)) << ")";
		}
		else if (this->_types.find(i.first)->second == typeid (bool).name()) {
		    stream << i.first << "=> bool(" << *(reinterpret_cast<bool*> (i.second)) << ")";
		}
		else if (this->_types.find(i.first)->second == typeid (array).name()) {
		    stream << i.first << "=> ";
		    (reinterpret_cast<array*> (i.second))->print(stream);
		}
		else if (this->_types.find(i.first)->second == typeid (dict).name()) {
		    stream << i.first << "=> ";
		    (reinterpret_cast<dict*> (i.second))->print(stream);
		}
		else {
		    stream << "unmanaged";
		}
	    }
	    stream << "}";
	}

	void dict::format (std::ostream & stream, int global) const {
	    if (global == 0) {
		for (auto i : this->_content) {
		    stream << "[" << i.first << "]" << std::endl;
		    if (this->_types.find(i.first)->second == typeid (dict).name()) {
			(reinterpret_cast<dict*> (i.second))->format(stream, 1);
		    }
		    else {
			stream << "unmanaged";
		    }
		    stream << std::endl;
		}
	    } else if (global == 1) {
		for (auto i : this->_content) {
		    stream << i.first << " = ";
		    if (this->_types.find(i.first)->second == typeid (float).name()) {
			stream << *(reinterpret_cast<float*> (i.second));
		    }
		    else if (this->_types.find(i.first)->second == typeid (int).name()) {
			stream << *(reinterpret_cast<int*> (i.second));
		    }
		    else if (this->_types.find(i.first)->second == typeid (std::string).name()) {
			stream << "\"" << *(reinterpret_cast<std::string*> (i.second)) << "\"" ;
		    }
		    else if (this->_types.find(i.first)->second == typeid (bool).name()) {
			if (*(reinterpret_cast<bool*> (i.second))) {
			    stream << "true";
			} else stream << "false";
		    }
		    else if (this->_types.find(i.first)->second == typeid (array).name()) {
			(reinterpret_cast<array*> (i.second))->format (stream);
		    }
		    else if (this->_types.find(i.first)->second == typeid (dict).name()) {
			(reinterpret_cast<dict*> (i.second))->format (stream, 2);
		    }
		    else {
			stream << "unmanaged";
		    }
		    stream << std::endl;
		}
	    } else {
		stream << "{";
		int j = 0;
		for (auto i : this->_content) {
		    if (j != 0)
		    stream << ", ";
		    j += 1;
		    if (this->_types.find(i.first)->second == typeid (float).name()) {
			stream << i.first << "=> float(" << *(reinterpret_cast<float*> (i.second)) << ")";
		    }
		    else if (this->_types.find(i.first)->second == typeid (int).name()) {
			stream << i.first << "=> int(" << *(reinterpret_cast<int*> (i.second)) << ")";
		    }
		    else if (this->_types.find(i.first)->second == typeid (std::string).name()) {
			stream << i.first << "=> str(" << *(reinterpret_cast<std::string*> (i.second)) << ")";
		    }
		    else if (this->_types.find(i.first)->second == typeid (bool).name()) {
			stream << i.first << "=> bool(" << *(reinterpret_cast<bool*> (i.second)) << ")";
		    }
		    else if (this->_types.find(i.first)->second == typeid (array).name()) {
			stream << i.first << "=> ";
			(reinterpret_cast<array*> (i.second))->format (stream);
		    }
		    else if (this->_types.find(i.first)->second == typeid (dict).name()) {
			stream << i.first << "=> ";
			(reinterpret_cast<dict*> (i.second))->format (stream, 2);
		    }
		    else {
			stream << "unmanaged";
		    }
		}
		stream << "}";
	    }
	}

	void dict::clone_content(std::map <std::string, std::string>& types, std::map <std::string, void*>& content) const {
	    for (auto it : this->_content) {
		types[it.first] = this->_types.find(it.first)->second;
		if (this->_types.find(it.first)->second == typeid (float).name()) {
		    content[it.first] = new float(*(reinterpret_cast<float*> (it.second)));
		}
		else if (this->_types.find(it.first)->second == typeid (int).name()) {
		    content[it.first] = new int(*(reinterpret_cast<int*> (it.second)));
		}
		else if (this->_types.find(it.first)->second == typeid (std::string).name()) {
		    content[it.first] = new std::string(*(reinterpret_cast<std::string*> (it.second)));
		}
		else if (this->_types.find(it.first)->second == typeid (bool).name()) {
		    content[it.first] = new bool(*(reinterpret_cast<bool*> (it.second)));
		}
		else if (this->_types.find(it.first)->second == typeid (array).name()) {
		    content[it.first] = new array(*(reinterpret_cast<array*> (it.second)));
		}
		else if (this->_types.find(it.first)->second == typeid (dict).name()) {
		    content[it.first] = new dict(*(reinterpret_cast<dict*> (it.second)));
		}
	    }
	}

	void dict::erase (const std::string & name) {
	    auto cnd = this-> _content.find (name);
	    if (cnd != this-> _content.end ()) {
		auto fnd = this-> _types.find (name);
		if (fnd->second == typeid (std::string).name()) {
		    (reinterpret_cast<std::string*> (cnd-> second))->~string();
		}
		else if (fnd->second == typeid (array).name()) {
		    (reinterpret_cast<array*> (cnd-> second))->clear();
		}
		else if (fnd->second == typeid (dict).name()) {
		    (reinterpret_cast<dict*> (cnd-> second))->clear();
		}

		delete cnd-> second;
	    }
	    this-> _types.erase (name);
	    this-> _content.erase (name);
	}

	void dict::clear() {
	    for (auto& it : this->_content) {
		if (this->_types.find(it.first)->second == typeid (std::string).name()) {
		    (reinterpret_cast<std::string*> (it.second))->~string();
		}
		else if (this->_types.find(it.first)->second == typeid (array).name()) {
		    (reinterpret_cast<array*> (it.second))->clear();
		}
		else if (this->_types.find(it.first)->second == typeid (dict).name()) {
		    (reinterpret_cast<dict*> (it.second))->clear();
		}

		delete it.second;
	    }

	    this->_types.clear();
	    this->_content.clear();
	}

	dict::~dict() {
	    this->clear();
	}

    }


    /***
     * ========================================================================
     * ========================================================================
     * =========================         parser           =====================
     * ========================================================================
     * ========================================================================
     */


    void* parse_value(tokenizer& tok, std::string& type);

    config::dict* parse_dict(tokenizer& tok, bool glob = false);

    config::array* parse_array(tokenizer& tok);

    int* parse_int(tokenizer& tok);

    float* parse_float(tokenizer& tok);

    std::string* parse_string(tokenizer& tok, char beginChar = '"');

    bool* parse_bool(tokenizer& tok);


    void assert_msg(bool test, const std::string& msg, tokenizer& tok) {
	if (!test) {
	    tok.rewind();
	    throw config::config_error(tok.getLineNumber(), msg + " at line : " + std::to_string(tok.getLineNumber()));
	}
    }

    config::dict* parse_dict(tokenizer& tok, bool glob) {
	config::dict* dic = new config::dict();
	while (true) {
	    auto name = tok.next();
	    if (name == "" || name == "[") {
		tok.rewind();
		break;
	    }

	    assert_msg(tok.next() == "=", "expected = ", tok);

	    std::string type;
	    auto value = parse_value(tok, type);
	    dic->insert(name, value, type);

	    if (!glob) {
		auto next = tok.next();
		assert_msg(next == "}" || next == ",", "expected } or , (not " + next + ")", tok);
		if (next == "}") break;
	    }
	}
	return dic;
    }

    config::array* parse_array(tokenizer& tok) {
	config::array* arr = new config::array();
	while (true) {
	    if (tok.next() != "]") {
		std::string type;
		tok.rewind();
		auto value = parse_value(tok, type);
		arr->push(value, type);
		auto t = tok.next();
		assert_msg(t == "," || t == "]", "expected ,", tok);
		if (t == "]") break;
	    }
	    else break;
	}

	return arr;
    }

    int* parse_int(tokenizer& tok) {
	auto x = tok.next();
	auto* i = new  int(std::stoi(x));
	return i;
    }

    float* parse_float(tokenizer& tok) {
	auto x = tok.next();
	auto* i = new float(std::stof(x));
	return i;
    }

    std::string* parse_string(tokenizer& tok, char end) {
	tok.skip(false);
	std::stringstream ss;
	while (true) {
	    auto next = tok.next();
	    if (next == "") {
		throw config::config_error(tok.getLineNumber(), "Unterminated string literal");
	    }
	    else if (next.length() == 1 && next[0] == end) {
		break;
	    }
	    ss << next;
	}
	tok.skip(true);

	return new std::string(ss.str());
    }

    void* parse_value(tokenizer& tok, std::string& type) {
	auto begin = tok.next();
	if (begin == "{") {
	    type = typeid (config::dict).name();
	    return parse_dict(tok, false);
	}
	else if (begin == "[") {
	    type = typeid (config::array).name();
	    return parse_array(tok);
	}
	else if (begin == "'" || begin == "\"") {
	    type = typeid (std::string).name();
	    return parse_string(tok, begin[0]);
	}
	else if (begin == "false") {
	    type = typeid (bool).name();
	    return new bool(false);
	}
	else if (begin == "true") {
	    type = typeid (bool).name();
	    return new bool(true);
	}
	else {
	    tok.rewind();
	    if (begin.find(".") != std::string::npos) {
		type = typeid (float).name();
		return parse_float(tok);
	    }
	    else {
		type = typeid (int).name();
		return parse_int(tok);
	    }
	}
    }


    config::dict parse(const std::string& content) {
	tokenizer tok(content, { "[", "]", "{", "}", "=", ",", "'", "\"", " ", "\n", "\t", "\r", "#" }, { " ", "\t", "\n", "\r" }, { "#" });

	config::dict dic;
	while (true) {
	    auto next = tok.next();

	    assert_msg(next == "[" || next == "" || next == "#", "expected [ (not " + next + ")", tok);
	    if (next == "[") {
		auto name = tok.next();
		assert_msg(tok.next() == "]", "expected ]", tok);
		auto content = parse_dict(tok, true);
		dic.insert(name, content, typeid (config::dict).name());
	    }
	    else break;
	}

	return dic;
    }


    config::dict parse_file(const std::string& filePath) {
#ifdef DEBUG
	if (!utils::file_exists(filePath)) {
	    throw utils::file_error("File not found : " + filePath);
	}
#endif

	std::ifstream t(filePath);
	std::stringstream buffer;
	buffer << t.rdbuf();			

	return parse(buffer.str ());
    }

    std::string format (const config::dict& cfg) {
	std::stringstream ss;
	cfg.format (ss, 0);

	return ss.str ();
    }
    
}



std::ostream& operator << (std::ostream& stream, const common::utils::config::array& array) {
    array.print(stream);
    return stream;
}

std::ostream& operator << (std::ostream& stream, const common::utils::config::dict& array) {
    array.print(stream);
    return stream;
}
