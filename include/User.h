// include/User.h
#ifndef USER_H
#define USER_H

#include <memory>      // Для умных указателей
#include <vector>      // Для контейнеров
#include <string>      // Для строк
#include <functional>  // Для лямбда-функций

// Предварительные объявления (чтобы избежать циклических зависимостей)
class Order;
template<typename T> class DatabaseConnection;

// БАЗОВЫЙ КЛАСС User (АБСТРАКТНЫЙ)
class User {
protected:
    // Данные пользователя
    int userId;
    std::string name;
    std::string email;
    std::string role;

    // АГРЕГАЦИЯ: shared_ptr для заказов
    // Заказы могут сущ независимо от пользователя
    std::vector<std::shared_ptr<Order>> orders;

    // Подключение к БД
    std::shared_ptr<DatabaseConnection<std::string>> db;

public:
    // Конструктор
    User(int id, const std::string& name, const std::string& email,
         const std::string& role,
         std::shared_ptr<DatabaseConnection<std::string>> dbConn);

    //ВИРТУАЛЬНЫЙ ДЕСТРУКТОР (для полиморфизма)
    virtual ~User() = default;

    //  ВИРТУАЛЬНЫЕ ФУНКЦИИ
    virtual void createOrder(const std::vector<std::pair<int, int>>& products) = 0;
    virtual std::string viewOrderStatus(int orderId) = 0;
    virtual bool cancelOrder(int orderId) = 0;

    // ОБЩИЕ МЕТОДЫ (не виртуальные)
    int getUserId() const { return userId; }
    std::string getName() const { return name; }
    std::string getEmail() const { return email; }
    std::string getRole() const { return role; }

    // Методы для работы с заказами (агрегация)
    void addOrder(std::shared_ptr<Order> order);
    std::vector<std::shared_ptr<Order>> getOrders() const;

    // ЛЯМБДА-ФУНКЦИЯ для проверки прав
    static std::function<bool(const User&, const std::string&)> getAccessChecker() {
        // Лямбда-функция проверяет, есть ли у пользователя нужная роль
        return [](const User& user, const std::string& requiredRole) -> bool {
            return user.getRole() == requiredRole;
        };
    }
};

//КЛАСС-НАСЛЕДНИК Admin
class Admin : public User {
public:
    Admin(int id, const std::string& name, const std::string& email,
          std::shared_ptr<DatabaseConnection<std::string>> dbConn);

    // ПЕРЕОПРЕДЕЛЕНИЕ виртуальных функций
    void createOrder(const std::vector<std::pair<int, int>>& products) override;
    std::string viewOrderStatus(int orderId) override;
    bool cancelOrder(int orderId) override;

    // СПЕЦИФИЧНЫЕ МЕТОДЫ Admin
    bool addProduct(const std::string& name, double price, int stockQuantity);
    bool updateProduct(int productId, const std::string& name,
                      double price, int stockQuantity);
    bool deleteProduct(int productId);

    // Просмотр всех заказов
    std::vector<std::vector<std::string>> viewAllOrders();

    // Обновление статуса заказа через хранимую процедуру
    bool updateOrderStatus(int orderId, const std::string& newStatus);

    // Работа с аудитом
    std::vector<std::vector<std::string>> getAuditLog();
    std::vector<std::vector<std::string>> getAuditLogByUser(int userId);

    // Генерация CSV отчета
    bool generateCSVReport(const std::string& filename);
};

// КЛАСС-НАСЛЕДНИК Manager
class Manager : public User {
public:
    Manager(int id, const std::string& name, const std::string& email,
            std::shared_ptr<DatabaseConnection<std::string>> dbConn);

    //  ПЕРЕОПРЕДЕЛЕНИЕ виртуальных функций
    void createOrder(const std::vector<std::pair<int, int>>& products) override;
    std::string viewOrderStatus(int orderId) override;
    bool cancelOrder(int orderId) override;

    //  СПЕЦИФИЧНЫЕ МЕТОДЫ Manager
    bool approveOrder(int orderId);
    bool updateStock(int productId, int newQuantity);

    // Просмотр ожидающих заказов
    std::vector<std::vector<std::string>> getPendingOrders();

    // История утвержденных заказов
    std::vector<std::vector<std::string>> getApprovedOrdersHistory();
};

// КЛАСС-НАСЛЕДНИК Customer
class Customer : public User {
private:
    int loyaltyLevel;  // Уровень лояльности: 0-обычный, 1-премиум

public:
    Customer(int id, const std::string& name, const std::string& email,
             int loyalty, std::shared_ptr<DatabaseConnection<std::string>> dbConn);

    // ПЕРЕОПРЕДЕЛЕНИЕ виртуальных функций
    void createOrder(const std::vector<std::pair<int, int>>& products) override;
    std::string viewOrderStatus(int orderId) override;
    bool cancelOrder(int orderId) override;

    //  СПЕЦИФИЧНЫЕ МЕТОДЫ Customer
    bool addToOrder(int orderId, int productId, int quantity);
    bool removeFromOrder(int orderItemId);
    bool makePayment(int orderId, const std::string& paymentMethod);

    // Возврат товара
    bool returnOrder(int orderId);

    // Просмотр истории своих заказов
    std::vector<std::vector<std::string>> getMyOrderHistory();

    // Геттер для уровня лояльности
    int getLoyaltyLevel() const { return loyaltyLevel; }
};

#endif // USER_H