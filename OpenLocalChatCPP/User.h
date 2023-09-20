#pragma once
#include <string>

class User
{
private:
	std::string name;
	std::string passwordHash;
	std::string lastToken;

public:
	User();
	User(std::string name, std::string password, bool isPassword = true);

	std::string getName();
	bool checkPassword(std::string password);
	bool isToken(std::string token);
	std::string generateToken();
	std::string computeHash(std::string password);
	void resetToken();
	std::string getPasswordHash();

	static bool isValidCharacter(char c) {
		return std::isalnum(c) || c == '_' || c == '-';
	}

	static bool isValidName(std::string name) {
		if (name.length() > 20 || name.length() < 4) {
			return false;
		}
		for (char c : name) {
			if (!std::isalnum(c)) {
				return false;
			}
		}
		return true;
	}

	static bool isValidPassword(std::string password) {
		if (password.length() > 32 || password.length() < 6) {
			return false;
		}
		for (char c : password) {
			if (!std::isalnum(c)) {
				return false;
			}
		}
		return true;
	}
};

