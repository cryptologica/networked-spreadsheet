#include <string>
#include "Cell.h"

using namespace std;

namespace ss
{
	// Constructor
	Cell::Cell(string loc, string val)
	{
		location = loc;
		value = val;
	}

	// Destructor
	Cell::~Cell()
	{
	}

	// Gets the location of the cell
	string Cell::getLocation()
	{
		return location;
	}

	// Gets the value of the cell
	string Cell::getValue()
	{
		return value;
	}

	// Sets the value of the cell
	void Cell::setValue(string val)
	{
		value = val;
	}
}