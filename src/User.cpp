// src/User.cpp
#include "../include/DatabaseConnection.h"
#include "../include/User.h"
#include "../include/Order.h"
#include <iostream>
#include <sstream>

//  РЕАЛИЗАЦИЯ БАЗОВОГО КЛАССА User
User::User(int id, const std::string& name, const std::string& email,
           const std::string& role,
           std::shared_ptr<DatabaseConnection<std::string>> dbConn)
    : userId(id), name(name), email(email), role(role), db(dbConn) {}

void User::addOrder(std::shared_ptr<Order> order) {
    orders.push_back(order);
}

std::vector<std::shared_ptr<Order>> User::getOrders() const {
    return orders;
}

//  РЕАЛИЗАЦИЯ КЛАССА Admin
Admin::Admin(int id, const std::string& name, const std::string& email,
             std::shared_ptr<DatabaseConnection<std::string>> dbConn)
    : User(id, name, email, "admin", dbConn) {}

// Реализация виртуальных функций Admin
void Admin::createOrder(const std::vector<std::pair<int, int>>& products) {
    std::cout << "Администратор создает заказ..." << std::endl;
    // Админ может создавать заказы для других пользователей
}

std::string Admin::viewOrderStatus(int orderId) {
    auto result = db->executeQuery(
        "SELECT getOrderStatus(" + std::to_string(orderId) + ")"
    );

    if (!result.empty() && !result[0].empty()) {
        return result[0][0];
    }
    return "Заказ не найден";
}

bool Admin::cancelOrder(int orderId) {
    // Используем транзакцию для отмены заказа
    db->beginTransaction();

    try {
        // 1. Обновляем статус заказа
        bool success = db->executeNonQuery(
            "UPDATE orders SET status = 'canceled' WHERE order_id = " +
            std::to_string(orderId)
        );

        if (!success) {
            db->rollbackTransaction();
            return false;
        }

        // 2. Возвращаем товары на склад
        auto items = db->executeQuery(
            "SELECT product_id, quantity FROM order_items WHERE order_id = " +
            std::to_string(orderId)
        );

        for (const auto& item : items) {
            if (item.size() >= 2) {
                std::string updateStock =
                    "UPDATE products SET stock_quantity = stock_quantity + " +
                    item[1] + " WHERE product_id = " + item[0];

                if (!db->executeNonQuery(updateStock)) {
                    db->rollbackTransaction();
                    return false;
                }
            }
        }

        // 3. Записываем в аудит
        std::string auditSQL =
            "INSERT INTO audit_log (entity_type, entity_id, operation, performed_by, details) "
            "VALUES ('order', " + std::to_string(orderId) + ", 'update', " +
            std::to_string(userId) + ", 'Заказ отменен администратором')";

        if (!db->executeNonQuery(auditSQL)) {
            db->rollbackTransaction();
            return false;
        }

        db->commitTransaction();
        return true;

    } catch (const std::exception& e) {
        db->rollbackTransaction();
        std::cerr << "Ошибка при отмене заказа: " << e.what() << std::endl;
        return false;
    }
}

// Специфичные методы Admin
bool Admin::addProduct(const std::string& name, double price, int stockQuantity) {
    std::string sql =
        "INSERT INTO products (name, price, stock_quantity) VALUES ('" +
        name + "', " + std::to_string(price) + ", " +
        std::to_string(stockQuantity) + ")";

    bool success = db->executeNonQuery(sql);

    if (success) {
        // Аудит операции
        std::string auditSQL =
            "INSERT INTO audit_log (entity_type, operation, performed_by, details) "
            "VALUES ('product', 'insert', " + std::to_string(userId) +
            ", 'Добавлен товар: " + name + "')";
        db->executeNonQuery(auditSQL);
    }

    return success;
}

bool Admin::updateProduct(int productId, const std::string& name,
                         double price, int stockQuantity) {
    std::string sql =
        "UPDATE products SET name = '" + name + "', price = " +
        std::to_string(price) + ", stock_quantity = " +
        std::to_string(stockQuantity) + " WHERE product_id = " +
        std::to_string(productId);

    return db->executeNonQuery(sql);
}

bool Admin::deleteProduct(int productId) {
    std::string sql =
        "DELETE FROM products WHERE product_id = " + std::to_string(productId);

    return db->executeNonQuery(sql);
}

