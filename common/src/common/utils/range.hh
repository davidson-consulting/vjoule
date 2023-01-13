#pragma once

namespace common::utils {

  /**
   * A range iterator, used in for loops
   * implemented in utils/range.cc
   */
  class rIterator {

    long curr;
    long _pad;

  public:

    rIterator(long, long);

    long operator* ();
    void operator++ ();
    bool operator!= (const rIterator&);

  };

  /**
   * A range value used in for loops
   * implemented in utils/range.cc
   */
  class range {
    long _beg;
    long _end;

  public:

    /**
     * \param beg the value of the first element
     * \param end the value of the end
     */
    range(long beg, long end);

    const rIterator begin() const;

    const rIterator end() const;

    long fst();

    long scd();

  };

}

