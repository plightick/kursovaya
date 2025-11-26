// Lightweight custom exceptions for domain errors
#pragma once

#include <stdexcept>
#include <string>

class BankingError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class NotFoundError : public BankingError {
public:
    using BankingError::BankingError;
};

class ValidationError : public BankingError {
public:
    using BankingError::BankingError;
};

class AuthError : public BankingError {
public:
    using BankingError::BankingError;
};

class CardExpiredError : public BankingError {
public:
    using BankingError::BankingError;
};


