// src/main.cpp
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include "../include/DatabaseConnection.h"
#include "../include/User.h"
#include "../include/Order.h"
#include "../include/Payment.h"

// Функция для отображения таблицы
void printTable(const std::vector<std::vector<std::string>>& data,
                const std::vector<std::string>& headers) {
    if (data.empty()) {
        std::cout << "Нет данных для отображения" << std::endl;
        return;
    }

    std::vector<size_t> columnWidths(headers.size(), 0);

    for (size_t i = 0; i < headers.size(); i++) {
        columnWidths[i] = std::max(columnWidths[i], headers[i].length());
    }

    for (const auto& row : data) {
        for (size_t i = 0; i < row.size() && i < headers.size(); i++) {
            columnWidths[i] = std::max(columnWidths[i], row[i].length());
        }
    }

    // Выводим заголовки
    std::cout << "\n";
    for (size_t i = 0; i < headers.size(); i++) {
        std::cout << std::left << std::setw(columnWidths[i] + 2) << headers[i];
    }
    std::cout << "\n";

    // Линия под заголовками
    for (size_t i = 0; i < headers.size(); i++) {
        std::cout << std::string(columnWidths[i] + 2, '-');
    }
    std::cout << "\n";

    // Выводим данные
    for (const auto& row : data) {
        for (size_t i = 0; i < row.size() && i < headers.size(); i++) {
            std::cout << std::left << std::setw(columnWidths[i] + 2) << row[i];
        }
        std::cout << "\n";
    }
}

// Функция аутентификации
std::shared_ptr<User> authenticateUser(
    std::shared_ptr<DatabaseConnection<std::string>> db) {

    std::cout << "\n=== АВТОРИЗАЦИЯ ===\n";
    std::cout << "Выберите роль для входа:\n";
    std::cout << "1. Администратор\n";
    std::cout << "2. Менеджер\n";
    std::cout << "3. Покупатель\n";
    std::cout << "4. Выход\n";
    std::cout << "Ваш выбор: ";

    int choice;
    std::cin >> choice;

    //ИСПОЛЬЗОВАНИЕ ЛЯМБДА-ФУНКЦИИ для обработки ввода
    auto clearInput = []() {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    };

    switch (choice) {
        case 1: {
            // Поиск администратора в БД
            auto result = db->executeQuery(
                "SELECT user_id, name, email FROM users WHERE role = 'admin' LIMIT 1"
            );

            if (!result.empty()) {
                int id = std::stoi(result[0][0]);
                std::string name = result[0][1];
                std::string email = result[0][2];

                std::cout << "Вы вошли как Администратор: " << name << std::endl;
                return std::make_shared<Admin>(id, name, email, db);
            }
            break;
        }
        case 2: {
            // Поиск менеджера в БД
            auto result = db->executeQuery(
                "SELECT user_id, name, email FROM users WHERE role = 'manager' LIMIT 1"
            );

            if (!result.empty()) {
                int id = std::stoi(result[0][0]);
                std::string name = result[0][1];
                std::string email = result[0][2];

                std::cout << "Вы вошли как Менеджер: " << name << std::endl;
                return std::make_shared<Manager>(id, name, email, db);
            }
            break;
        }
        case 3: {
            // Поиск покупателя в БД
            auto result = db->executeQuery(
                "SELECT user_id, name, email, loyalty_level FROM users WHERE role = 'customer' LIMIT 1"
            );

            if (!result.empty()) {
                int id = std::stoi(result[0][0]);
                std::string name = result[0][1];
                std::string email = result[0][2];
                int loyalty = std::stoi(result[0][3]);

                std::cout << "Вы вошли как Покупатель: " << name;
                if (loyalty == 1) {
                    std::cout << " (Премиум)";
                }
                std::cout << std::endl;

                return std::make_shared<Customer>(id, name, email, loyalty, db);
            }
            break;
        }
        case 4:
            return nullptr;
        default:
            std::cout << "Неверный выбор!" << std::endl;
            clearInput();
            break;
    }

    std::cout << "Пользователь не найден!" << std::endl;
    return nullptr;
}

