#include "User.h"
#include <iostream>
#include "uwebsockets/App.h"
#include <map>
#include <fstream>

std::map<std::string, User*> users;
std::string generalChatID = "general";
std::string filename = "users.txt";

struct PerSocketData {
};

bool hasUser(std::string name) {
	return users.find(name) != users.end();
}

void addUserToFile(User user) {
	std::ofstream file;
	file.open(filename, std::ios::app);
	file << user.getName() << " " << user.getPasswordHash() << std::endl;
	file.close();
}

void loadUsersFromFile() {
	std::ifstream file;
	file.open(filename);
	std::string name, passwordHash;
	while (file >> name >> passwordHash) {
		users[name] = new User(name, passwordHash, false);
	}
	file.close();
}

void sendMessageToUser(uWS::WebSocket<false, true, PerSocketData>* ws, std::string message, uWS::OpCode opCode) {
	ws->send(message, opCode);
}

void publishMessageToGeneral(uWS::WebSocket<false, true, PerSocketData>* ws, std::string message, uWS::OpCode opCode) {
	ws->publish(generalChatID, message, opCode);
}

void sendAndPublishMessage(uWS::WebSocket<false, true, PerSocketData>* ws, std::string message, uWS::OpCode opCode) {
	sendMessageToUser(ws, message, opCode);
	publishMessageToGeneral(ws, message, opCode);
}

void splitUserInfo(std::string userInfo, std::string& username, std::string& password) {
	username = userInfo.substr(0, userInfo.find(" "));
	password = userInfo.substr(userInfo.find(" ") + 1);
}

void splitTokenAndMessage(std::string tokenAndMessage, std::string& username, std::string& token, std::string& message) {
	username = tokenAndMessage.substr(0, tokenAndMessage.find(" "));
	tokenAndMessage = tokenAndMessage.substr(tokenAndMessage.find(" ") + 1);
	token = tokenAndMessage.substr(0, tokenAndMessage.find(" "));
	message = tokenAndMessage.substr(tokenAndMessage.find(" ") + 1);
}

int main() {
	srand(time(NULL));
	loadUsersFromFile();

	uWS::TemplatedApp<false>::WebSocketBehavior<PerSocketData> wsb = {
		.compression = uWS::SHARED_COMPRESSOR,
		.maxPayloadLength = 16 * 1024 * 1024,
		.idleTimeout = 10,
		.open = [](auto* ws) {
			std::cout << "Client connected" << std::endl;
		},
		.message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {
			std::cout << "Message received: " << message << std::endl;
			if (message.starts_with("join")) {
				std::string username, password;
				splitUserInfo(std::string(message.substr(5)), username, password);

				if ("join " + username + " " + password != message) {
					ws->send("0 invalid_format", opCode);
					return;
				}

				if (!User::isValidPassword(password)) {
					ws->send("0 invalid_password", opCode);
					return;
				}

				if (!User::isValidName(username)) {
					ws->send("0 invalid_name", opCode);
					return;
				}
				if (!hasUser(username)) {
					ws->send("0 not_exists", opCode);
					return;
				}
				User* user = users[username];
				if (!user->checkPassword(password)) {
					ws->send("0 wrong_password", opCode);
					return;
				}
				std::string token = user->generateToken();
				ws->send("1 " + token, opCode);
				ws->subscribe(generalChatID);
				return;
			}
			if (message.starts_with("register")) {
				std::string username, password;
				splitUserInfo(std::string(message.substr(9)), username, password);

				if ("register " + username + " " + password != message) {
					ws->send("0 invalid_format", opCode);
					return;
				}

				if (!User::isValidPassword(password)) {
					ws->send("0 invalid_password", opCode);
					return;
				}

				if (!User::isValidName(username)) {
					ws->send("0 invalid_name", opCode);
					return;
				}
				if (hasUser(username)) {
					ws->send("0 exists", opCode);
					return;
				}
				users[username] = new User(username, password);
				ws->send("1", opCode);
				addUserToFile(*users[username]);
				return;
			}
			if (message.starts_with("chat")) {
				std::string username, token, chatMessage;
				splitTokenAndMessage(std::string(message.substr(5)), username, token, chatMessage);
				if (chatMessage.length() > 4096) {
					ws->send("0 too_long", opCode);
					return;
				}
				if (!hasUser(username)) {
					ws->send("0 not_exists", opCode);
					return;
				}
				User* user = users[username];
				if (!user->isToken(token)) {
					ws->send("0 wrong_token", opCode);
					return;
				}
				chatMessage = username + ": " + chatMessage;
				sendAndPublishMessage(ws, chatMessage, opCode);
				return;
			}
			if (message.starts_with("disconnect")) {
				std::string username, token;
				splitTokenAndMessage(std::string(message.substr(11)), username, token, token);
				if (!hasUser(username)) {
					ws->send("0 not_exists", opCode);
					return;
				}
				User* user = users[username];
				if (!user->isToken(token)) {
					ws->send("0 wrong_token", opCode);
					return;
				}
				user->resetToken();
				ws->send("1", opCode);
				ws->close();
				return;
			}
			ws->send("0 unknown_command", opCode);
		},
		.drain = [](auto* ws) {
			std::cout << "Drain event" << std::endl;
		},
		.ping = [](auto* ws, std::string_view message) {
			// std::cout << "Ping event" << std::endl;
		},
		.pong = [](auto* ws, std::string_view message) {
			// std::cout << "Pong event" << std::endl;
		},
		.close = [](auto* ws, int code, std::string_view message) {
			std::cout << "Client disconnected" << std::endl;
		}
	};

	uWS::App wsApp = uWS::App();

	wsApp.ws<PerSocketData>("/*", std::move(wsb)).listen(9001, [](auto* listen_socket) {
		if (listen_socket) {
			std::cout << "Listening on port " << 9001 << std::endl;
		}
	}).run();
}
