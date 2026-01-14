// include/Payment.h
#ifndef PAYMENT_H
#define PAYMENT_H

#include <iostream>
#include <memory>
#include <string>
#include <ctime>
#include <random>

// КОНКРЕТНЫЕ СТРАТЕГИИ ОПЛАТЫ (наследники)
// 1. Оплата банковской картой
class CreditCardPayment : public PaymentStrategy {
private:
    std::string cardNumber;
    std::string cardHolder;
    std::string expiryDate;

public:
    CreditCardPayment(const std::string& card, const std::string& holder,
                     const std::string& expiry)
        : cardNumber(card), cardHolder(holder), expiryDate(expiry) {}

    bool pay(double amount) override;
    std::string getName() const override;
};

// 2. Оплата электронным кошельком
class WalletPayment : public PaymentStrategy {
private:
    std::string walletId;
    std::string walletType;  // Яндекс.Деньги, Qiwi, etc

public:
    WalletPayment(const std::string& id, const std::string& type)
        : walletId(id), walletType(type) {}

    bool pay(double amount) override;
    std::string getName() const override;
};

// 3. Оплата через СБП
class SBPPayment : public PaymentStrategy {
private:
    std::string phoneNumber;
    std::string bankName;

public:
    SBPPayment(const std::string& phone, const std::string& bank)
        : phoneNumber(phone), bankName(bank) {}

    bool pay(double amount) override;
    std::string getName() const override;
};

#endif