// Меню администратора
void showAdminMenu(std::shared_ptr<Admin> admin) {
    int choice;

    // ЛЯМБДА-ФУНКЦИЯ для проверки прав доступа
    auto checkAdminAccess = User::getAccessChecker();

    do {
        std::cout << "\n=== МЕНЮ АДМИНИСТРАТОРА ===\n";
        std::cout << "1. Добавить новый товар\n";
        std::cout << "2. Обновить товар\n";
        std::cout << "3. Удалить товар\n";
        std::cout << "4. Просмотреть все заказы\n";
        std::cout << "5. Просмотреть детали заказа\n";
        std::cout << "6. Изменить статус заказа\n";
        std::cout << "7. Просмотреть историю статусов заказа\n";
        std::cout << "8. Просмотреть журнал аудита\n";
        std::cout << "9. Сформировать отчет (CSV)\n";
        std::cout << "10. Выйти\n";
        std::cout << "Ваш выбор: ";
        std::cin >> choice;

        // Проверка прав доступа
        if (!checkAdminAccess(*admin, "admin")) {
            std::cout << "Доступ запрещен!" << std::endl;
            continue;
        }

        switch (choice) {
            case 1: {
                std::string name;
                double price;
                int quantity;

                std::cout << "Название товара: ";
                std::cin.ignore();
                std::getline(std::cin, name);
                std::cout << "Цена: ";
                std::cin >> price;
                std::cout << "Количество на складе: ";
                std::cin >> quantity;

                if (admin->addProduct(name, price, quantity)) {
                    std::cout << "Товар успешно добавлен!" << std::endl;
                } else {
                    std::cout << "Ошибка при добавлении товара" << std::endl;
                }
                break;
            }
            case 2: {
                int productId;
                std::string name;
                double price;
                int quantity;

                std::cout << "ID товара для обновления: ";
                std::cin >> productId;
                std::cout << "Новое название: ";
                std::cin.ignore();
                std::getline(std::cin, name);
                std::cout << "Новая цена: ";
                std::cin >> price;
                std::cout << "Новое количество: ";
                std::cin >> quantity;

                if (admin->updateProduct(productId, name, price, quantity)) {
                    std::cout << "Товар обновлен!" << std::endl;
                } else {
                    std::cout << "Ошибка при обновлении товара" << std::endl;
                }
                break;
            }
            case 3: {
                int productId;
                std::cout << "ID товара для удаления: ";
                std::cin >> productId;

                if (admin->deleteProduct(productId)) {
                    std::cout << "Товар успешно удален!" << std::endl;
                } else {
                    std::cout << "Ошибка при удалении товара" << std::endl;
                }
                break;
            }
            case 4: {
                auto orders = admin->viewAllOrders();
                printTable(orders, {"ID заказа", "Клиент", "Статус", "Сумма", "Дата", "Товаров"});
                break;
            }
            case 5: {
                int orderId;
                std::cout << "ID заказа: ";
                std::cin >> orderId;

                auto details = admin->viewOrderStatus(orderId);
                std::cout << "Статус заказа: " << details << std::endl;
                break;
            }
            case 6: {
                int orderId;
                std::string newStatus;

                std::cout << "ID заказа: ";
                std::cin >> orderId;
                std::cout << "Новый статус (pending/completed/canceled/returned): ";
                std::cin >> newStatus;

                if (admin->updateOrderStatus(orderId, newStatus)) {
                    std::cout << "Статус заказа обновлен!" << std::endl;
                } else {
                    std::cout << "Ошибка при обновлении статуса" << std::endl;
                }
                break;
            }
            case 7: {
                int orderId;
                std::cout << "ID заказа: ";
                std::cin >> orderId;

                auto history = admin->db->executeQuery(
                    "SELECT * FROM getOrderStatusHistory(" + std::to_string(orderId) + ")"
                );
                printTable(history, {"ID", "Старый статус", "Новый статус", "Дата", "Кем изменен"});
                break;
            }
            case 8: {
                auto auditLog = admin->getAuditLog();
                printTable(auditLog, {"ID", "Тип", "ID сущности", "Операция", "Кем", "Дата", "Детали"});
                break;
            }
            case 9: {
                if (admin->generateCSVReport("audit_report.csv")) {
                    std::cout << "Отчет успешно сформирован!" << std::endl;
                } else {
                    std::cout << "Ошибка при формировании отчета" << std::endl;
                }
                break;
            }
            case 10:
                std::cout << "Выход из системы..." << std::endl;
                break;
            default:
                std::cout << "Неверный выбор!" << std::endl;
        }

    } while (choice != 10);
}

