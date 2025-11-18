// Lightweight custom exceptions for domain errors
#pragma once

#include <stdexcept>
#include <string>

class BankingError : public std::runtime_error {
public:
    explicit BankingError(const std::string &message) : std::runtime_error(message) {}
};

class NotFoundError : public BankingError {
public:
    explicit NotFoundError(const std::string &message) : BankingError(message) {}
};

class ValidationError : public BankingError {
public:
    explicit ValidationError(const std::string &message) : BankingError(message) {}
};

class AuthError : public BankingError {
public:
    explicit AuthError(const std::string &message) : BankingError(message) {}
};

class CardExpiredError : public BankingError {
public:
    explicit CardExpiredError(const std::string &message) : BankingError(message) {}
};


