#include "User.h"
#include "uuid.h"
#include <iostream>
#include "cryptopp/sha.h"
#include "cryptopp/hex.h"

std::string User::computeHash(std::string password) {
	CryptoPP::SHA256 hash;
	byte digest[CryptoPP::SHA256::DIGESTSIZE];
	hash.CalculateDigest(digest, (byte*)password.c_str(), password.length());
	std::string passwordHash;
	CryptoPP::HexEncoder encoder;
	encoder.Attach(new CryptoPP::StringSink(passwordHash));
	encoder.Put(digest, sizeof(digest));
	encoder.MessageEnd();
	std::cout << "Computed hash " << passwordHash << std::endl;
	return passwordHash;
}

User::User() : User("", "") {}

User::User(std::string name, std::string password, bool isPassword) {
	this->name = name;
	if (isPassword) {
		this->passwordHash = computeHash(password);	
	}
	else {
		this->passwordHash = password;
	}
	this->lastToken = "";
}

std::string User::getName() {
	return name;
}

bool User::checkPassword(std::string password) {
	std::cout << "Checking password " << password << " against " << passwordHash << std::endl;
	return computeHash(password) == passwordHash;
}

bool User::isToken(std::string token) {
	return lastToken != "" && token == lastToken;
}

std::string User::generateToken() {
	lastToken = newUUID();
	return lastToken;
}

void User::resetToken() {
	lastToken = "";
}

std::string User::getPasswordHash() {
	return passwordHash;
}