std::vector<std::vector<std::string>> Admin::viewAllOrders() {
    return db->executeQuery(
        "SELECT o.order_id, u.name as customer, o.status, "
        "o.total_price, o.order_date, COUNT(oi.order_item_id) as items_count "
        "FROM orders o "
        "JOIN users u ON o.user_id = u.user_id "
        "LEFT JOIN order_items oi ON o.order_id = oi.order_id "
        "GROUP BY o.order_id, u.name, o.status, o.total_price, o.order_date "
        "ORDER BY o.order_date DESC"
    );
}

bool Admin::updateOrderStatus(int orderId, const std::string& newStatus) {
    // Используем хранимую процедуру
    std::string sql =
        "CALL updateOrderStatus(" + std::to_string(orderId) + ", '" +
        newStatus + "', " + std::to_string(userId) + ", NULL)";

    return db->executeNonQuery(sql);
}

std::vector<std::vector<std::string>> Admin::getAuditLog() {
    return db->executeQuery(
        "SELECT a.log_id, a.entity_type, a.entity_id, a.operation, "
        "u.name as performed_by, a.performed_at, a.details "
        "FROM audit_log a "
        "LEFT JOIN users u ON a.performed_by = u.user_id "
        "ORDER BY a.performed_at DESC "
        "LIMIT 100"
    );
}

std::vector<std::vector<std::string>> Admin::getAuditLogByUser(int userId) {
    return db->executeQuery(
        "SELECT * FROM getAuditLogByUser(" + std::to_string(userId) + ")"
    );
}

bool Admin::generateCSVReport(const std::string& filename) {
    std::cout << "Генерация CSV отчета: " << filename << std::endl;

    auto reportData = db->executeQuery(
        "SELECT o.order_id, u.name as customer, o.status, "
        "h.new_status, h.changed_at, a.operation, a.performed_at, a.details "
        "FROM orders o "
        "LEFT JOIN users u ON o.user_id = u.user_id "
        "LEFT JOIN order_status_history h ON o.order_id = h.order_id "
        "LEFT JOIN audit_log a ON o.order_id = a.entity_id AND a.entity_type = 'order' "
        "WHERE o.order_date >= CURRENT_DATE - INTERVAL '30 days' "
        "ORDER BY o.order_id, h.changed_at"
    );

    std::cout << "Отчет содержит " << reportData.size() << " записей" << std::endl;
    return !reportData.empty();
}

//  РЕАЛИЗАЦИЯ КЛАССА Manager
Manager::Manager(int id, const std::string& name, const std::string& email,
                 std::shared_ptr<DatabaseConnection<std::string>> dbConn)
    : User(id, name, email, "manager", dbConn) {}

// Реализация виртуальных функций Manager
void Manager::createOrder(const std::vector<std::pair<int, int>>& products) {
    std::cout << "Менеджер создает заказ..." << std::endl;
    // Менеджер может создавать заказы для клиентов
}

std::string Manager::viewOrderStatus(int orderId) {
    auto result = db->executeQuery(
        "SELECT status FROM orders WHERE order_id = " + std::to_string(orderId)
    );

    if (!result.empty() && !result[0].empty()) {
        return result[0][0];
    }
    return "Заказ не найден";
}

bool Manager::cancelOrder(int orderId) {
    // Менеджер может отменять только pending заказы
    auto statusResult = db->executeQuery(
        "SELECT status FROM orders WHERE order_id = " + std::to_string(orderId)
    );

    if (!statusResult.empty() && statusResult[0][0] == "pending") {
        return db->executeNonQuery(
            "UPDATE orders SET status = 'canceled' WHERE order_id = " +
            std::to_string(orderId)
        );
    }
    return false;
}

// спецц методы Manager
bool Manager::approveOrder(int orderId) {
    // Используем транзакцию для утверждения заказа
    db->beginTransaction();

    try {
        // 1. Проверяем, что заказ существует и в статусе pending
        auto checkResult = db->executeQuery(
            "SELECT status FROM orders WHERE order_id = " +
            std::to_string(orderId) + " AND status = 'pending'"
        );

        if (checkResult.empty()) {
            db->rollbackTransaction();
            return false;
        }

        // 2. Обновляем статус на completed
        bool success = db->executeNonQuery(
            "UPDATE orders SET status = 'completed' WHERE order_id = " +
            std::to_string(orderId)
        );

        if (!success) {
            db->rollbackTransaction();
            return false;
        }

        // 3. Записываем в аудит
        std::string auditSQL =
            "INSERT INTO audit_log (entity_type, entity_id, operation, performed_by, details) "
            "VALUES ('order', " + std::to_string(orderId) + ", 'update', " +
            std::to_string(userId) + ", 'Заказ утвержден менеджером')";

        if (!db->executeNonQuery(auditSQL)) {
            db->rollbackTransaction();
            return false;
        }

        db->commitTransaction();
        return true;

    } catch (const std::exception& e) {
        db->rollbackTransaction();
        std::cerr << "Ошибка при утверждении заказа: " << e.what() << std::endl;
        return false;
    }
}

