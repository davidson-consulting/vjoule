#pragma once
#include <common/net/tcp/addr.hh>
#include <common/net/packet.hh>
#include <common/net/header.hh>
#include <string>


namespace common::net {

    class TcpListener;
    class TcpStream {

	/// The socket of the stream
	int _sockfd = 0;

	/// The address connected or not depending on _sockfd
	SockAddrV4 _addr;

    private :

	friend TcpListener;
	    
	/**
	 * Construction of a stream from an already connected socket
	 * @warning: should be used only by the listener
	 */
	TcpStream (int sock, SockAddrV4 addr);
	    
    public :

	/**
	 * Construction of a stream for a given tcp address
	 * @warning: does not connect the stream (cf. this-> connect);
	 */
	TcpStream (SockAddrV4 addr);
	    
	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================           GETTERS            =========================
	 * ================================================================================
	 * ================================================================================
	 */

	/**
	 * @returns: the address of the stream
	 */
	SockAddrV4 addr () const;

	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================           CONNECT            =========================
	 * ================================================================================
	 * ================================================================================
	 */
	    
	/**
	 * Connect the stream as a client
	 * @info: use the addr given in the constructor
	 * @warning: close the current stream if connected to something
	 * @returns: true if the connection succeeded, false otherwise
	 */
	bool connect ();	    

	/**
	 * @returns: true if the stream is opened
	 */
	bool isOpen () const;
      
	/**
	 * Close the stream if connected
	 */
	void close ();

	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================        SEND / RECEIVE        =========================
	 * ================================================================================
	 * ================================================================================
	 */
	    
	/**
	 * Send a int into the stream
	 * @params: 
	 *   - i: the int to send
	 */
	bool sendInt (unsigned long i);
	    
	    
	/**
	 * Send a message through the stream
	 * @returns: true, iif the send was successful
	 */
	bool send (const std::string & msg);
	    
	/**
	 * Receive a message from the stream
	 * @params: 
	 *   - size: the size of the string to receive
	 */
	std::string receive (unsigned long size);

	/**
	 * Send a packet to the stream
	 * @params:
	 *    - packet: the packet to send
	 */
	bool send (const Packet& packet);

	/**
	 * Send a packet to the stream
	 * @params: 
	 *    - packet: assuming it is the result of net::writePacketToBytes
	 */
	bool sendPacket (const std::vector <char> & pack);
	
	/**
	 * Receive a packet from the stream    
	 * @returns: a packet read from the stream
	 */
	Packet receivePacket ();

	/**
	 * Receive a packet from the stream
	 * @returns: a packet read from the stream
	 */
	Header receiveHeader ();

	/**
	 * Send a packet to the stream
	 * @params:
	 *     - head: the packet to send
	 */
	bool send (const Header & head);

	/**
	 * Receive an int from the stream
	 */
	unsigned long receiveInt ();


    };
	
	
}    
