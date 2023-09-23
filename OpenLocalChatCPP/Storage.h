#pragma once
#include "User.h"
#include <map>
#include "uwebsockets/App.h"

struct PerSocketData {
};

class Storage
{
private:
	std::map<std::string, User*> users;
	std::map<std::string, std::string> tokens;
	std::map<uWS::WebSocket<false, true, PerSocketData>*, std::string> socketsToken;
	std::string filename = "users.txt";
	static bool isInitialized;
public:
	Storage();
	~Storage();
	bool hasUser(std::string name);
	bool hasToken(std::string token);
	void addUserToFile(User user);
	void loadUsersFromFile();
	
	void addUser(std::string name, std::string password);
	User* getUser(std::string name);

	User* getUserByToken(std::string token);
	std::string getUsernameByToken(std::string token);

	void eraseToken(std::string token);
	void addToken(std::string token, std::string username);
	void addToken(std::string token, User* user);
	
	void addSocket(uWS::WebSocket<false, true, PerSocketData>* ws, std::string token);
	bool isSocketToken(uWS::WebSocket<false, true, PerSocketData>* ws);
	std::string getTokenBySocket(uWS::WebSocket<false, true, PerSocketData>* ws);
	void eraseSocket(uWS::WebSocket<false, true, PerSocketData>* ws);

	User* getUserBySocket(uWS::WebSocket<false, true, PerSocketData>* ws);
	std::string getUsernameBySocket(uWS::WebSocket<false, true, PerSocketData>* ws);
};