bool Manager::updateStock(int productId, int newQuantity) {
    if (newQuantity < 0) {
        std::cerr << "Количество не может быть отрицательным" << std::endl;
        return false;
    }

    std::string sql =
        "UPDATE products SET stock_quantity = " + std::to_string(newQuantity) +
        " WHERE product_id = " + std::to_string(productId);

    bool success = db->executeNonQuery(sql);

    if (success) {
        // Аудит операции
        std::string auditSQL =
            "INSERT INTO audit_log (entity_type, entity_id, operation, performed_by, details) "
            "VALUES ('product', " + std::to_string(productId) + ", 'update', " +
            std::to_string(userId) + ", 'Обновлено количество на складе: " +
            std::to_string(newQuantity) + "')";
        db->executeNonQuery(auditSQL);
    }

    return success;
}

std::vector<std::vector<std::string>> Manager::getPendingOrders() {
    return db->executeQuery(
        "SELECT o.order_id, u.name as customer, o.total_price, "
        "o.order_date, COUNT(oi.order_item_id) as items_count "
        "FROM orders o "
        "JOIN users u ON o.user_id = u.user_id "
        "LEFT JOIN order_items oi ON o.order_id = oi.order_id "
        "WHERE o.status = 'pending' "
        "GROUP BY o.order_id, u.name, o.total_price, o.order_date "
        "ORDER BY o.order_date"
    );
}

std::vector<std::vector<std::string>> Manager::getApprovedOrdersHistory() {
    return db->executeQuery(
        "SELECT o.order_id, u.name as customer, o.total_price, "
        "o.order_date, o.status "
        "FROM orders o "
        "JOIN users u ON o.user_id = u.user_id "
        "WHERE o.status = 'completed' "
        "AND EXISTS (SELECT 1 FROM audit_log a WHERE a.entity_id = o.order_id "
        "AND a.performed_by = " + std::to_string(userId) +
        " AND a.details LIKE '%утвержден менеджером%') "
        "ORDER BY o.order_date DESC"
    );
}

// РЕАЛИЗАЦИЯ КЛАССА Customer
Customer::Customer(int id, const std::string& name, const std::string& email,
                   int loyalty, std::shared_ptr<DatabaseConnection<std::string>> dbConn)
    : User(id, name, email, "customer", dbConn), loyaltyLevel(loyalty) {}

// Реализация виртуальных функций Customer
void Customer::createOrder(const std::vector<std::pair<int, int>>& products) {
    if (products.empty()) {
        std::cout << "Нельзя создать пустой заказ!" << std::endl;
        return;
    }

    std::cout << "Создание заказа для клиента " << name << "..." << std::endl;

    // Преобразуем продукты в JSON для хранимой процедуры
    std::string jsonProducts = "[";
    for (size_t i = 0; i < products.size(); ++i) {
        jsonProducts += "{\"product_id\": " + std::to_string(products[i].first) +
                       ", \"quantity\": " + std::to_string(products[i].second) + "}";
        if (i < products.size() - 1) jsonProducts += ",";
    }
    jsonProducts += "]";

    // Вызываем хранимую процедуру createOrder
    std::string sql = "CALL createOrder(" + std::to_string(userId) +
                     ", '" + jsonProducts + "'::jsonb, NULL, NULL)";

    if (db->executeNonQuery(sql)) {
        std::cout << "✅ Заказ успешно создан!" << std::endl;
    } else {
        std::cout << "❌ Ошибка при создании заказа" << std::endl;
    }
}

std::string Customer::viewOrderStatus(int orderId) {
    // Проверяем, что заказ принадлежит этому пользователю
    auto checkResult = db->executeQuery(
        "SELECT user_id FROM orders WHERE order_id = " + std::to_string(orderId)
    );

    if (!checkResult.empty() && std::stoi(checkResult[0][0]) == userId) {
        auto result = db->executeQuery(
            "SELECT getOrderStatus(" + std::to_string(orderId) + ")"
        );

        if (!result.empty() && !result[0].empty()) {
            return result[0][0];
        }
    }

    return "Заказ не найден или доступ запрещен";
}

