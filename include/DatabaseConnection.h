// include/DatabaseConnection.h
#ifndef DATABASECONNECTION_H
#define DATABASECONNECTION_H

#include <pqxx/pqxx>      // Библиотека для PostgreSQL
#include <memory>         // Для умных указателей
#include <vector>         // Для контейнеров
#include <string>         // Для строк
#include <iostream>       // Для вывода
#include <stdexcept>      // Для исключений

// ШАБЛОННЫЙ КЛАСС DatabaseConnection<T>
template<typename T>
class DatabaseConnection {
private:
    //  УМНЫЕ УКАЗАТЕЛИ
    std::unique_ptr<pqxx::connection> conn;          // unique_ptr - единоличное владение
    std::unique_ptr<pqxx::work> currentTransaction;  // Текущая транзакция

public:
    //КОНСТРУКТОР
    explicit DatabaseConnection(const T& connectionString) {
        try {
            // Создаем подключение с помощью unique_ptr
            conn = std::make_unique<pqxx::connection>(connectionString);

            if (conn->is_open()) {
                std::cout << "Подключено к БД: " << conn->dbname() << std::endl;
            } else {
                throw std::runtime_error("Не удалось открыть соединение");
            }
        } catch (const std::exception& e) {
            throw std::runtime_error("Ошибка подключения: " + std::string(e.what()));
        }
    }

    // executeQuery
    std::vector<std::vector<std::string>> executeQuery(const std::string& sql) {
        std::vector<std::vector<std::string>> results;

        try {
            if (!conn->is_open()) {
                throw std::runtime_error("Соединение с БД закрыто");
            }

            // Используем nontransaction для SELECT
            pqxx::nontransaction ntx(*conn);
            pqxx::result res = ntx.exec(sql);

            // Преобразуем результат в вектор
            for (const auto& row : res) {
                std::vector<std::string> rowData;
                for (const auto& field : row) {
                    rowData.push_back(field.c_str());
                }
                results.push_back(rowData);
            }

        } catch (const std::exception& e) {
            std::cerr << "Ошибка запроса: " << e.what() << std::endl;
            std::cerr << "SQL: " << sql << std::endl;
        }

        return results;
    }

    //  executeNonQuery
    bool executeNonQuery(const std::string& sql) {
        try {
            if (!conn->is_open()) {
                throw std::runtime_error("Соединение с БД закрыто");
            }

            pqxx::work w(*conn);
            w.exec(sql);
            w.commit();
            return true;

        } catch (const std::exception& e) {
            std::cerr << "❌ Ошибка выполнения: " << e.what() << std::endl;
            return false;
        }
    }

    // ТРАНЗАКЦИИ
    // beginTransaction
    void beginTransaction() {
        if (!currentTransaction) {
            currentTransaction = std::make_unique<pqxx::work>(*conn);
            std::cout << "🔄 Транзакция начата" << std::endl;
        }
    }

    // commitTransaction
    bool commitTransaction() {
        if (currentTransaction) {
            try {
                currentTransaction->commit();
                currentTransaction.reset();
                std::cout << "Транзакция завершена" << std::endl;
                return true;
            } catch (const std::exception& e) {
                std::cerr << "Ошибка коммита: " << e.what() << std::endl;
                return false;
            }
        }
        return false;
    }

    // rollbackTransaction
    bool rollbackTransaction() {
        if (currentTransaction) {
            try {
                currentTransaction->abort();
                currentTransaction.reset();
                std::cout << "↩Транзакция откатана" << std::endl;
                return true;
            } catch (const std::exception& e) {
                std::cerr << "Ошибка отката: " << e.what() << std::endl;
                return false;
            }
        }
        return false;
    }

    // createFunction
    bool createFunction(const std::string& functionSQL) {
        return executeNonQuery(functionSQL);
    }

    // createTrigger
    bool createTrigger(const std::string& triggerSQL) {
        return executeNonQuery(triggerSQL);
    }

    // getTransactionStatus
    std::string getTransactionStatus() const {
        if (currentTransaction) {
            return "Транзакция активна";
        }
        return "Нет активной транзакции";
    }

    //  ДЕСТРУКТОР
    ~DatabaseConnection() {
        // Автоматический откат при разрушении объекта
        if (currentTransaction) {
            rollbackTransaction();
        }

        // Закрытие соединения
        if (conn && conn->is_open()) {
            conn->close();
            std::cout << "🔌 Соединение закрыто" << std::endl;
        }
    }

    // Дополнительный метод для проверки подключения
    bool isConnected() const {
        return conn && conn->is_open();
    }
};

#endif // DATABASECONNECTION_H