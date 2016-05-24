/*
 * Spreadsheet Server 1.7
 * Project Killers - CS 3505
 * spreadsheet.cc
 * 
 * Uses code by Christopher M. Kohlhoff (chris at kohlhoff dot com)
 * Copyright (c) 2003-2012 Distributed under the Boost Software License
 * 
 */


#include "spreadsheet_server.h"
#include "XmlWriter.h"
#include "pugixml.h"


using namespace xml;
using namespace pugi;

spreadsheet::spreadsheet(std::string name, std::string password, int version)
	:name_(name),
	password_(password),
	version_(version),
	undo("EMPTY", "")
{
}

// Gets the argument of the line
std::string spreadsheet::getArgu(std::string line)
{
	std::stringstream ss(line);
	std::string word;
	int n = 1;
	while(getline(ss, line, ':'))
		if(n == 0)
			return word;
		else
			n--;
	return NULL;
}

// Function to write out the whole spreadsheet into an XML file
bool spreadsheet::write(const char* filename)
{
	std::ofstream f(filename);
	XmlStream xml(f);

	xml << tag("spreadsheet");
	// For each cell add it
	std::list<ss::Cell>::iterator it;
	for(it = cells.begin(); it != cells.end(); it++)
	{
		xml << tag("cell")
			<< tag("name")
			<< chardata()
			<< it->getLocation()
			<< endtag("name")
			<< tag("contents")
			<< chardata()
			<< it->getValue()
			<< endtag("contents")
			<< endtag("cell");
	}
	// close spreadsheet tag
	xml	<< endtag("spreadsheet");
	return true;
}

// Reads a spreadsheet object from a file
bool spreadsheet::read(const char* filename)
{
	try
	{
		xml_document doc;
		
		xml_parse_result result = doc.load_file(filename);
		
		for (xml_node_iterator it = doc.begin(); it != doc.end(); ++it)
		{
			for (xml_node_iterator ait = it->begin(); ait != it->end(); ++ait)
			{
				ss::Cell toAdd(ait->child("name").child_value(), ait->child("contents").child_value());
				cells.push_back(toAdd);
			}
		}
	}
	catch(...)
	{
		return false;
	}
	return true;
}

// Reads a spreadsheet object from a file
std::string spreadsheet::read_to_string(const char* filename)
{
	std::string xml_doc;
	xml_doc += "<spreadsheet>";
	try
	{
		xml_document doc;
		
		xml_parse_result result = doc.load_file(filename);
		
		for (xml_node_iterator it = doc.begin(); it != doc.end(); ++it)
		{
			for (xml_node_iterator ait = it->begin(); ait != it->end(); ++ait)
			{
				xml_doc += "<cell>";
				xml_doc += "<name>";
				xml_doc += ait->child("name").child_value();
				xml_doc += "</name>";
				xml_doc += "<contents>";
				xml_doc += ait->child("contents").child_value();
				xml_doc += "</contents>";
				xml_doc += "</cell>";
			}
		}
		xml_doc += "</spreadsheet>";
	}
	catch(...)
	{
		return "FILE WAS NOT FOUND";
	}
	return xml_doc;
}
	  
// Updates a cell
bool spreadsheet::update(std::string loc, std::string val)
{
	// Iterate through list if cell exists, update it
	std::list<ss::Cell>::iterator it;
	
	// Save cell before adding
	undo = ss::Cell(loc, get_value(loc));

	for(it = cells.begin(); it != cells.end(); it++)
	{
		if(it->getLocation() == loc)
		{
			it->setValue(val);
			return true;
		}
	}

	// Cell doesn't exist add it to the list
	ss::Cell toAdd(loc, val);
	cells.push_back(toAdd);

	return true;
}

// Called when a client wants to connect to the spreadsheet
void spreadsheet::join(spreadsheet_participant_ptr participant)
{
	std::cout << "Participant joined spreadsheet..." << std::endl;    
	participants_.insert(participant); // Inserts the new participant into spreadsheet
		
	// Send spreadsheet xml to new participant 
	// (Rest of join command response is server generated)
	

	const char* f = "temp";
	write(f);
	participant->deliver((read_to_string(f) + "\n"));
}

// Called when a client disconnects from the spreadsheet
void spreadsheet::leave(spreadsheet_participant_ptr participant)
{
	std::cout << "Participant left spreadsheet..." << std::endl;  
	participants_.erase(participant); // Remove participant from spreadsheet
}

// Gets the value of a certain cell
std::string spreadsheet::get_value(std::string loc)
{
	std::list<ss::Cell>::iterator it;

	// Find the cell
	for(it = cells.begin(); it != cells.end(); it++)
	{
		if(it->getLocation() == loc) return it->getValue();
	}

	// Not found return IS_EMPTY
	return "";
}

std::string spreadsheet::get_password()
{
	return password_;
}

std::string spreadsheet::get_name()
{
	return name_;
}

int spreadsheet::get_version()
{
	return version_;
}