// Меню менеджера
void showManagerMenu(std::shared_ptr<Manager> manager) {
    int choice;

    auto checkManagerAccess = User::getAccessChecker();

    do {
        std::cout << "\n=== МЕНЮ МЕНЕДЖЕРА ===\n";
        std::cout << "1. Просмотреть ожидающие заказы\n";
        std::cout << "2. Утвердить заказ\n";
        std::cout << "3. Обновить количество товара\n";
        std::cout << "4. Просмотреть детали заказа\n";
        std::cout << "5. Изменить статус заказа\n";
        std::cout << "6. Просмотреть историю утвержденных заказов\n";
        std::cout << "7. Выйти\n";
        std::cout << "Ваш выбор: ";
        std::cin >> choice;

        if (!checkManagerAccess(*manager, "manager")) {
            std::cout << "Доступ запрещен!" << std::endl;
            continue;
        }

        switch (choice) {
            case 1: {
                auto orders = manager->getPendingOrders();
                printTable(orders, {"ID заказа", "Клиент", "Сумма", "Дата", "Товаров"});
                break;
            }
            case 2: {
                int orderId;
                std::cout << "ID заказа для утверждения: ";
                std::cin >> orderId;

                if (manager->approveOrder(orderId)) {
                    std::cout << "Заказ утвержден!" << std::endl;
                } else {
                    std::cout << "Ошибка при утверждении заказа" << std::endl;
                }
                break;
            }
            case 3: {
                int productId, quantity;
                std::cout << "ID товара: ";
                std::cin >> productId;
                std::cout << "Новое количество: ";
                std::cin >> quantity;

                if (manager->updateStock(productId, quantity)) {
                    std::cout << "Количество товара обновлено!" << std::endl;
                } else {
                    std::cout << "Ошибка при обновлении количества" << std::endl;
                }
                break;
            }
            case 4: {
                int orderId;
                std::cout << "ID заказа: ";
                std::cin >> orderId;

                auto status = manager->viewOrderStatus(orderId);
                std::cout << "Статус заказа: " << status << std::endl;
                break;
            }
            case 5: {
                int orderId;
                std::string newStatus;

                std::cout << "ID заказа: ";
                std::cin >> orderId;
                std::cout << "Новый статус: ";
                std::cin >> newStatus;

                if (manager->cancelOrder(orderId)) {
                    std::cout << "Статус заказа изменен!" << std::endl;
                } else {
                    std::cout << "Ошибка при изменении статуса" << std::endl;
                }
                break;
            }
            case 6: {
                auto history = manager->getApprovedOrdersHistory();
                printTable(history, {"ID заказа", "Клиент", "Сумма", "Дата", "Статус"});
                break;
            }
            case 7:
                std::cout << "Выход из системы..." << std::endl;
                break;
            default:
                std::cout << "Неверный выбор!" << std::endl;
        }

    } while (choice != 7);
}

