//
// Created by Екатерина on 14.01.2026.
//

#include "../include/Payment.h"
#include <iostream>
#include <iomanip>

//РЕАЛИЗАЦИЯ CreditCardPayment

bool CreditCardPayment::pay(double amount) {
    std::cout << "\n=== ОПЛАТА БАНКОВСКОЙ КАРТОЙ ===" << std::endl;
    std::cout << "Держатель карты: " << cardHolder << std::endl;
    std::cout << "Карта: **** **** **** " <<
        cardNumber.substr(cardNumber.length() - 4) << std::endl;
    std::cout << "Срок действия: " << expiryDate << std::endl;
    std::cout << "Сумма: $" << std::fixed << std::setprecision(2) << amount << std::endl;

    // Симуляция обработки платежа
    std::cout << "Отправка запроса в банк..." << std::endl;
    std::cout << "Ожидание ответа..." << std::endl;

    // В реальной системе здесь был бы запрос к платежному шлюзу
    bool paymentSuccess = true; // Симуляция успешного платежа

    if (paymentSuccess) {
        std::cout << "Платеж одобрен банком" << std::endl;
        return true;
    } else {
        std::cout << "Платеж отклонен банком" << std::endl;
        return false;
    }
}

std::string CreditCardPayment::getName() const {
    return "Банковская карта";
}

// РЕАЛИЗАЦИЯ WalletPayment

bool WalletPayment::pay(double amount) {
    std::cout << "\n=== ОПЛАТА ЭЛЕКТРОННЫМ КОШЕЛЬКОМ ===" << std::endl;
    std::cout << "Тип кошелька: " << walletType << std::endl;
    std::cout << "ID кошелька: " << walletId << std::endl;
    std::cout << "Сумма: $" << std::fixed << std::setprecision(2) << amount << std::endl;

    // Симуляция обработки платежа
    std::cout << "Подключение к платежной системе..." << std::endl;
    std::cout << "Списание средств с кошелька..." << std::endl;

    bool paymentSuccess = true; // Симуляция успешного платежа

    if (paymentSuccess) {
        std::cout << "Средства успешно списаны" << std::endl;
        return true;
    } else {
        std::cout << "Недостаточно средств на кошельке" << std::endl;
        return false;
    }
}

std::string WalletPayment::getName() const {
    return "Электронный кошелек (" + walletType + ")";
}

// РЕАЛИЗАЦИЯ SBPPayment
bool SBPPayment::pay(double amount) {
    std::cout << "\n=== ОПЛАТА ЧЕРЕЗ СБП ===" << std::endl;
    std::cout << "Банк: " << bankName << std::endl;
    std::cout << "Номер телефона: " << phoneNumber << std::endl;
    std::cout << "Сумма: $" << std::fixed << std::setprecision(2) << amount << std::endl;

    // Симуляция обработки платежа
    std::cout << "Генерация QR-кода..." << std::endl;
    std::cout << "Ожидание подтверждения платежа..." << std::endl;
    std::cout << "Проверка статуса в банке..." << std::endl;

    bool paymentSuccess = true; // Симуляция успешного платежа

    if (paymentSuccess) {
        std::cout << "Платеж подтвержден через СБП" << std::endl;
        return true;
    } else {
        std::cout << "Платеж не подтвержден" << std::endl;
        return false;
    }
}

std::string SBPPayment::getName() const {
    return "СБП (" + bankName + ")";
}