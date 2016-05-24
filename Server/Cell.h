//Cell.h
//
//  Cell is a class to represent a Spreadsheet cell
//  Written by Damon Earl

#ifndef CELL_H
#define CELL_H

#include <string>

namespace ss
{
	class Cell
	{
	public:
		Cell(std::string loc, std::string val);
		~Cell();
		std::string getLocation();
		std::string getValue();
		void setValue(std::string);

	private:
		std::string location;
		std::string value;
	};
}

#endif