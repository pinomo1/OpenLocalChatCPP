#include "Storage.h"
#include <iostream>
#include <vector>
#include <string>

Storage* s;
std::string generalChatID = "general";

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

void splitTokenAndMessage(std::string tokenAndMessage, std::string& token, std::string& message) {
	token = tokenAndMessage.substr(0, tokenAndMessage.find(" "));
	message = tokenAndMessage.substr(tokenAndMessage.find(" ") + 1);
}

bool checkToken(std::string token, std::string* error) {
	if (!s->hasToken(token)) {
		*error = "no_such_token";
		return false;
	}
	std::string username = s->getUsernameByToken(token);
	if (!s->hasUser(username)) {
		*error = "no_such_user";
		s->eraseToken(token);
		return false;
	}
	User* user = s->getUser(username);
	if (!user->isToken(token)) {
		*error = "no_such_token";
		s->eraseToken(token);
		return false;
	}
	return true;
}

User* userByToken(std::string token, std::string* error) {
	if (!checkToken(token, error)) {
		return nullptr;
	}
	return s->getUserByToken(token);
}

template<class ...Args>
bool isFormatValid(std::string message, std::string command, Args... args) {
	std::vector<std::string> stringArgs = { args... };
	std::string format = command;
	for (std::string arg : stringArgs) {
		format += " " + arg;
	}
	return message == format;
}

void sendError(uWS::WebSocket<false, true, PerSocketData>* ws, std::string error, uWS::OpCode opCode) {
	ws->send("0 " + error, opCode);
}

void sendInvalidFormatError(uWS::WebSocket<false, true, PerSocketData>* ws, uWS::OpCode opCode) {
	sendError(ws, "invalid_format", opCode);
}

bool credentialsChecker(std::string username, std::string password, bool registration = true, std::string* error = nullptr) {
	if (!User::isValidPassword(password)) {
		*error = "invalid_password";
		return false;
	}

	if (!User::isValidName(username)) {
		*error = "invalid_name";
		return false;
	}
	if (registration && s->hasUser(username)) {
		*error = "user_exists";
		return false;
	}
	if (!registration && !s->hasUser(username)) {
		*error = "no_such_user";
		return false;
	}
	return true;
}

int main() {
	s = new Storage();
	srand(time(NULL));

	uWS::TemplatedApp<false>::WebSocketBehavior<PerSocketData> wsb = {
		.compression = uWS::SHARED_COMPRESSOR,
		.maxPayloadLength = 16 * 1024 * 1024,
		.idleTimeout = 10,
		.open = [](auto* ws) {
			std::cout << "Client connected" << std::endl;
		},
		.message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {
			std::cout << "Message received: " << message << std::endl;
			std::string msg = std::string(message);
			std::string command = std::string(message.substr(0, message.find(" ")));
			std::string args = std::string(message.substr(message.find(" ") + 1));
			std::string error = "";

			if (command == "login") {
				std::string username, password;
				splitUserInfo(args, username, password);

				if (!isFormatValid(msg, command, username, password)) {
					sendInvalidFormatError(ws, opCode);
					return;
				}

				if (!credentialsChecker(username, password, false, &error)) {
					sendError(ws, error, opCode);
					return;
				}

				User* user = s->getUser(username);
				if (!user->checkPassword(password)) {
					sendError(ws, "wrong_password", opCode);
					return;
				}
				std::string oldToken = user->getToken();
				if (s->hasToken(oldToken)) {
					s->eraseToken(oldToken);
				}
				std::string token = user->generateToken();
				s->addToken(token, username);
				ws->send("1 " + token, opCode);
				return;
			}
			if (command == "join") {
				std::string token;
				token = args;

				if (!isFormatValid(msg, command, token)) {
					sendInvalidFormatError(ws, opCode);
					return;
				}

				User* user = userByToken(token, &error);
				if (user == nullptr) {
					sendError(ws, error, opCode);
					return;
				}
				s->addSocket(ws, token);
				ws->send("1", opCode);
				ws->subscribe(generalChatID);
				sendAndPublishMessage(ws, "System: " + s->getUsernameByToken(token) + " joined", opCode);
				return;
			}
			if (command == "register") {
				std::string username, password;
				splitUserInfo(args, username, password);

				if (!isFormatValid(msg, command, username, password)) {
					sendInvalidFormatError(ws, opCode);
					return;
				}

				if (!credentialsChecker(username, password, true, &error)) {
					sendError(ws, error, opCode);
					return;
				}

				s->addUser(username, password);
				ws->send("1", opCode);
				return;
			}
			if (command == "chat") {
				std::string token, chatMessage;
				splitTokenAndMessage(args, token, chatMessage);
				if (chatMessage.length() > 4096) {
					sendError(ws, "message_too_long", opCode);
					return;
				}

				if (!isFormatValid(msg, command, token, chatMessage)) {
					sendInvalidFormatError(ws, opCode);
					return;
				}

				User* user = userByToken(token, &error);
				if (user == nullptr) {
					sendError(ws, error, opCode);
					return;
				}

				std::string username = s->getUsernameByToken(token);
				chatMessage = username + ": " + chatMessage;
				sendAndPublishMessage(ws, chatMessage, opCode);
				return;
			}
			if (command == "logout") {
				std::string token = args;
				if (!isFormatValid(msg, command, token)) {
					sendInvalidFormatError(ws, opCode);
					return;
				}
				User* user = userByToken(token, &error);
				if (user == nullptr) {
					sendError(ws, error, opCode);
					return;
				}
				user->resetToken();
				if (s->hasToken(token)) {
					s->eraseToken(token);
					s->eraseSocket(ws);
				}
				publishMessageToGeneral(ws, "System: " + user->getName() + " left", opCode);
				ws->send("1", opCode);
				ws->close();
				return;
			}
			sendError(ws, "no_such_command", opCode);
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
			if (s->isSocketToken(ws)) 
			{
				std::string token = s->getTokenBySocket(ws);
				std::string username = s->getUsernameByToken(token);
				publishMessageToGeneral(ws, "System: " + username + " left", uWS::OpCode::TEXT);
				s->eraseSocket(ws);
			}
		}
	};

	uWS::App wsApp = uWS::App();

	wsApp.ws<PerSocketData>("/*", std::move(wsb)).listen(9001, [](auto* listen_socket) {
		if (listen_socket) {
			std::cout << "Listening on port " << 9001 << std::endl;
		}
	}).run();
}
