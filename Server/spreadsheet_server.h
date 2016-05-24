
/*
 * Spreadsheet Server 1.7
 * Project Killers - CS 3505
 * spreadsheet_server.h
 * 
 * Uses code by Christopher M. Kohlhoff (chris at kohlhoff dot com)
 * Copyright (c) 2003-2012 Distributed under the Boost Software License
 * 
 */
 
 
#ifndef SPREADSHEET_SERVER_H
#define SPREADSHEET_SERVER_H
 
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <list>
#include <set>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include "Cell.h"

using boost::asio::ip::tcp;

//----------------------------------------------------------------------

class spreadsheet_participant
{
	public:
		virtual ~spreadsheet_participant() {}
		virtual void deliver(std::string cmd) = 0; 
};

//----------------------------------------------------------------------

typedef boost::shared_ptr<spreadsheet_participant> spreadsheet_participant_ptr;

//----------------------------------------------------------------------

class spreadsheet 
{
	public:
		spreadsheet(std::string name, std::string password, int version);
		std::string getArgu(std::string line);
		bool update(std::string loc, std::string val);
		void join(spreadsheet_participant_ptr participant);
		void leave(spreadsheet_participant_ptr participant);
		void process_cmd(spreadsheet_participant_ptr participant, std::string cmd);
		bool write(const char* filename);
		bool read(const char* filename);
		std::string get_value(std::string loc);
		std::string get_password();
		std::string get_name();
		int get_version();
		std::string get_length();
		std::string read_to_string(const char* filename);
	private:
		std::set<spreadsheet_participant_ptr> participants_; 
		std::string password_;
		std::string name_;
		int version_;
		std::list<ss::Cell> cells;
		std::string string_XML_;
		ss::Cell undo;
};

//----------------------------------------------------------------------

class server_response
{
	public:
		server_response(std::string response, spreadsheet* sheet)
			:response_(response),
			sheet_(sheet){}
	
		std::string response(){return response_;}
		
		spreadsheet* sheet(){return sheet_;}
		
	private:
		std::string response_;
		spreadsheet* sheet_;
};

//----------------------------------------------------------------------

class client_connection
	:public spreadsheet_participant,
	public boost::enable_shared_from_this<client_connection>
{
	public:
		client_connection(boost::asio::io_service& io_service, void* server);
		tcp::socket& socket();
		void start();
		void deliver(std::string cmd);
		void handle_read(const boost::system::error_code& error);
		void handle_write(const boost::system::error_code& error);
		void handle_connect(const boost::system::error_code& error);
	private:
		tcp::socket socket_;
		spreadsheet* sheet_;
		void* server_;
		server_response connect_status_;
		char read_char[1];
		std::stringstream buf;
		int line_count;
		int max_lines;
		std::deque<std::string> write_cmds_;
};

//----------------------------------------------------------------------

typedef boost::shared_ptr<client_connection> client_connection_ptr;

//----------------------------------------------------------------------

class spreadsheet_server
{
	public:
		spreadsheet_server(boost::asio::io_service& io_service,const tcp::endpoint& endpoint);
		void start_accept();
		void handle_accept(client_connection_ptr connection, const boost::system::error_code& error);
		server_response client_command(std::string cmd);
	private:
		bool load_spreadsheet(std::string name, std::string password);	
		boost::asio::io_service& io_service_;
		tcp::acceptor acceptor_;
		std::map<std::string,spreadsheet*> loaded_spreadsheets_; // Store open spreadsheets, locate by name
};

#endif
