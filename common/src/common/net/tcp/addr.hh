#pragma once

#include <string>
#include <iostream>

namespace common::net {

    class Ipv4Address {

	unsigned char _a = 0;

	unsigned char _b = 0;

	unsigned char _c = 0;

	unsigned char _d = 0;

    public :

	Ipv4Address (int full);

	Ipv4Address (unsigned char a, unsigned char b, unsigned char c, unsigned char d);


	/**
	 * Store the four part of the ip A.B.C.D in a single u64 (A << 24 | B << 16 | C << 8 | D)
	 */
	unsigned int toN () const;

	unsigned char A () const;
	unsigned char B () const;
	unsigned char C () const;
	unsigned char D () const;
	    
	    
    };
      	
    class SockAddrV4 {

	Ipv4Address _addr;

	unsigned short _port;

    public :

	SockAddrV4 (Ipv4Address addr, unsigned short port);

	SockAddrV4 (const std::string & addr);

	/**
	 * localhost with port
	 */
	SockAddrV4 (unsigned short port);
	    
	Ipv4Address ip () const;

	unsigned short port () const;
	    	    
    };

	
}    



std::ostream & operator<< (std::ostream & s, const common::net::Ipv4Address & addr);
std::ostream & operator<< (std::ostream & s, const common::net::SockAddrV4 & addr);

bool operator==(const common::net::SockAddrV4& left, const common::net::SockAddrV4& right);
