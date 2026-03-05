#include "application.hpp"
#include <iostream>

int main()
{
	std::cout << "Starting application..." << std::endl;
	try
	{
		Application app;
		app.Run();
	}
	catch (std::runtime_error& e)
	{
		std::cout << e.what() << std::endl;
	}
	
}