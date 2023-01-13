#include <common/net/tcp/addr.hh>
#include <common/utils/tokenizer.hh>
#include <stdexcept>

union Packer {
    unsigned char pack [4];
    unsigned int n;
};
       
namespace common::net {
	
    Ipv4Address::Ipv4Address (int full) {
	Packer p;
	p.n = full;
	this-> _a = p.pack [0];
	this-> _b = p.pack [1];
	this-> _c = p.pack [2];
	this-> _d = p.pack [3];
    }
	
    Ipv4Address::Ipv4Address (unsigned char a, unsigned char b, unsigned char c, unsigned char d) :
	_a (a), _b (b), _c (c), _d (d)
    {	    
    }

    unsigned int Ipv4Address::toN () const {
	Packer p;
	p.pack [0] = this-> _a;
	p.pack [1] = this-> _b;
	p.pack [2] = this-> _c;
	p.pack [3] = this-> _d;

	return p.n;
    }

    unsigned char Ipv4Address::A () const {
	return this-> _a;
    }

    unsigned char Ipv4Address::B () const {
	return this-> _b;
    }

    unsigned char Ipv4Address::C () const {
	return this-> _c;
    }

    unsigned char Ipv4Address::D () const {
	return this-> _d;
    }
	

    SockAddrV4::SockAddrV4 (Ipv4Address addr, unsigned short port) :
	_addr (addr), _port (port)
    {}

    SockAddrV4::SockAddrV4 (unsigned short port) :
	_addr (127,0,0,1), _port (port)
    {}    
    
    SockAddrV4::SockAddrV4 (const std::string & addr) :
	_addr (Ipv4Address (0, 0, 0, 0)), _port (0)
    {
	utils::tokenizer tok (addr, {".", ":", " "}, {" "});
	auto fst = std::atoi (tok.next ().c_str ());
	if (tok.next () != ".") throw std::runtime_error (std::string ("Malformed addr : ") + addr);
	auto scd = std::atoi (tok.next ().c_str ());
	if (tok.next () != ".") throw std::runtime_error (std::string ("Malformed addr : ") + addr);
	auto thd = std::atoi (tok.next ().c_str ());
	if (tok.next () != ".") throw std::runtime_error (std::string ("Malformed addr : ") + addr);
	auto fth = std::atoi (tok.next ().c_str ());
	if (tok.next () != ":") throw std::runtime_error (std::string ("Malformed addr : ") + addr);
	auto port = std::atoi (tok.next ().c_str ());
	    
	this-> _addr = Ipv4Address (fst, scd, thd, fth);
	this-> _port = port;	    
    }

    Ipv4Address SockAddrV4::ip () const {
	return this-> _addr;
    }

    unsigned short SockAddrV4::port () const {
	return this-> _port;
    }		

}    

std::ostream & operator<< (std::ostream & s, const common::net::Ipv4Address & addr) {
    s << (int) addr.A () << "." << (int) addr.B () << "." << (int) addr.C () << "." << (int) addr.D ();
    return s;
}

std::ostream & operator<< (std::ostream & s, const common::net::SockAddrV4 & addr) {
    s << addr.ip () << ":" << (int) addr.port ();
    return s;
}


bool operator== (const common::net::SockAddrV4 & left, const common::net::SockAddrV4 & right) {
    return left.ip ().A () == right.ip ().A () &&
	left.ip ().A () == right.ip ().A () &&
	left.ip ().A () == right.ip ().A () &&
	left.ip ().A () == right.ip ().A () &&
	left.port () == right.port ();
}
