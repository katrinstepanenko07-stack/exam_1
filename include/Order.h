// include/Order.h
#ifndef ORDER_H
#define ORDER_H

#include <memory>       // Для умных указателей
#include <vector>       // Для контейнеров
#include <string>       // Для строк
#include <algorithm>    // Для STL алгоритмов
#include <numeric>      // Для std::accumulate
#include <functional>   // Для лямбда-функций

// Предварительное объявление
class PaymentStrategy;

//  КЛАСС OrderItem (КОМПОЗИЦИЯ с Order)
class OrderItem {
private:
    int itemId;
    int productId;
    std::string productName;
    int quantity;
    double price;

public:
    OrderItem(int id, int prodId, const std::string& name, int qty, double pr)
        : itemId(id), productId(prodId), productName(name), quantity(qty), price(pr) {}

    // Запрещаем копирование (композиция)
    OrderItem(const OrderItem&) = delete;
    OrderItem& operator=(const OrderItem&) = delete;

    // Разрешаем перемещение
    OrderItem(OrderItem&&) = default;
    OrderItem& operator=(OrderItem&&) = default;

    // Методы
    double getTotal() const {
        return quantity * price;
    }

    // Геттеры
    int getItemId() const { return itemId; }
    int getProductId() const { return productId; }
    std::string getProductName() const { return productName; }
    int getQuantity() const { return quantity; }
    double getPrice() const { return price; }
};

// АБСТРАКТНЫЙ КЛАСС PaymentStrategy
class PaymentStrategy {
public:
    virtual ~PaymentStrategy() = default;

    //  ВИРТУАЛЬНАЯ ФУНКЦИЯ
    virtual bool pay(double amount) = 0;
    virtual std::string getName() const = 0;
};

//  КЛАСС Payment (КОМПОЗИЦИЯ с Order)
class Payment {
private:
    std::unique_ptr<PaymentStrategy> strategy;  // ⭐ unique_ptr - владение
    double amount;
    bool isCompleted;
    std::string transactionId;

public:
    Payment(double amt, std::unique_ptr<PaymentStrategy> strat);

    // Запрещаем копирование
    Payment(const Payment&) = delete;
    Payment& operator=(const Payment&) = delete;

    // Метод выполнения оплаты
    bool process();

    // Геттеры
    bool getStatus() const { return isCompleted; }
    double getAmount() const { return amount; }
    std::string getTransactionId() const { return transactionId; }

private:
    // Генерация ID транзакции
    std::string generateTransactionId();
};

//  КЛАСС Order (основной)
class Order {
private:
    int orderId;
    int userId;
    std::string status;  // pending, completed, canceled, returned
    double totalPrice;

    //КОМПОЗИЦИЯ: Order ВЛАДЕЕТ OrderItem через unique_ptr
    std::vector<std::unique_ptr<OrderItem>> items;

    //КОМПОЗИЦИЯ: Order ВЛАДЕЕТ Payment через unique_ptr
    std::unique_ptr<Payment> payment;

public:
    // Конструктор
    Order(int id, int uid, const std::string& stat, double total);

    // Запрещаем копирование
    Order(const Order&) = delete;
    Order& operator=(const Order&) = delete;

    // Разрешаем перемещение
    Order(Order&&) = default;
    Order& operator=(Order&&) = default;

    // МЕТОДЫ ДЛЯ РАБОТЫ С ЭЛЕМЕНТАМИ ЗАКАЗА
    void addItem(std::unique_ptr<OrderItem> item);
    bool removeItem(int productId);

    //  МЕТОДЫ ДЛЯ ОПЛАТЫ
    void createPayment(std::unique_ptr<PaymentStrategy> strategy);
    bool processPayment();

    // ЛЯМБДА-ФУНКЦИИ и STL АЛГОРИТМЫ

    // 1. Фильтрация заказов по статусу (лямбда + std::copy_if)
    static std::vector<std::shared_ptr<Order>> filterOrdersByStatus(
        const std::vector<std::shared_ptr<Order>>& orders,
        const std::string& targetStatus);

    // 2. Подсчет общей суммы (лямбда + std::accumulate)
    double calculateTotal() const;

    // 3. Подсчет количества заказов в статусе
    static int countOrdersByStatus(
        const std::vector<std::shared_ptr<Order>>& orders,
        const std::string& targetStatus);

    // ГЕТТЕРЫ и СЕТТЕРЫ
    int getOrderId() const { return orderId; }
    int getUserId() const { return userId; }
    std::string getStatus() const { return status; }
    double getTotalPrice() const { return totalPrice; }
    const std::vector<std::unique_ptr<OrderItem>>& getItems() const { return items; }

    void setStatus(const std::string& newStatus) { status = newStatus; }
    void setTotalPrice(double price) { totalPrice = price; }
};

#endif // ORDER_H