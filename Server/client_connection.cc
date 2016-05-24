/*
 * Spreadsheet Server 1.7
 * Project Killers - CS 3505
 * client_connection.cc
 * 
 * Uses code by Christopher M. Kohlhoff (chris at kohlhoff dot com)
 * Copyright (c) 2003-2012 Distributed under the Boost Software License
 * 
 */


#include "spreadsheet_server.h"


client_connection::client_connection(boost::asio::io_service& io_service, void* server)
	:socket_(io_service),
	sheet_(NULL),
	server_(server),
	connect_status_(std::string(),NULL)
{
	line_count = 0;
	max_lines = 0;
}
		
tcp::socket& client_connection::socket()
{
	return socket_;
}

// Called by server if there were no errors in the connection
void client_connection::start()
{
	// signals a read from socket, calls handle_connect as data recieved callback
		
	boost::asio::async_read(socket_,
	boost::asio::buffer(read_char, 1),
	boost::bind(&client_connection::handle_connect, shared_from_this(),
	boost::asio::placeholders::error));
}
		
// Called by spreadsheet, should deliver message out the socket
void client_connection::deliver(std::string cmd)
{
	bool write_in_progress = !write_cmds_.empty(); // Test if the socket is currently sending data (send queue is empty)
	write_cmds_.push_back(cmd); // Push new message onto queue
	
	if (!write_in_progress)
	{
		// Sends message asynchronously, calls handle_write as send complete callback
		boost::asio::async_write(socket_,
		boost::asio::buffer(write_cmds_.front().data(),
		write_cmds_.front().length()),
		boost::bind(&client_connection::handle_write, shared_from_this(),
		boost::asio::placeholders::error));
	}
}
		
// Handles reading the actual data in the message and passes the message to the chat room for processing
void client_connection::handle_read(const boost::system::error_code& error)
{
	//std::cout << "handle_read" << std::endl;	
	if (!error)
	{
		buf << read_char;	
		if(read_char[0] == '\n')
		{	
			// If on the first line of a new command...
			// Test leading char to see what type of command
			// We know how many lines each type of command is, 
			// dont clear buffer until command is complete
			// then send the entire thing to the spreadsheet.
			
			line_count++;
				
			if(line_count == 1) // First line of command
			{
				switch(buf.str().at(0)) // switch on first character
				{					
					case 'C':
						// CHANGE command...
						max_lines = 6; 
					break;
					
					case 'U':
						// UNDO command...
						max_lines = 3;
					break;
					
					case 'S':
						// SAVE command...
						max_lines = 2;
					break;
					
					case 'L':
						// Leave command...
						max_lines = 2;
					break;
					
					default:
						// Bad command? Simply send line to spreadsheet
						// for processing.
						max_lines = 0;
					break;
				}
				
			}	
				
			if(line_count >= max_lines) // Time to send command to spreadsheet
			{	
				// Process LEAVE commands locally... (Only buffer two lines
				// to maintain compatibility with other clients?)
				if(buf.str().at(0) == 'L')
				{
					sheet_->leave(shared_from_this()); // Remove self from spreadsheet
					// A new async recieve won't get called and the 
					// boost shared pointer will delete itself and call the destructor
					return;	
				}

				sheet_->process_cmd(shared_from_this(),buf.str()); // Send command to spreadsheet
				buf.str(std::string()); // Clear buffer
				line_count = 0;
			}
		}
		// set up the next read from the socket, will call handle_read when data recieved.
		boost::asio::async_read(socket_,
		boost::asio::buffer(read_char, 1),
		boost::bind(&client_connection::handle_read, shared_from_this(),
		boost::asio::placeholders::error));
	}
	else
	{
		sheet_->leave(shared_from_this()); // Remove self from spreadsheet
		
		// A new async recieve won't get called and the 
		// boost shared pointer will delete itself and call the destructor
	}
}

// Tests for errors and sends if there is more data in queue, will get called recursively as a callback until all data is gone
void client_connection::handle_write(const boost::system::error_code& error)
{
	if (!error)
	{
		write_cmds_.pop_front();
		if (!write_cmds_.empty())
		{
			boost::asio::async_write(socket_,
			boost::asio::buffer(write_cmds_.front().data(),
			write_cmds_.front().length()),
			boost::bind(&client_connection::handle_write, shared_from_this(),
			boost::asio::placeholders::error));
		}
	}
	else
	{
		sheet_->leave(shared_from_this()); // Remove self from spreadsheet
		
		// A new async recieve won't get called and the 
		// boost shared pointer will delete itself and call the destructor.
	}
}

void client_connection::handle_connect(const boost::system::error_code& error)	
{
	if (!error)
	{
		buf << read_char;	
		if(read_char[0] == '\n')
		{	
			// Create/Join commands are 3 lines long,
			// keep adding to the buffer until the third newline char.
			// Then send to the server and clear buffer.
			line_count++;
			if(line_count >= 3)
			{	
				// send complete command to server  
				connect_status_ = ((spreadsheet_server*) server_)->client_command(buf.str());
				buf.str(std::string());
				line_count = 0;
				// Send response from server to client.
				deliver(connect_status_.response());
				
				// See if a spreadsheet pointer was assigned.
				// (Means that a join was sucessfull)
				if(connect_status_.sheet() != NULL)
				{
					sheet_ = connect_status_.sheet();
					sheet_->join(shared_from_this()); // Connects to spreadsheet
					
					// set up the next read from the socket, will call handle_read when data recieved.
					boost::asio::async_read(socket_,
					boost::asio::buffer(read_char, 1),
					boost::bind(&client_connection::handle_read, shared_from_this(),
					boost::asio::placeholders::error));
					return; // Make sure method exits and doesn't hit the other async recieve!
				}
			}
		}
		
		// set up the next read from the socket, will call handle_connect when data recieved.
		boost::asio::async_read(socket_,
		boost::asio::buffer(read_char, 1),
		boost::bind(&client_connection::handle_connect, shared_from_this(),
		boost::asio::placeholders::error));
		
	}

	// If there was an error, a new async recieve won't get called and the 
	// boost shared pointer will delete itself and call the destructor.
}