std::string spreadsheet::get_length()
{

}

// Processes spreadsheet commands
void spreadsheet::process_cmd(spreadsheet_participant_ptr participant, std::string cmd) // Called whenever a client sends something
{ 
	// The client_connection object only sends complete commands to 
	// the spreadsheet object...
	// This means that a varying number of lines will be recieved in each string.
	// CHANGE --> 6 lines
	// UNDO --> 3 Lines
	// SAVE --> 2 lines
	// Unknown string --> 1 line
	// Connection commands are routed to the server
	
	std::cout << "Spreadsheet Command:\n" << cmd;
	
	std::string trash;
	
	std::stringstream ss(cmd);
	std::string cur;
	ss >> cur;
	if(cur  == "CHANGE") // Update the cell
	{
		std::string cell_name;
		std::string version;
		std::string cell_value;
		ss >> trash; // Throw away name line
		
		getline(ss, trash, ':');
		ss >> version;
		
		int v = atoi( version.c_str() );
		
		// Check version
		if(v < version_)
		{
			// send fail message
			std::string message = "CHANGE WAIT\nName:" + name_ + "\nVersion:" + boost::lexical_cast<std::string>(version_) + "\n";
			participant->deliver(message);
			
			return;
		}
		// Update version
		version_ = v+1;
			
		getline(ss, trash, ':');
		ss >> cell_name;
		
		ss >> trash; // Throw away length
		
		//cell_value = ss.str(); // Entire stringstream
		//ss >> cell_value;
		//std::ss.getline(ss,cell_value); 
		
		getline(ss, trash, '\n'); // Gets rid of newline char at start of last line
		getline(ss, cell_value, '\n');
		cell_value = cell_value.substr(0, cell_value.size()-1);
		
		std::cout << "New Cell Value: "<< cell_value << std::endl;
		
		update(cell_name, cell_value);
		
		std::set<spreadsheet_participant_ptr>::iterator it;
		for(it = participants_.begin(); it != participants_.end(); it++)
		{
			if((*it) != participant) // send normal change message
			{
				std::string message = "UPDATE\nName:" + name_ + 
									  "\nVersion:" + boost::lexical_cast<std::string>(version_) + 
									  "\nCell:" + cell_name + 
									  "\nLength:0" + "\n" + 
									  cell_value + "\n";
				(*it)->deliver(message);
			}
			else // Send custom message
			{
				std::string message = "CHANGE OK\nName:" + name_ + "\nVersion:" + boost::lexical_cast<std::string>(version_) + "\n";
				(*it)->deliver(message);
			}
		}
		
	}
	else if(cur == "UNDO") // Undo the last command
	{
		std::string version;
		ss >> trash; // Throw away name line
		
		getline(ss, trash, ':');
		ss >> version;
		
		int v = atoi(version.c_str());
		
		// Check version
		if(v < version_)
		{
			// send fail message
			std::string message = "UNDO WAIT\nName:" + name_ + "\nVersion:" + boost::lexical_cast<std::string>(version_) + "\n";
			participant->deliver(message);
			
			return;
		}
		
		// Check to make sure undo is possible if so undo
		if(undo.getLocation() != "EMPTY")
		{
			std::string cell_name = undo.getLocation();
			std::string cell_value = undo.getValue();
			update(cell_name, cell_value);
		
			// Update version
			version_ = v+1;
			
			std::set<spreadsheet_participant_ptr>::iterator it;
			for(it = participants_.begin(); it != participants_.end(); it++)
			{
				if((*it) == participant) // send specific message to undo initiator
				{
					std::string message = "UNDO OK\nName:" + name_ + 
										  "\nVersion:" + boost::lexical_cast<std::string>(version_) + 
										  "\nCell:" + cell_name + 
										  "\nLength:0" + "\n" + 
										  cell_value + "\n";
					(*it)->deliver(message);
				}

				// Send normal message to everyone
				std::string message = "UPDATE\nName:" + name_ + 
										  "\nVersion:" + boost::lexical_cast<std::string>(version_) + 
										  "\nCell:" + cell_name + 
										  "\nLength:0" + "\n" + 
										  cell_value + "\n";
				(*it)->deliver(message);
			}
		}
		else
		{
			// send fail message
			std::string message = "UNDO END\nName:" + name_ + "\nVersion:" + boost::lexical_cast<std::string>(version_) + "\n";
			participant->deliver(message);
			
			return;
		}	
	}
	else if(cur == "SAVE") // Save the spreadsheet
	{
		try
		{
			getline(ss, trash, ':');
			std::string filename;
			ss >> filename;
			write(filename.c_str());

			std::string message = "SAVE OK\nName:" + name_ + "\n";
			participant->deliver(message);
		}
		catch(...)
		{
			// send fail message
			std::string message = "SAVE FAIL\nName:" + name_ + "\n Unable to save file...\n";
			participant->deliver(message);
		}
	}
	else
	{
		// Send error message, unknown command recieved
		participant->deliver("ERROR\n");
	}

}
