#include "Storage.h"
#include <fstream>

bool Storage::isInitialized = false;

Storage::Storage()
{
	if (isInitialized) {
		throw std::exception("Storage is already initialized");
	}
	loadUsersFromFile();
	isInitialized = true;
}

Storage::~Storage()
{
	for (auto it = users.begin(); it != users.end(); it++) {
		delete it->second;
	}
	isInitialized = false;
}

bool Storage::hasUser(std::string name)
{
	return users.find(name) != users.end();
}

bool Storage::hasToken(std::string token) {
	return tokens.find(token) != tokens.end();
}

void Storage::addUserToFile(User user) {
	std::ofstream file;
	file.open(filename, std::ios::app);
	file << user.getName() << " " << user.getPasswordHash() << std::endl;
	file.close();
}

void Storage::loadUsersFromFile() {
	std::ifstream file;
	file.open(filename);
	std::string name, passwordHash;
	while (file >> name >> passwordHash) {
		users[name] = new User(name, passwordHash, false);
	}
	file.close();
}

User* Storage::getUser(std::string name) {
	return users[name];
}

User* Storage::getUserByToken(std::string token) {
	return users[tokens[token]];
}

std::string Storage::getUsernameByToken(std::string token) {
	return tokens[token];
}

void Storage::eraseToken(std::string token) {
	tokens.erase(token);
}

void Storage::addToken(std::string token, std::string username) {
	tokens[token] = username;
}

void Storage::addToken(std::string token, User* user) {
	addToken(token, user->getName());
}

void Storage::addUser(std::string name, std::string password) {
	users[name] = new User(name, password, true);
	addUserToFile(*users[name]);
}

void Storage::addSocket(uWS::WebSocket<false, true, PerSocketData>* ws, std::string token) {
	socketsToken[ws] = token;
}

bool Storage::isSocketToken(uWS::WebSocket<false, true, PerSocketData>* ws) {
	return socketsToken.find(ws) != socketsToken.end();
}

std::string Storage::getTokenBySocket(uWS::WebSocket<false, true, PerSocketData>* ws) {
	return socketsToken[ws];
}

void Storage::eraseSocket(uWS::WebSocket<false, true, PerSocketData>* ws) {
	socketsToken.erase(ws);
}

User* Storage::getUserBySocket(uWS::WebSocket<false, true, PerSocketData>* ws) {
	return getUserByToken(getTokenBySocket(ws));
}

std::string Storage::getUsernameBySocket(uWS::WebSocket<false, true, PerSocketData>* ws) {
	return getUsernameByToken(getTokenBySocket(ws));
}
