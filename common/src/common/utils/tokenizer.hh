#pragma once

#include <string>
#include <list>

namespace common::utils {

  /**
   * Cut a string into tokens
   */
  class tokenizer {
  private:

    std::string _content;

    std::list <std::string> _tokens;

    std::list <std::string> _skips;

    std::list <std::string> _comments;

    std::list <int> _lastLocs;

    int _cursor = 0;

    bool _doSkip = true;

  public:

    /**
     *
     */
    tokenizer(const std::string& content, const std::list <std::string>& tokens, const std::list <std::string>& skips, const std::list <std::string>& comments = {});

    /**
     * @return: the next token
     */
    std::string next();


    /**
     * go back to the previous token location
     */
    void rewind();

    /**
     * Skip the skip tokens?
     */
    void skip(bool enable = true);


    /**
     * @return the line number at the cursor position
     */
    int getLineNumber();

  private:

    std::string nextPure();


  };

}

