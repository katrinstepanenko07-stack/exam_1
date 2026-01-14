Задача на экзамен по информатике
Цель работы:
Разработка полнофункциональной системы управления онлайн-магазином на языке C++ с использованием принципов объектно-ориентированного программирования, умных указателей, подключением к базе данных PostgreSQL и реализацией ролевой модели доступа.

Краткое описание системы:
Система представляет собой консольное приложение для управления интернет-магазином, поддерживающее три типа пользователей:
Администраторы - полный доступ ко всем функциям
Менеджеры - управление заказами и товарами
Покупатели - просмотр каталога и оформление заказов

Используемые технологии:
C++17 -  язык программирования
PostgreSQL - система управления базами данных
libpqxx - C++ клиентская библиотека для PostgreSQL
CMake - система сборки
STL - стандартная библиотека шаблонов

Архитектура проекта
Описание классов и их взаимосвязей






User (абстрактный)
├── Admin
├── Manager
└── Customer

DatabaseConnection<T> (шаблонный)
Order ──┐
├── OrderItem
└── Payment
└── PaymentStrategy (абстрактный)
├── CreditCardPayment
├── EWalletPayment
└── SBPPayment






Применение принципов ООП
Наследование:
class User {
protected:
    int id;
    std::string username;
    UserRole role;
    
public:
    virtual void displayInfo() const = 0;
};

class Customer : public User {
private:
    std::string address;
    std::string phone;
    
public:
    void displayInfo() const override;
};
Полиморфизм:
std::vector<std::unique_ptr<User>> users;
users.push_back(std::make_unique<Customer>("John", "john@email.com"));
users.push_back(std::make_unique<Manager>("Alice", "manager"));

for (const auto& user : users) {
    user->displayInfo();  // Полиморфный вызов
}
Композиция:
cpp
class Order {
private:
    int order_id;
    std::vector<OrderItem> items;  // Композиция
    Payment payment;               // Композиция
    
public:
    void addItem(const Product& product, int quantity);
};
Агрегация:
class ShoppingCart {
private:
    Customer* customer;  // Агрегация
    std::vector<Product*> products;  // Агрегация
    
public:
    void setCustomer(Customer* cust);
};

Использование шаблонного класса DatabaseConnection<T>
template<typename T>
class DatabaseConnection {
private:
    std::unique_ptr<pqxx::connection> conn;
    
public:
    DatabaseConnection(const std::string& connection_string);
    // CRUD операции
    bool create(const T& obj);
    std::optional<T> read(int id);
    bool update(const T& obj);
    bool remove(int id);
    // Специализированные запросы
    template<typename Predicate>
    std::vector<T> find_if(Predicate pred);
};

