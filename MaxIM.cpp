// MaxIM.cpp : Defines the entry point for the console application.
//

#include "Manager.hpp"

extern void log(const char * s);

int main(int argc, char * argv[])
{
	std::string host = "login.messaging.aol.com";
//	std::string host = "205.188.251.43";
	std::string port = "5190";

	Manager manager("226475459", "samoteka", host, port);
//	Manager manager("289849585", "Lv5%3!Lr", host, port);

	vector bos, cookie;
	int ec = manager.LoginToAuthServer(bos, cookie);
	
	std::cout << "LoginToAuthServer() exited with the ec: " << ec << std::endl << std::endl;
	

	return 0;
}
