#pragma once

#include <iostream>

namespace common::utils {

  template <typename Z, typename ... T>
  class func_ptr {
  protected:

    Z (* _func)(T...args);

  public :

    func_ptr (void (*func)(T ... args)) :
      _func (func) {}


    virtual void operator () (T ... args) const {
      this-> _func (args...);
    }

    virtual bool operator== (const func_ptr<Z,T...> & other) const {
      return this-> _func == other._func;
    }

    virtual bool is_closure () const {
      return false;
    }

    virtual bool is_const_closure () const {
      return false;
    }

    virtual func_ptr <Z, T...>* clone () {
      return new func_ptr<Z, T...> (this-> _func);
    }

  };

  template <class X, typename Z, typename ... T>
  class delegate :
    public func_ptr <Z, T...>
  {
  private :

    Z (X::* _dg)(T...args);
    X * _closure;

  public :

    delegate (X * closure, void (X::*func) (T... args)) :
      func_ptr<Z,T...> (nullptr),
      _dg (func),
      _closure (closure)
    {}

    void operator () (T ... args) const override {
      (this-> _closure->*_dg) (args...);
    }

    bool is_closure () const override {
      return true;
    }

    func_ptr<Z, T...>* clone () override {
      return new delegate<X, Z, T...> (this-> _closure, this-> _dg);
    }

    bool operator== (const func_ptr <Z,T...> & other) const override {
      if (other.is_closure ()) {
        auto aux = reinterpret_cast <const delegate <X,Z,T...> *> (&other);
        return this-> _dg == aux-> _dg && this-> _closure == aux-> _closure;
      }
      return false;
    }

  };

  template <class X, typename Z, typename ... T>
  class const_delegate :
    public func_ptr <Z, T...>
  {
  private :

    Z (X::* _dg)(T...args) const;
    const X * _closure;

  public :

    const_delegate (const X * closure, void (X::*func) (T... args) const) :
      func_ptr<Z,T...> (nullptr),
      _dg (func),
      _closure (closure)
    {}

    void operator () (T ... args) const override {
      (this-> _closure->*_dg) (args...);
    }

    bool is_const_closure () const override {
      return true;
    }

    func_ptr<Z, T...>* clone () override {
      return new const_delegate<X, Z, T...> (this-> _closure, this-> _dg);
    }

    bool operator== (const func_ptr <Z,T...> & other) const override {
      if (other.is_const_closure ()) {
        auto aux = reinterpret_cast <const const_delegate <X,Z,T...> *> (&other);
        return this-> _dg == aux-> _dg && this-> _closure == aux-> _closure;
      }
      return false;
    }

  };


}
