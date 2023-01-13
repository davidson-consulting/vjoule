#include <common/utils/tokenizer.hh>
#include <common/utils/range.hh>
#include <iostream>
#include <algorithm>

namespace common::utils {

  tokenizer::tokenizer(const std::string& content, const std::list <std::string>& tokens, const std::list <std::string>& skips, const std::list <std::string>& comments) :
    _content(content + "\n"),
    _tokens(tokens),
    _skips(skips),
    _comments(comments)
  {}

  unsigned long min(unsigned long u1, unsigned long u2) {
    return u1 < u2 ? u1 : u2;
  }


  std::string tokenizer::next() {
    while (true) {
      std::string x = this->nextPure();
      if (std::find(this->_comments.begin(), this->_comments.end(), x) != this->_comments.end()) {
	while (x != "\n" && x != "") {
	  x = this->nextPure();
	}
      }
      else {
	if (x == "") return x;
	if (!this->_doSkip || std::find(this->_skips.begin(), this->_skips.end(), x) == this->_skips.end())
	  return x;
      }
    }
  }

  std::string tokenizer::nextPure() {
    if (this->_cursor == this->_content.length()) return "";

    unsigned long max = 0, beg = this->_content.length() - this->_cursor;
    for (auto it : this->_tokens) {
      auto id = this->_content.find(it, this->_cursor) - this->_cursor;
      if (id != std::string::npos) {
	if (id == beg && it.length() > max) max = it.length();
	else if (id < beg) {
	  beg = id;
	  max = it.length();
	}
      }
    }

    auto lastCursor = this->_cursor;
    this->_lastLocs.push_back(lastCursor);

    if (beg == this->_content.length() - this->_cursor) {
      this->_cursor = this->_content.length();
      return this->_content.substr(lastCursor);
    }
    else if (beg == 0) {
      this->_cursor += max;
      return this->_content.substr(lastCursor, min(max, this->_content.length() - lastCursor));
    }
    else if (beg > 0) {
      this->_cursor += beg;
      return this->_content.substr(lastCursor, min(beg, this->_content.length() - lastCursor));
    }

    return "";
  }

  void tokenizer::rewind() {
    this->_cursor = this->_lastLocs.back();
    this->_lastLocs.pop_back();
  }

  void tokenizer::skip(bool enable) {
    this->_doSkip = enable;
  }

  int tokenizer::getLineNumber() {
    int nb = 1;
    for (auto i : utils::range(0, min(this->_content.length(), this->_cursor - 1))) {
      if (this->_content[i] == '\n') nb += 1;
    }

    return nb;
  }

}


