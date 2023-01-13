#include <common/net/tcp/stream.hh>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

namespace common::net {

    TcpStream::TcpStream (int sock, SockAddrV4 addr) :
	_sockfd (sock),
	_addr (addr)
    {
    }

    TcpStream::TcpStream (SockAddrV4 addr) :
	_sockfd (0),
	_addr (addr)
    {
    }

    bool TcpStream::connect () {
	this-> close ();
	this-> _sockfd = socket (AF_INET, SOCK_STREAM, 0);
	if (this-> _sockfd == -1) {
	    return false;
	}

	sockaddr_in sin = { 0 };
	sin.sin_addr.s_addr = this-> _addr.ip ().toN ();
	sin.sin_port = htons(this-> _addr.port ());
	sin.sin_family = AF_INET;

	if (::connect (this-> _sockfd, (sockaddr*) &sin, sizeof (sockaddr_in)) != 0) {
	    return false;
	}

	return true;
    }
	
	
    bool TcpStream::sendInt (unsigned long i) {
	if (this-> _sockfd != 0) {
	    if (write (this-> _sockfd, &i, sizeof (unsigned long)) == -1) {
		this-> _sockfd = 0;
		return false;
	    }
	    return true;
	}

	return false;
    }

    bool TcpStream::send (const std::string & msg) {
	if (this-> _sockfd != 0) {
	    if (write (this-> _sockfd, msg.c_str (), msg.length () * sizeof (char)) == -1) {
		this-> _sockfd = 0;
		return false;
	    }
	    return true;
	}
	return false;
    }

    std::string TcpStream::receive (unsigned long len) {
	if (this-> _sockfd != 0) {
	    auto buf = new char [len + 1];

	    auto r = read (this-> _sockfd, buf, len * sizeof (char));
	    if (r == -1) {
		this-> _sockfd = 0;
	    } else buf [r] = '\0';
		
	    auto ret = std::string (buf);
	    delete buf;

	    return ret;
	}

	return "";	    
    }

    unsigned long TcpStream::receiveInt () {
	unsigned long res = 0;
	if (this-> _sockfd != 0) {
	    auto r = read (this-> _sockfd, &res, sizeof (unsigned long));
	    if (r == -1) {
		this-> _sockfd = 0;
	    }
	}

	return res;
    }



    bool TcpStream::send (const Packet & packet) {
	if (this-> _sockfd != 0) {
	    auto values = net::writePacketToBytes (packet);
	    int size = values.size ();
	    if (write (this-> _sockfd, &size, sizeof (int)) == -1) {
		this-> _sockfd = 0;
		return false;
	    }
      
	    if (write (this-> _sockfd, values.data (), sizeof (char) * values.size ()) == -1) {
		this-> _sockfd = 0;
		return false;
	    }
	    return true;
	}
	return false;
    }


    bool TcpStream::sendPacket (const std::vector <char>& values) {
	if (this-> _sockfd != 0) {
	    int size = values.size ();
	    if (write (this-> _sockfd, &size, sizeof (int)) == -1) {
		this-> _sockfd = 0;
		return false;
	    }
      
	    if (write (this-> _sockfd, values.data (), sizeof (char) * values.size ()) == -1) {
		this-> _sockfd = 0;
		return false;
	    }
	    return true;
	}
	return false;	
    }
    
    bool TcpStream::send (const Header & head) {
	if (this-> _sockfd != 0) {
	    auto values = net::writeHeaderToBytes (head);
	    int size = values.size ();
	    if (write (this-> _sockfd, &size, sizeof (int)) == -1) {
		this-> _sockfd = 0;
		return false;
	    }
      
	    if (write (this-> _sockfd, values.data (), sizeof (char) * values.size ()) == -1) {
		this-> _sockfd = 0;
		return false;
	    }
	    return true;
	}
	return false;
    }
  
    Packet TcpStream::receivePacket () {
	std::vector <char> result;
	if (this-> _sockfd != 0) {
	    int size = 0;
	    if (read (this-> _sockfd, &size, sizeof (int)) != sizeof (int)) {
		this-> close ();
		return Packet ();
	    }

	    if (size == 0) {
		this-> close ();
		return Packet ();
	    }
      
	    result.resize (size);
	    if (read (this-> _sockfd, result.data (), size) != size) {
		this-> close ();
		return Packet ();
	    }
      
	    return net::readPacketFromBytes (result);
	}

	return Packet ();
        
    }

    Header TcpStream::receiveHeader () {
	std::vector <char> result;
	if (this-> _sockfd != 0) {
	    int size = 0;
	    if (read (this-> _sockfd, &size, sizeof (int)) != sizeof (int)) {
		this-> close ();
		return Header ();
	    }

	    if (size == 0) {
		this-> close ();
		return Header ();
	    }
      
	    result.resize (size);
	    if (read (this-> _sockfd, result.data (), size) != size) {
		this-> close ();
		return Header ();
	    }
      
	    return net::readHeaderFromBytes (result);
	}
    
	return Header ();
    }
  
    SockAddrV4 TcpStream::addr () const {
	return this-> _addr;
    }
	
    void TcpStream::close  () {
	if (this-> _sockfd != 0) {
	    ::close (this-> _sockfd);
	    this-> _sockfd = 0;
	}
    }

    bool TcpStream::isOpen () const {
	return this-> _sockfd != 0;
    }
	
}