// Меню покупателя
void showCustomerMenu(std::shared_ptr<Customer> customer) {
    int choice;

    auto checkCustomerAccess = User::getAccessChecker();

    do {
        std::cout << "\n=== МЕНЮ ПОКУПАТЕЛЯ ===\n";
        std::cout << "1. Создать новый заказ\n";
        std::cout << "2. Добавить товар в заказ\n";
        std::cout << "3. Удалить товар из заказа\n";
        std::cout << "4. Просмотреть мои заказы\n";
        std::cout << "5. Просмотреть статус заказа\n";
        std::cout << "6. Оплатить заказ\n";
        std::cout << "7. Вернуть заказ\n";
        std::cout << "8. Выйти\n";
        std::cout << "Ваш выбор: ";
        std::cin >> choice;

        if (!checkCustomerAccess(*customer, "customer")) {
            std::cout << "Доступ запрещен!" << std::endl;
            continue;
        }

        switch (choice) {
            case 1: {
                std::vector<std::pair<int, int>> products;
                char addMore = 'y';

                // Показываем доступные товары
                auto availableProducts = customer->db->executeQuery(
                    "SELECT product_id, name, price, stock_quantity FROM products WHERE stock_quantity > 0"
                );

                std::cout << "\n=== ДОСТУПНЫЕ ТОВАРЫ ===\n";
                printTable(availableProducts, {"ID", "Название", "Цена", "В наличии"});

                while (addMore == 'y' || addMore == 'Y') {
                    int productId, quantity;

                    std::cout << "\nID товара: ";
                    std::cin >> productId;
                    std::cout << "Количество: ";
                    std::cin >> quantity;

                    products.push_back({productId, quantity});

                    std::cout << "Добавить еще товар? (y/n): ";
                    std::cin >> addMore;
                }

                customer->createOrder(products);
                break;
            }
            case 2: {
                int orderId, productId, quantity;

                std::cout << "ID заказа: ";
                std::cin >> orderId;
                std::cout << "ID товара: ";
                std::cin >> productId;
                std::cout << "Количество: ";
                std::cin >> quantity;

                if (customer->addToOrder(orderId, productId, quantity)) {
                    std::cout << "Товар добавлен в заказ!" << std::endl;
                } else {
                    std::cout << "Ошибка при добавлении товара" << std::endl;
                }
                break;
            }
            case 3: {
                int orderItemId;
                std::cout << "ID элемента заказа: ";
                std::cin >> orderItemId;

                if (customer->removeFromOrder(orderItemId)) {
                    std::cout << "Товар удален из заказа!" << std::endl;
                } else {
                    std::cout << "Ошибка при удалении товара" << std::endl;
                }
                break;
            }
            case 4: {
                auto orders = customer->getMyOrderHistory();
                printTable(orders, {"ID заказа", "Статус", "Сумма", "Дата", "Товаров"});
                break;
            }
            case 5: {
                int orderId;
                std::cout << "ID заказа: ";
                std::cin >> orderId;

                auto status = customer->viewOrderStatus(orderId);
                std::cout << "Статус заказа: " << status << std::endl;
                break;
            }
            case 6: {
                int orderId;
                std::cout << "ID заказа для оплаты: ";
                std::cin >> orderId;

                std::cout << "Выберите способ оплаты:\n";
                std::cout << "1. Банковская карта\n";
                std::cout << "2. Электронный кошелек\n";
                std::cout << "3. СБП\n";
                std::cout << "Ваш выбор: ";

                int paymentChoice;
                std::cin >> paymentChoice;

                std::string paymentMethod;
                std::unique_ptr<PaymentStrategy> strategy;

                switch (paymentChoice) {
                    case 1: {
                        std::string cardNumber, cardHolder, expiry;

                        std::cout << "Номер карты: ";
                        std::cin >> cardNumber;
                        std::cout << "Держатель карты: ";
                        std::cin.ignore();
                        std::getline(std::cin, cardHolder);
                        std::cout << "Срок действия (MM/YY): ";
                        std::cin >> expiry;

                        strategy = std::make_unique<CreditCardPayment>(cardNumber, cardHolder, expiry);
                        paymentMethod = "credit_card";
                        break;
                    }
                    case 2: {
                        std::string walletId, walletType;

                        std::cout << "ID кошелька: ";
                        std::cin >> walletId;
                        std::cout << "Тип кошелька (Yandex/Qiwi/etc): ";
                        std::cin >> walletType;

                        strategy = std::make_unique<WalletPayment>(walletId, walletType);
                        paymentMethod = "wallet";
                        break;
                    }
                    case 3: {
                        std::string phone, bank;

                        std::cout << "Номер телефона: ";
                        std::cin >> phone;
                        std::cout << "Банк: ";
                        std::cin >> bank;

                        strategy = std::make_unique<SBPPayment>(phone, bank);
                        paymentMethod = "sbp";
                        break;
                    }
                    default:
                        std::cout << "Неверный выбор!" << std::endl;
                        continue;
                }

                // Создаем заказ и оплачиваем его
                if (customer->makePayment(orderId, paymentMethod)) {
                    std::cout << "Заказ успешно оплачен!" << std::endl;
                } else {
                    std::cout << "Ошибка при оплате заказа" << std::endl;
                }
                break;
            }
            case 7: {
                int orderId;
                std::cout << "ID заказа для возврата: ";
                std::cin >> orderId;

                if (customer->returnOrder(orderId)) {
                    std::cout << "Заказ успешно возвращен!" << std::endl;
                } else {
                    std::cout << "Нельзя вернуть этот заказ" << std::endl;
                }
                break;
            }
            case 8:
                std::cout << "Выход из системы..." << std::endl;
                break;
            default:
                std::cout << "Неверный выбор!" << std::endl;
        }

    } while (choice != 8);
}

// Главная функция
int main() {
    std::cout << "=== СИСТЕМА ИНТЕРНЕТ-МАГАЗИНА ===\n";

    //  ПОДКЛЮЧЕНИЯ К БД
    std::string connectionString =
        "host=localhost "
        "port=5432 "
        "dbname=online_store "
        "user= "
        "password=***";

    try {
        //ИСПОЛЬЗОВАНИЕ УМНЫХ УКАЗАТЕЛЕЙ
        auto db = std::make_shared<DatabaseConnection<std::string>>(connectionString);

        if (!db->isConnected()) {
            std::cerr << "Не удалось подключиться к базе данных!" << std::endl;
            return 1;
        }

        std::cout << "Успешное подключение к базе данных!\n";

        // Главный цикл программы
        while (true) {
            auto user = authenticateUser(db);

            if (!user) {
                std::cout << "До свидания!" << std::endl;
                break;
            }

            // Показываем соответствующее меню в зависимости от роли
            std::string role = user->getRole();

            if (role == "admin") {
                auto admin = std::dynamic_pointer_cast<Admin>(user);
                if (admin) {
                    showAdminMenu(admin);
                }
            } else if (role == "manager") {
                auto manager = std::dynamic_pointer_cast<Manager>(user);
                if (manager) {
                    showManagerMenu(manager);
                }
            } else if (role == "customer") {
                auto customer = std::dynamic_pointer_cast<Customer>(user);
                if (customer) {
                    showCustomerMenu(customer);
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Критическая ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}