Работа с базой данных
Структура базы данных
-- Основные таблицы
CREATE TABLE users (
    user_id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(20) NOT NULL CHECK (role IN ('customer', 'manager', 'admin')),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE products (
    product_id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    price DECIMAL(10,2) NOT NULL,
    stock_quantity INTEGER NOT NULL,
    category VARCHAR(50)
);

CREATE TABLE orders (
    order_id SERIAL PRIMARY KEY,
    customer_id INTEGER REFERENCES users(user_id),
    status VARCHAR(20) DEFAULT 'pending',
    total_amount DECIMAL(10,2) DEFAULT 0.00,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Таблицы для аудита
CREATE TABLE order_status_history (
    history_id SERIAL PRIMARY KEY,
    order_id INTEGER REFERENCES orders(order_id),
    old_status VARCHAR(20),
    new_status VARCHAR(20),
    changed_by INTEGER REFERENCES users(user_id),
    changed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE audit_log (
    audit_id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(user_id),
    action_type VARCHAR(50) NOT NULL,
    table_name VARCHAR(50),
    record_id INTEGER,
    old_values JSONB,
    new_values JSONB,
    ip_address INET,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);





Хранимые процедуры и функции
-- Функция для автоматического обновления updated_at
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

-- Триггер для orders
CREATE TRIGGER update_orders_updated_at 
BEFORE UPDATE ON orders 
FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

-- Функция логирования изменений статуса
CREATE OR REPLACE FUNCTION log_order_status_change()
RETURNS TRIGGER AS $$
BEGIN
    IF OLD.status != NEW.status THEN
        INSERT INTO order_status_history 
        (order_id, old_status, new_status, changed_by)
        VALUES (NEW.order_id, OLD.status, NEW.status, 
               current_setting('app.user_id')::INTEGER);
    END IF;
    RETURN NEW;
END;
$$ language 'plpgsql';
Механизм транзакций и отката

cpp
bool OrderManager::processOrder(int order_id) {
    try {
        pqxx::work txn(*db_connection);
        
        // Проверка доступности товаров
        auto items = getOrderItems(order_id, txn);
        for (const auto& item : items) {
            checkStockAvailability(item.product_id, item.quantity, txn);
        }
        
        // Резервирование товаров
        for (const auto& item : items) {
            reserveStock(item.product_id, item.quantity, txn);
        }
        
        // Обновление статуса заказа
        updateOrderStatus(order_id, "processing", txn);
        
        // Создание записи аудита
        logAudit("order_process", "orders", order_id, txn);
        
        txn.commit();  // Все изменения фиксируются
        return true;
        
    } catch (const std::exception& e) {
        // Автоматический откат при исключении
        std::cerr << "Ошибка обработки заказа: " << e.what() << std::endl;
        return false;
    }
}


Умные указатели и STL
Использование std::unique_ptr и std::shared_ptr
std::unique_ptr (исключительное владение):
cpp
class DatabaseManager {
private:
    std::unique_ptr<DatabaseConnection<User>> user_db;
    std::unique_ptr<DatabaseConnection<Order>> order_db;
    
public:
    DatabaseManager() 
        : user_db(std::make_unique<DatabaseConnection<User>>()),
          order_db(std::make_unique<DatabaseConnection<Order>>()) {}
};
std::shared_ptr (разделяемое владение):

cpp
class ShoppingCart {
private:
    std::shared_ptr<Customer> customer;
    std::vector<std::shared_ptr<Product>> products;
    
public:
    void addProduct(std::shared_ptr<Product> product) {
        products.push_back(product);
    }
};
Примеры использования STL алгоритмов
std::find_if - поиск элемента по условию:
cpp
auto it = std::find_if(orders.begin(), orders.end(),
    [order_id](const Order& order) {
        return order.getId() == order_id;
    });
std::copy_if - фильтрация элементов:
cpp
std::vector<Order> pending_orders;
std::copy_if(all_orders.begin(), all_orders.end(),
             std::back_inserter(pending_orders),
             [](const Order& order) {
                 return order.getStatus() == "pending";
             });
std::accumulate - агрегация данных:
cpp
double total_revenue = std::accumulate(orders.begin(), orders.end(), 0.0,
    [](double sum, const Order& order) {
        return sum + order.getTotalAmount();
    });
Лямбда-выражения:
cpp
// Сортировка заказов по дате
std::sort(orders.begin(), orders.end(),
    [](const Order& a, const Order& b) {
        return a.getCreatedAt() > b.getCreatedAt();
    });

// Фильтрация дорогих заказов
auto expensive_orders = std::count_if(orders.begin(), orders.end(),
    [threshold = 10000.0](const Order& order) {
        return order.getTotalAmount() > threshold;
    });


Логика ролей и прав доступа
ВОЗМОЖНОСТИ РОЛЕЙ
Администратор:
- Полный доступ ко всем функциям системы
- Управление продуктами (добавление, обновление, удаление)
- Просмотр и изменение любых заказов
- Доступ к журналу аудита и истории изменений
- Генерация отчетов

Менеджер:
- Утверждение заказов от покупателей
- Обновление информации о товарах на складе
- Изменение статусов заказов в рамках полномочий
- Доступ к истории утвержденных заказов

Покупатель:
- Создание новых заказов
- Добавление/удаление товаров в заказе
- Просмотр статусов и списка своих заказов
- Оплата заказов через различные методы
- Оформление возвратов (при соблюдении условий)


Реализация проверки прав доступа
cpp
class AccessController {
public:
    bool checkPermission(UserRole role, const std::string& action) {
        static const std::map<UserRole, std::set<std::string>> permissions = {
            {UserRole::CUSTOMER, {"view_products", "create_order", "view_own_orders"}},
            {UserRole::MANAGER, {"view_products", "view_all_orders", 
                                "update_order_status", "manage_products"}},
            {UserRole::ADMIN, {"view_products", "view_all_orders", 
                              "update_order_status", "manage_products",
                              "manage_users", "view_audit_log"}}
        };
        
        auto it = permissions.find(role);
        if (it != permissions.end()) {
            return it->second.find(action) != it->second.end();
        }
        return false;
    }
};
Ограничения доступа к истории заказов
cpp
std::vector<Order> OrderManager::getOrderHistory(int user_id, UserRole role) {
    if (role == UserRole::CUSTOMER) {
        // Покупатель видит только свои заказы
        return db_connection->executeQuery<std::vector<Order>>(
            "SELECT * FROM orders WHERE customer_id = $1 ORDER BY created_at DESC",
            user_id
        );
    } else if (role == UserRole::MANAGER || role == UserRole::ADMIN) {
        // Менеджер и администратор видят все заказы
        return db_connection->executeQuery<std::vector<Order>>(
            "SELECT * FROM orders ORDER BY created_at DESC"
        );
    }
    return {};
}

Аудит и история изменений
Таблицы аудита
order_status_history - история изменений статусов заказов:
sql
CREATE TABLE order_status_history (
    history_id SERIAL PRIMARY KEY,
    order_id INTEGER NOT NULL REFERENCES orders(order_id),
    old_status VARCHAR(20),
    new_status VARCHAR(20) NOT NULL,
    changed_by INTEGER REFERENCES users(user_id),
    changed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    reason TEXT
);
audit_log - журнал аудита всех действий:

sql
CREATE TABLE audit_log (
    audit_id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(user_id),
    action_type VARCHAR(50) NOT NULL,
    table_name VARCHAR(50),
    record_id INTEGER,
    old_values JSONB,
    new_values JSONB,
    ip_address INET,
    user_agent TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
Механизм автоматического логирования

cpp
class AuditLogger {
private:
    std::shared_ptr<DatabaseConnection<AuditLog>> db;
    
public:
    void logAction(int user_id, const std::string& action, 
                   const std::string& table_name = "",
                   int record_id = 0,
                   const std::string& old_values = "",
                   const std::string& new_values = "") {
        
        AuditLog log;
        log.user_id = user_id;
        log.action_type = action;
        log.table_name = table_name;
        log.record_id = record_id;
        log.old_values = old_values;
        log.new_values = new_values;
        log.ip_address = getClientIP();
        log.user_agent = getUserAgent();
        
        db->create(log);
    }
};
Примеры записей аудита
json
{
  "audit_id": 1,
  "user_id": 42,
  "action_type": "order_status_update",
  "table_name": "orders",
  "record_id": 123,
  "old_values": {"status": "pending"},
  "new_values": {"status": "processing"},
  "ip_address": "192.168.1.100",
  "created_at": "2024-01-15 10:30:00"
}

{
  "audit_id": 2,
  "user_id": 1,
  "action_type": "user_create",
  "table_name": "users",
  "record_id": 43,
  "old_values": null,
  "new_values": {"username": "new_user", "role": "customer"},
  "ip_address": "192.168.1.1",
  "created_at": "2024-01-15 11:15:00"
}





Отчёт в формате CSV
ОПИСАНИЕ ОТЧЕТА

Отчёт содержит:
Основную информацию о заказе
Историю изменений статуса
Аудиторские записи по заказу
Статистику изменений


SQL-запрос для формирования отчёта

sql
WITH order_audit_stats AS (
    SELECT 
        o.order_id,
        COUNT(DISTINCT osh.history_id) as status_changes_count,
        MAX(osh.changed_at) as last_status_change,
        COUNT(DISTINCT al.audit_id) as total_audit_records,
        MAX(al.created_at) as last_audit_time,
        STRING_AGG(DISTINCT al.action_type, ', ' ORDER BY al.action_type) as audit_actions
    FROM orders o
    LEFT JOIN order_status_history osh ON o.order_id = osh.order_id
    LEFT JOIN audit_log al ON o.order_id = al.record_id AND al.table_name = 'orders'
    GROUP BY o.order_id
)
SELECT 
    o.order_id as "ID заказа",
    u.username as "Покупатель",
    o.status as "Статус заказа",
    o.total_amount as "Сумма заказа",
    TO_CHAR(o.created_at, 'YYYY-MM-DD HH24:MI:SS') as "Дата заказа",
    COALESCE(TO_CHAR(oas.last_status_change, 'YYYY-MM-DD HH24:MI:SS'), 'Нет данных') as "Последнее изменение статуса",
    oas.status_changes_count as "Кол-во изменений статуса",
    COALESCE(
        (SELECT action_type FROM audit_log 
         WHERE record_id = o.order_id AND table_name = 'orders'
         ORDER BY created_at DESC LIMIT 1),
        'Нет данных'
    ) as "Последняя операция аудита",
    COALESCE(TO_CHAR(oas.last_audit_time, 'YYYY-MM-DD HH24:MI:SS'), 'Нет данных') as "Время последнего аудита",
    oas.total_audit_records as "Всего записей аудита"
FROM orders o
JOIN users u ON o.customer_id = u.user_id
LEFT JOIN order_audit_stats oas ON o.order_id = oas.order_id
WHERE o.created_at >= CURRENT_DATE - INTERVAL '30 days'
ORDER BY o.order_id DESC;


ПРИМЕР СОДЕРЖИМОГО CSV-файла
ID заказа;Покупатель;Статус заказа;Сумма заказа;Дата заказа;Последнее изменение статуса;Кол-во изменений статуса;Последняя операция аудита;Время последнего аудита;Всего записей аудита
4;customer_john;processing;45000.00;2024-01-09 13:45:12;2024-01-09 13:49:00;1;approve;2024-01-09 13:49:00;3
3;customer_alice;processing;3004.00;2024-01-09 12:52:29;Нет данных;0;payment;2024-01-09 12:53:25;4
2;customer_bob;canceled;35000.00;2024-01-09 12:50:16;2024-01-09 15:00:36;1;update;2024-01-09 15:00:36;4
1;customer_eve;processing;0.00;2024-01-09 12:47:59;Нет данных;0;create;2024-01-09 12:47:59;1



Сборка и запуск проекта
Системные требования:
Компилятор C++ с поддержкой C++17 
CMake 3.15
PostgreSQL 14 
libpqxx 7.7 

Установка зависимостей на macOS:
brew install postgresql@14
brew install libpqxx
brew install cmake

СБОРКА ПРОЕКТА

Сборка через CMake:

bash
git clone <репозиторий>
cd online-store

mkdir build && cd build

cmake ..

сборка проекта
make -j$(nproc)

bash
создание бд и пользователч
sudo -u postgres psql -c "CREATE DATABASE online_store;"
sudo -u postgres psql -c "CREATE USER store_user WITH PASSWORD 'secure_password';"
sudo -u postgres psql -c "GRANT ALL PRIVILEGES ON DATABASE online_store TO store_user;"


ПРИМЕР РАБОТЫ ПРОГРАММЫ

=== ИНТЕРНЕТ-МАГАЗИН ===
Подключено к базе данных
Выберите роль:

1. Администратор
2. Менеджер
3. Покупатель
4. Выход
   Выбор: 1
   Добро пожаловать, Администратор!

=== Меню администратора ===

1. Добавить новый продукт
2. Обновить информацию о продукте
3. Удалить продукт
4. Просмотр всех заказов
5. Просмотр деталей заказа
6. Изменить статус заказа
7. Просмотр истории статусов заказа
8. Просмотр журнала аудита
9. Сформировать отчёт (CSV)
10. Выход
    Выбор: 4

=== Просмотр всех заказов ===
ID Покупатель Статус Сумма Дата заказа Товаров

---

4 Покупатель processing 45000.00 2026-01-09 2
3 Покупатель processing 3004.00 2026-01-09 2
2 Покупатель canceled 35000.00 2026-01-09 1
1 Покупатель processing 0.00 2026-01-09 0

ПРИМЕРЫ РАБОТЫ МЕНЮ ДЛЯ РАЗНЫХ РОЛЕЙ

- Меню менеджера:

=== Меню менеджера ===

1. Просмотр заказов в ожидании утверждения
2. Утвердить заказ
3. Обновить количество товара на складе
4. Просмотр деталей заказа
5. Изменить статус заказа (в рамках полномочий)
6. Просмотр истории утверждённых заказов
7. Просмотр истории статусов заказов
8. Выход

- Меню покупателя:

=== Меню покупателя ===

1. Создать новый заказ
2. Добавить товар в заказ
3. Удалить товар из заказа
4. Просмотр моих заказов
5. Просмотр статуса заказа
6. Оплатить заказ
7. Оформить возврат заказа
8. Просмотр истории статусов заказа
9. Выход

Примеры логов и истории изменений заказов

Логи приложения:

text
[2024-01-15 10:30:15] INFO: Пользователь 'admin' вошел в систему
[2024-01-15 10:31:22] INFO: Заказ #123 создан пользователем 'customer_john'
[2024-01-15 10:32:45] INFO: Статус заказа #123 изменен с 'pending' на 'processing'
[2024-01-15 10:33:10] INFO: Товар 'Ноутбук Dell XPS' добавлен в заказ #123
[2024-01-15 10:35:00] INFO: Отчет 'history_changes.csv' успешно экспортирован
История изменений заказа:

text
Заказ #123 - История изменений:
┌─────────────────┬─────────────┬─────────────┬─────────────┬─────────────────────┐
│ Изменение       │ Старый      │ Новый       │ Изменил     │ Время изменения     │
├─────────────────┼─────────────┼─────────────┼─────────────┼─────────────────────┤
│ Статус          │ pending     │ processing  │ manager_1   │ 2024-01-15 10:32:45 │
│ Статус          │ processing  │ shipped     │ manager_2   │ 2024-01-15 14:15:30 │
│ Статус          │ shipped     │ delivered   │ system      │ 2024-01-16 11:20:15 │
└─────────────────┴─────────────┴─────────────┴─────────────┴─────────────────────┘
Журнал аудита:

text
┌─────────┬─────────────┬──────────────────┬──────────┬─────────────────────┐
│ ID      │ Пользователь│ Действие         │ Объект   │ Время               │
├─────────┼─────────────┼──────────────────┼──────────┼─────────────────────┤
│ 1001    │ admin       │ user_create      │ users#42 │ 2024-01-15 09:00:00 │
│ 1002    │ manager_1   │ order_update     │ orders#123│ 2024-01-15 10:32:45│
│ 1003    │ customer_john│ order_create    │ orders#124│ 2024-01-15 11:15:30│
│ 1004    │ system      │ stock_update     │ products#5│ 2024-01-15 12:00:00│
└─────────┴─────────────┴──────────────────┴──────────┴─────────────────────┘

СТРУКТУРА ПРОЕКТА
OnlineStore/
├── CMakeLists.txt # Файл конфигурации сборки
├── README.md # Основная документация
├── include/
│   ├── DatabaseConnection.h
│   ├── User.h
│   ├── Order.h
│   └── Payment.h
├── src/
│   ├── main.cpp
│   ├── DatabaseConnection.cpp
│   ├── User.cpp
│   ├── Order.cpp
│   └── Payment.cpp
├── sql/
│   └── database_setup.sql
└── reports/ 
└── audit_report.csv # Пример CSV-отчёта