bool Customer::cancelOrder(int orderId) {
    // Проверяем, что заказ принадлежит пользователю и в статусе pending
    auto checkResult = db->executeQuery(
        "SELECT status FROM orders WHERE order_id = " +
        std::to_string(orderId) + " AND user_id = " + std::to_string(userId)
    );

    if (!checkResult.empty() && checkResult[0][0] == "pending") {
        return db->executeNonQuery(
            "UPDATE orders SET status = 'canceled' WHERE order_id = " +
            std::to_string(orderId)
        );
    }
    return false;
}

// Спец методы Customer
bool Customer::addToOrder(int orderId, int productId, int quantity) {
    if (quantity <= 0) {
        std::cerr << "Количество должно быть больше 0" << std::endl;
        return false;
    }

    // Проверяем, что заказ принадлежит пользователю и еще не завершен
    auto checkResult = db->executeQuery(
        "SELECT status FROM orders WHERE order_id = " +
        std::to_string(orderId) + " AND user_id = " + std::to_string(userId)
    );

    if (checkResult.empty() || checkResult[0][0] != "pending") {
        std::cerr << "Нельзя добавить товар в этот заказ" << std::endl;
        return false;
    }

    // Получаем цену продукта
    auto priceResult = db->executeQuery(
        "SELECT price FROM products WHERE product_id = " + std::to_string(productId)
    );

    if (priceResult.empty()) {
        std::cerr << "Товар не найден" << std::endl;
        return false;
    }

    double price = std::stod(priceResult[0][0]);

    // Добавляем товар в заказ
    std::string sql =
        "INSERT INTO order_items (order_id, product_id, quantity, price) VALUES (" +
        std::to_string(orderId) + ", " + std::to_string(productId) + ", " +
        std::to_string(quantity) + ", " + std::to_string(price) + ")";

    return db->executeNonQuery(sql);
}

bool Customer::removeFromOrder(int orderItemId) {
    // Проверяем, что элемент заказа принадлежит заказу пользователя
    auto checkResult = db->executeQuery(
        "SELECT o.order_id FROM order_items oi "
        "JOIN orders o ON oi.order_id = o.order_id "
        "WHERE oi.order_item_id = " + std::to_string(orderItemId) +
        " AND o.user_id = " + std::to_string(userId) +
        " AND o.status = 'pending'"
    );

    if (checkResult.empty()) {
        std::cerr << "Нельзя удалить этот товар из заказа" << std::endl;
        return false;
    }

    std::string sql =
        "DELETE FROM order_items WHERE order_item_id = " +
        std::to_string(orderItemId);

    return db->executeNonQuery(sql);
}

bool Customer::makePayment(int orderId, const std::string& paymentMethod) {
    // Проверяем, что заказ принадлежит пользователю и в статусе pending
    auto checkResult = db->executeQuery(
        "SELECT status, total_price FROM orders WHERE order_id = " +
        std::to_string(orderId) + " AND user_id = " + std::to_string(userId)
    );

    if (checkResult.empty() || checkResult[0][0] != "pending") {
        std::cerr << "Нельзя оплатить этот заказ" << std::endl;
        return false;
    }

    // Обновляем заказ - устанавливаем способ оплаты и статус
    std::string sql =
        "UPDATE orders SET status = 'completed', payment_method = '" +
        paymentMethod + "', payment_status = 'paid' WHERE order_id = " +
        std::to_string(orderId);

    return db->executeNonQuery(sql);
}

bool Customer::returnOrder(int orderId) {
    // Проверяем возможность возврата через функцию canReturnOrder
    auto canReturnResult = db->executeQuery(
        "SELECT canReturnOrder(" + std::to_string(orderId) + ")"
    );

    if (!canReturnResult.empty() && canReturnResult[0][0] == "t") {
        // Проверяем, что заказ принадлежит пользователю
        auto checkResult = db->executeQuery(
            "SELECT user_id FROM orders WHERE order_id = " + std::to_string(orderId)
        );

        if (!checkResult.empty() && std::stoi(checkResult[0][0]) == userId) {
            return db->executeNonQuery(
                "UPDATE orders SET status = 'returned' WHERE order_id = " +
                std::to_string(orderId)
            );
        }
    }

    std::cout << "Нельзя вернуть этот заказ" << std::endl;
    return false;
}

std::vector<std::vector<std::string>> Customer::getMyOrderHistory() {
    return db->executeQuery(
        "SELECT o.order_id, o.status, o.total_price, o.order_date, "
        "COUNT(oi.order_item_id) as items_count "
        "FROM orders o "
        "LEFT JOIN order_items oi ON o.order_id = oi.order_id "
        "WHERE o.user_id = " + std::to_string(userId) +
        " GROUP BY o.order_id, o.status, o.total_price, o.order_date "
        "ORDER BY o.order_date DESC"
    );
}