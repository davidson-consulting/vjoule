#include <common/net/tcp/listener.hh>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <common/utils/log.hh>
#include <fcntl.h>

namespace common::net {

  bool ignoreSigPipe () {
    utils::Logger::globalInstance ().info ("Disabling SIGPIPE");
    signal(SIGPIPE, SIG_IGN); // ignore
    return true;
  }
	
  bool __SIG__ = ignoreSigPipe ();
	

	
  TcpListener::TcpListener (SockAddrV4 addr) :
    _addr (addr)
  {
  }


  void TcpListener::start () {
    this-> bind ();
  }

  TcpStream TcpListener::accept () {
    sockaddr_in client = { 0 };
    unsigned int len = sizeof (sockaddr_in);

    auto sock = ::accept (this-> _sockfd, (sockaddr*) (&client), &len);
    if (sock <= 0) {
      std::cout << "Failed to accept client" << std::endl;
      return TcpStream (0, SockAddrV4 (Ipv4Address (0, 0, 0, 0), 0));
    }
    
    auto addr = SockAddrV4 (Ipv4Address (client.sin_addr.s_addr), ntohs (client.sin_port));	    
    return TcpStream (sock, addr);
  }


    bool TcpListener::isOpen () const {
	return this-> _sockfd != 0;
    }

  void TcpListener::close () {
    if (this-> _sockfd != 0) {
      ::close (this-> _sockfd);
      this-> _sockfd = 0;
    }
  }

  unsigned short TcpListener::port () const {
    return this-> _port;
  }

	
  void TcpListener::bind () {
    this-> _sockfd = socket (AF_INET, SOCK_STREAM, 0);
    if (this-> _sockfd == -1) {
      std::cout << "Error creating socket" << std::endl;
      exit (-1);
    }

    sockaddr_in sin = { 0 };
    sin.sin_addr.s_addr = this-> _addr.ip ().toN ();
    sin.sin_port = htons(this-> _addr.port ());
    sin.sin_family = AF_INET;

    if (::bind (this-> _sockfd, (sockaddr*) &sin, sizeof (sockaddr_in)) != 0) {
      std::cout << "Error binding socket" << std::endl;
      exit (-1);	
    }

    if (listen (this-> _sockfd, 100) != 0) {
      std::cout << "Error listening socket" << std::endl;
      exit (-1);	
    }

    if (this-> _addr.port () == 0) {
      unsigned int len = sizeof (sockaddr_in);
      auto r = getsockname (this-> _sockfd, (sockaddr*) &sin, &len);
      if (r == 0) {
	this-> _port = ntohs (sin.sin_port);
      }
    } else this-> _port = this-> _addr.port ();
	    
  }


}    


