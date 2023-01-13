#pragma once

#include <common/net/tcp/addr.hh>
#include <common/net/tcp/stream.hh>


namespace common::net {


    class TcpListener {

	int _sockfd = 0;

	SockAddrV4 _addr;

	/// The binded port
	short _port;

    public :
	    
	TcpListener (SockAddrV4 addr);

	/**
	 * Start the tcp listener
	 */
	void start ();

	/**
	 * Accept incoming connexions
	 */
	TcpStream accept ();

	/**
	 * @returns: true if the listener is running
	 */
	bool isOpen () const;
      
	/**
	 * Close the tcp listener
	 */	    
	void close ();
	    
	/**
	 * @returns: the binded port
	 */
	unsigned short port () const;

    private : 

	void bind ();


	    
    };
	

}    

