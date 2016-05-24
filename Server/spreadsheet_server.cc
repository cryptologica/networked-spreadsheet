/*
 * Spreadsheet Server 1.7
 * Project Killers - CS 3505
 * spreadsheet_server.cc
 * 
 * Uses code by Christopher M. Kohlhoff (chris at kohlhoff dot com)
 * Copyright (c) 2003-2012 Distributed under the Boost Software License
 * 
 */


#include "spreadsheet_server.h"



spreadsheet_server::spreadsheet_server(boost::asio::io_service& io_service,
const tcp::endpoint& endpoint)
	:io_service_(io_service),
	acceptor_(io_service, endpoint)
{
	std::cout << "Server Running..." << std::endl;
	start_accept(); // Sets up for the first recieve upon the server's creation.
}

void spreadsheet_server::start_accept() // Called when a new connection is recieved...
{
	// Creates a new client_connection referenced by a pointer
	client_connection_ptr new_connection(new client_connection(io_service_,this));
	   
	// Accepts the socket and connects it to the new session, calls handle_accept on completion
	acceptor_.async_accept(new_connection->socket(),
	boost::bind(&spreadsheet_server::handle_accept, this, new_connection,
	boost::asio::placeholders::error));
}

void spreadsheet_server::handle_accept(client_connection_ptr connection, 
const boost::system::error_code& error)
{  
	std::cout << "New Socket Recieved..." << std::endl;
	    
	if (!error)
	{
		connection->start();
	}
	start_accept(); // Calls the accept routine in the server for the next incomming socket connection
}

server_response spreadsheet_server::client_command(std::string cmd)
{	
	// Split out elements of command
	std::stringstream ss(cmd);
	std::string cmd_type;
	std::string name;
	std::string password;
	std::string trash;
	bool fail_flag = false;
	
	std::cout << "Server Command Recieved:\n" <<cmd;
	
	ss >> cmd_type; 			// First line is saved as command type
	getline(ss, trash, ':'); 	// Get rid of first half of 2nd line. :-)
	ss >> name; 				// Get name
	getline(ss, trash, ':'); 	// Get rid of first half of 3rd line.
	ss >> password; 			// Get password
	
	// Determine type of command	
	if(cmd_type  == "CREATE")
	{	
		// Look for spreadsheet in map
		if(loaded_spreadsheets_.find(name) == loaded_spreadsheets_.end())
		{
			// Spreadsheet not currently loaded in map
			// Check for spreadsheet file and load into map if it exists.
			fail_flag = load_spreadsheet(name,password); // Returns true if loads a file into map (create will fail)
		}
		else
		{
			// Spreadsheet in map 
			fail_flag = true;
		}
		
		if(fail_flag)
		{
			// Send fail message
			std::string message = "CREATE FAIL\nName:" + name + "\nSpreadsheet file already exists on server.\n";
			return server_response(message,NULL); 
		}
		else
		{
			// OK to create spreadsheet!
			
			// Would be a memory leak, but we don't unload spreadsheets until the program exits anyways.
			// I guess you could unload a spreadsheet by manually calling the destructor through the
			// pointer and then deleting the value out of the map.
			spreadsheet* new_spreadsheet = new spreadsheet(name,password,0);
			
			loaded_spreadsheets_.insert(std::pair<std::string,spreadsheet*>(name,new_spreadsheet));
			std::string message = "CREATE OK\nName:" + name + "\nPassword:" + password + "\n";
			return server_response(message,NULL); 
		}
	}
	else if(cmd_type == "JOIN")
	{
		// Look for spreadsheet in map
		if(loaded_spreadsheets_.find(name) == loaded_spreadsheets_.end())
		{
			// Spreadsheet not currently loaded in map
			// Check for spreadsheet file and load into map if it exists.
			fail_flag = !(load_spreadsheet(name,password)); // Returns true if loads a file into map (join will pass, result is inverted)
		}
		
		// Fail flag should be false if spreadsheet file was found/loaded
		if(fail_flag)
		{
			// Send fail message
			std::string message = "JOIN FAIL\nName:" + name + "\nSpreadsheet does not exist on server.\n";
			return server_response(message,NULL); 
		}
		else // Found spreadsheet
		{		
			spreadsheet* load_temp = loaded_spreadsheets_[name];
			// Check password
			if(password == load_temp->get_password())
			{
				// Join to spreadsheet
				// Send OK message, (Actual XML data will be sent by spreadsheet upon client join)
				/*std::string message = "JOIN SP OK\nName:" + name + "\nVersion:" + boost::lexical_cast<std::string>(load_temp->get_version())
				 +"\nLength:" + boost::lexical_cast<std::string>(load_temp->get_length()) + "\n";*/
				 
				 std::string message = "JOIN OK\nName:" + name + "\nVersion:" + boost::lexical_cast<std::string>(load_temp->get_version())
				 +"\nLength:0" + "\n";

				 				
				return server_response(message,load_temp); 
			}
			else
			{
				// Send fail message
				std::string message = "JOIN FAIL\nName:" + name + "\nIncorrect spreadsheet password.\n";
				return server_response(message,NULL); 			
			}
		}
		
	}
	else
	{
		// Send error message, unknown command recieved
		return server_response("ERROR\n",NULL); 	
	}
}
	
bool spreadsheet_server::load_spreadsheet(std::string name, std::string password)
{
	// Find spreadsheet file in system and load into map if it exists.
	// Return true if could find and load.
	
	
	// Code to add spreadsheet into server (probably want to modify to allow initializing with data?)
	// If modify constructor don't forget to modify code in client_command join section as well.
	std::stringstream ss(name);
	char* f;
	ss >> f;
	std::ifstream ifile(f);
	if(ifile)
	{
		spreadsheet* new_spreadsheet = new spreadsheet(name,password,0);
		loaded_spreadsheets_.insert(std::pair<std::string,spreadsheet*>(name,new_spreadsheet));
		new_spreadsheet->read((f));
		return true;
	}
	else
	{
		return false;
	}
}	
		
int main(int argc, char* argv[])
{
	try
	{
		boost::asio::io_service io_service;
		tcp::endpoint endpoint(tcp::v4(), 2014); // should be port 1984
		new spreadsheet_server(io_service, endpoint);
		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
	return 0;
}
