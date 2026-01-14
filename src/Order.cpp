// src/Order.cpp
#include "../include/Order.h"
#include "../include/Payment.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <iomanip>

// РЕАЛИЗАЦИЯ КЛАССА Payment
Payment::Payment(double amt, std::unique_ptr<PaymentStrategy> strat)
    : amount(amt), strategy(std::move(strat)), isCompleted(false) {
    transactionId = generateTransactionId();
}

bool Payment::process() {
    if (!strategy) {
        std::cerr << "Стратегия оплаты не установлена" << std::endl;
        return false;
    }

    std::cout << "Обработка оплаты..." << std::endl;
    std::cout << "Сумма: $" << std::fixed << std::setprecision(2) << amount << std::endl;
    std::cout << "Способ: " << strategy->getName() << std::endl;
    std::cout << "ID транзакции: " << transactionId << std::endl;

    isCompleted = strategy->pay(amount);

    if (isCompleted) {
        std::cout << "Оплата успешно завершена" << std::endl;
    } else {
        std::cout << "Ошибка оплаты" << std::endl;
    }

    return isCompleted;
}

std::string Payment::generateTransactionId() {
    // Генерация уникального ID транзакции
    std::time_t now = std::time(nullptr);
    std::string timestamp = std::to_string(now);

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);

    return "TRX-" + timestamp + "-" + std::to_string(dis(gen));
}

//РЕАЛИЗАЦИЯ КЛАССА Order
Order::Order(int id, int uid, const std::string& stat, double total)
    : orderId(id), userId(uid), status(stat), totalPrice(total) {}

void Order::addItem(std::unique_ptr<OrderItem> item) {
    items.push_back(std::move(item));
}

bool Order::removeItem(int productId) {
    // ИСПОЛЬЗОВАНИЕ STL АЛГОРИТМА find_if С ЛЯМБДОЙ
    auto it = std::find_if(items.begin(), items.end(),
        [productId](const std::unique_ptr<OrderItem>& item) {
            return item->getProductId() == productId;
        });

    if (it != items.end()) {
        items.erase(it);
        return true;
    }
    return false;
}

void Order::createPayment(std::unique_ptr<PaymentStrategy> strategy) {
    payment = std::make_unique<Payment>(totalPrice, std::move(strategy));
}

bool Order::processPayment() {
    if (payment) {
        bool result = payment->process();
        if (result) {
            status = "completed";
        }
        return result;
    }
    return false;
}

//ЛЯМБДА-ФУНКЦИЯ для фильтрации заказов по статусу
std::vector<std::shared_ptr<Order>> Order::filterOrdersByStatus(
    const std::vector<std::shared_ptr<Order>>& orders,
    const std::string& targetStatus) {

    std::vector<std::shared_ptr<Order>> filteredOrders;

    //ИСПОЛЬЗОВАНИЕ STL АЛГОРИТМА copy_if С ЛЯМБДОЙ
    std::copy_if(orders.begin(), orders.end(), std::back_inserter(filteredOrders),
        [&targetStatus](const std::shared_ptr<Order>& order) {
            return order->getStatus() == targetStatus;
        });

    return filteredOrders;
}

//ЛЯМБДА-ФУНКЦИЯ для подсчета общей суммы
double Order::calculateTotal() const {
    //ИСПОЛЬЗОВАНИЕ STL АЛГОРИТМА accumulate С ЛЯМБДОЙ
    return std::accumulate(items.begin(), items.end(), 0.0,
        [](double sum, const std::unique_ptr<OrderItem>& item) {
            return sum + item->getTotal();
        });
}

// ЛЯМБДА-ФУНКЦИЯ для подсчета заказов по статусу
int Order::countOrdersByStatus(
    const std::vector<std::shared_ptr<Order>>& orders,
    const std::string& targetStatus) {

    // Еще один способ с использованием count_if
    return std::count_if(orders.begin(), orders.end(),
        [&targetStatus](const std::shared_ptr<Order>& order) {
            return order->getStatus() == targetStatus;
        });
}