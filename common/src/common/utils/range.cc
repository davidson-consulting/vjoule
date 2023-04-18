#include <common/utils/range.hh>

namespace common::utils {

  rIterator::rIterator(long curr, long pad) :
    curr(curr),
    _pad(pad)
  {}

  long rIterator::operator* () {
    return this->curr;
  }

  void rIterator::operator++ () {
    this->curr += this->_pad;
  }

  bool rIterator::operator!= (const rIterator& ot) {
    return ot.curr != this->curr;
  }

  range::range(long beg, long end) :
    _beg(beg),
    _end(end)
  {}

  const rIterator range::begin() const {
    if (_beg > _end) return rIterator(_beg, -1);
    else return rIterator(_beg, 1);
  }

  const rIterator range::end() const {
    return rIterator(_end, 1);
  }

  long range::fst() {
    return _beg;
  }

  long range::scd() {
    return _end;
  }

  range r(long beg, long end) {
    return { beg, end };
  }

}

