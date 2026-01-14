-- ======================================================
-- ⭐ ХРАНИМЫЕ ПРОЦЕДУРЫ (требование)
-- ======================================================

-- Процедура createOrder с транзакцией
CREATE OR REPLACE PROCEDURE createOrder(
    user_id_param INTEGER,
    product_items JSONB,
    OUT new_order_id INTEGER,
    OUT result_message TEXT
)
LANGUAGE plpgsql
AS $$
DECLARE
order_total DECIMAL(10,2) := 0;
    item RECORD;
    product_record RECORD;
    insufficient_stock BOOLEAN := FALSE;
    stock_error_message TEXT := '';
BEGIN
    -- ⭐ НАЧАЛО ТРАНЗАКЦИИ (требование)
BEGIN
        -- Проверяем существование пользователя
        IF NOT EXISTS (SELECT 1 FROM users WHERE user_id = user_id_param) THEN
            RAISE EXCEPTION 'Пользователь с ID % не найден', user_id_param;
END IF;

        -- Проверяем все товары на наличие
FOR item IN SELECT * FROM jsonb_array_elements(product_items) AS items
    LOOP
SELECT * INTO product_record
FROM products
WHERE product_id = (items->>'product_id')::INTEGER;

IF NOT FOUND THEN
                RAISE EXCEPTION 'Товар с ID % не найден', (items->>'product_id')::INTEGER;
END IF;

            -- Проверяем количество на складе
            IF product_record.stock_quantity < (items->>'quantity')::INTEGER THEN
                insufficient_stock := TRUE;
                stock_error_message := format('Недостаточно товара: %s. В наличии: %s, Заказано: %s',
                    product_record.name,
                    product_record.stock_quantity,
                    (items->>'quantity')::INTEGER);
                EXIT;
END IF;
END LOOP;

        -- ⭐ ЕСЛИ ОШИБКА - ОТМЕНА ТРАНЗАКЦИИ (требование)
        IF insufficient_stock THEN
            RAISE EXCEPTION '%', stock_error_message;
END IF;

        -- Создаем заказ
INSERT INTO orders (user_id, status, total_price)
VALUES (user_id_param, 'pending', 0)
    RETURNING order_id INTO new_order_id;

-- Добавляем элементы заказа и считаем сумму
FOR item IN SELECT * FROM jsonb_array_elements(product_items) AS items
    LOOP
SELECT * INTO product_record
FROM products
WHERE product_id = (items->>'product_id')::INTEGER;

-- Вставляем элемент заказа
INSERT INTO order_items (order_id, product_id, quantity, price)
VALUES (
           new_order_id,
           (items->>'product_id')::INTEGER,
           (items->>'quantity')::INTEGER,
           product_record.price
       );

-- Обновляем общую сумму
order_total := order_total + (product_record.price * (items->>'quantity')::INTEGER);

            -- Обновляем количество на складе
UPDATE products
SET stock_quantity = stock_quantity - (items->>'quantity')::INTEGER
WHERE product_id = (items->>'product_id')::INTEGER;
END LOOP;

        -- Обновляем общую сумму заказа
UPDATE orders
SET total_price = order_total
WHERE order_id = new_order_id;

-- Записываем в историю статусов
INSERT INTO order_status_history (order_id, old_status, new_status, changed_by)
VALUES (new_order_id, NULL, 'pending', user_id_param);

-- Записываем в аудит
INSERT INTO audit_log (entity_type, entity_id, operation, performed_by, details)
VALUES ('order', new_order_id, 'insert', user_id_param,
        format('Создан заказ на сумму: %s', order_total));

result_message := 'Заказ успешно создан';

        -- ⭐ ЗАВЕРШЕНИЕ ТРАНЗАКЦИИ
COMMIT;

EXCEPTION WHEN OTHERS THEN
        -- ⭐ ОТКАТ ТРАНЗАКЦИИ ПРИ ОШИБКЕ (требование)
        ROLLBACK;
        result_message := 'Ошибка создания заказа: ' || SQLERRM;
        new_order_id := NULL;

        -- Записываем ошибку в аудит
INSERT INTO audit_log (entity_type, operation, performed_by, details)
VALUES ('order', 'error', user_id_param,
        'Ошибка создания заказа: ' || SQLERRM);
END;
END;
$$;

-- Процедура updateOrderStatus
CREATE OR REPLACE PROCEDURE updateOrderStatus(
    order_id_param INTEGER,
    new_status_param VARCHAR,
    changed_by_param INTEGER,
    OUT result_message TEXT
)
LANGUAGE plpgsql
AS $$
DECLARE
old_status_var VARCHAR;
    current_status_var VARCHAR;
BEGIN
BEGIN
        -- Получаем текущий статус
SELECT status INTO current_status_var
FROM orders
WHERE order_id = order_id_param;

IF NOT FOUND THEN
            RAISE EXCEPTION 'Заказ не найден';
END IF;

        -- Сохраняем старый статус
        old_status_var := current_status_var;

        -- Обновляем статус
UPDATE orders
SET status = new_status_param
WHERE order_id = order_id_param;

-- ⭐ ТРИГГЕР автоматически добавит запись в order_status_history

-- Записываем в аудит
INSERT INTO audit_log (entity_type, entity_id, operation, performed_by, details)
VALUES ('order', order_id_param, 'update', changed_by_param,
        format('Статус изменен с %s на %s', old_status_var, new_status_param));

result_message := 'Статус успешно обновлен';
COMMIT;

EXCEPTION WHEN OTHERS THEN
        ROLLBACK;
        result_message := 'Ошибка обновления статуса: ' || SQLERRM;
END;
END;
$$;

-- ======================================================
-- ⭐ ФУНКЦИИ PostgreSQL (требование)
-- ======================================================

-- 1. getOrderStatus - возвращает статус заказа
CREATE OR REPLACE FUNCTION getOrderStatus(order_id_param INTEGER)
RETURNS VARCHAR AS $$
DECLARE
order_status VARCHAR;
BEGIN
SELECT status INTO order_status
FROM orders
WHERE order_id = order_id_param;

RETURN COALESCE(order_status, 'not_found');
END;
$$ LANGUAGE plpgsql;

-- 2. getUserOrderCount - количество заказов пользователя
CREATE OR REPLACE FUNCTION getUserOrderCount(user_id_param INTEGER)
RETURNS INTEGER AS $$
DECLARE
order_count INTEGER;
BEGIN
SELECT COUNT(*) INTO order_count
FROM orders
WHERE user_id = user_id_param;

RETURN COALESCE(order_count, 0);
END;
$$ LANGUAGE plpgsql;

-- 3. getTotalSpentByUser - общая сумма покупок
CREATE OR REPLACE FUNCTION getTotalSpentByUser(user_id_param INTEGER)
RETURNS DECIMAL AS $$
DECLARE
total_spent DECIMAL;
BEGIN
SELECT COALESCE(SUM(total_price), 0) INTO total_spent
FROM orders
WHERE user_id = user_id_param
  AND status IN ('completed', 'returned');

RETURN total_spent;
END;
$$ LANGUAGE plpgsql;

-- 4. canReturnOrder - проверка возможности возврата (30 дней)
CREATE OR REPLACE FUNCTION canReturnOrder(order_id_param INTEGER)
RETURNS BOOLEAN AS $$
DECLARE
order_record RECORD;
    days_passed INTEGER;
BEGIN
SELECT status, order_date INTO order_record
FROM orders
WHERE order_id = order_id_param;

IF NOT FOUND THEN
        RETURN FALSE;
END IF;

    -- Проверяем, что заказ завершен
    IF order_record.status != 'completed' THEN
        RETURN FALSE;
END IF;

    -- Проверяем, что прошло не более 30 дней
    days_passed := EXTRACT(DAY FROM (CURRENT_TIMESTAMP - order_record.order_date));

RETURN days_passed <= 30;
END;
$$ LANGUAGE plpgsql;

-- 5. getOrderStatusHistory - история статусов заказа
CREATE OR REPLACE FUNCTION getOrderStatusHistory(order_id_param INTEGER)
RETURNS TABLE(
    history_id INTEGER,
    old_status VARCHAR,
    new_status VARCHAR,
    changed_at TIMESTAMP,
    changed_by_name VARCHAR
) AS $$
BEGIN
RETURN QUERY
SELECT
    h.history_id,
    h.old_status,
    h.new_status,
    h.changed_at,
    u.name as changed_by_name
FROM order_status_history h
         LEFT JOIN users u ON h.changed_by = u.user_id
WHERE h.order_id = order_id_param
ORDER BY h.changed_at DESC;
END;
$$ LANGUAGE plpgsql;

-- 6. getAuditLogByUser - журнал аудита по пользователю
CREATE OR REPLACE FUNCTION getAuditLogByUser(user_id_param INTEGER)
RETURNS TABLE(
    log_id INTEGER,
    entity_type VARCHAR,
    entity_id INTEGER,
    operation VARCHAR,
    performed_at TIMESTAMP,
    details TEXT,
    ip_address INET
) AS $$
BEGIN
RETURN QUERY
SELECT
    a.log_id,
    a.entity_type,
    a.entity_id,
    a.operation,
    a.performed_at,
    a.details,
    a.ip_address
FROM audit_log a
WHERE a.performed_by = user_id_param
ORDER BY a.performed_at DESC
    LIMIT 100;
END;
$$ LANGUAGE plpgsql;

-- 7. generateAuditReport - генерация отчета для CSV
CREATE OR REPLACE FUNCTION generateAuditReport(start_date DATE, end_date DATE)
RETURNS TABLE(
    order_id INTEGER,
    customer_name VARCHAR,
    order_status VARCHAR,
    status_change VARCHAR,
    change_date TIMESTAMP,
    operation_type VARCHAR,
    operation_date TIMESTAMP,
    details TEXT
) AS $$
BEGIN
RETURN QUERY
SELECT
    o.order_id,
    u.name as customer_name,
    o.status as order_status,
    CONCAT(h.old_status, ' -> ', h.new_status) as status_change,
    h.changed_at as change_date,
    a.operation as operation_type,
    a.performed_at as operation_date,
    COALESCE(a.details, '') as details
FROM orders o
         JOIN users u ON o.user_id = u.user_id
         LEFT JOIN order_status_history h ON o.order_id = h.order_id
         LEFT JOIN audit_log a ON o.order_id = a.entity_id AND a.entity_type = 'order'
WHERE o.order_date BETWEEN start_date AND end_date
ORDER BY o.order_id, h.changed_at;
END;
$$ LANGUAGE plpgsql;

-- ======================================================
-- ⭐ ТРИГГЕРЫ (требование)
-- ======================================================

-- 1. Триггер для автоматического обновления order_date при изменении статуса
CREATE OR REPLACE FUNCTION update_order_date_on_status_change()
RETURNS TRIGGER AS $$
BEGIN
    IF OLD.status IS DISTINCT FROM NEW.status THEN
        NEW.order_date = CURRENT_TIMESTAMP;
END IF;
RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_update_order_date
    BEFORE UPDATE OF status ON orders
    FOR EACH ROW
    EXECUTE FUNCTION update_order_date_on_status_change();

-- 2. Триггер для обновления total_price при изменении цены продукта
CREATE OR REPLACE FUNCTION update_order_prices_on_product_change()
RETURNS TRIGGER AS $$
BEGIN
    IF OLD.price IS DISTINCT FROM NEW.price THEN
        -- Обновляем цену в элементах заказа
UPDATE order_items oi
SET price = NEW.price
    FROM orders o
WHERE oi.product_id = NEW.product_id
  AND oi.order_id = o.order_id
  AND o.status IN ('pending', 'completed');

-- Пересчитываем общую сумму заказов
UPDATE orders o
SET total_price = (
    SELECT SUM(oi.quantity * oi.price)
    FROM order_items oi
    WHERE oi.order_id = o.order_id
)
WHERE o.order_id IN (
    SELECT DISTINCT order_id
    FROM order_items
    WHERE product_id = NEW.product_id
)
  AND o.status IN ('pending', 'completed');
END IF;
RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_update_order_prices
    AFTER UPDATE OF price ON products
    FOR EACH ROW
    EXECUTE FUNCTION update_order_prices_on_product_change();

-- 3. Триггер аудита для сохранения истории статусов
CREATE OR REPLACE FUNCTION log_order_status_change()
RETURNS TRIGGER AS $$
BEGIN
    IF OLD.status IS DISTINCT FROM NEW.status THEN
        INSERT INTO order_status_history (order_id, old_status, new_status, changed_by)
        VALUES (NEW.order_id, OLD.status, NEW.status, NEW.user_id);
END IF;
RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_log_order_status_change
    AFTER UPDATE OF status ON orders
    FOR EACH ROW
    EXECUTE FUNCTION log_order_status_change();

-- 4. Триггер аудита для товаров
CREATE OR REPLACE FUNCTION audit_product_changes()
RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'INSERT' THEN
        INSERT INTO audit_log (entity_type, entity_id, operation, performed_by, details)
        VALUES ('product', NEW.product_id, 'insert', NULL,
                format('Новый товар: %s, Цена: %s', NEW.name, NEW.price));
    ELSIF TG_OP = 'UPDATE' THEN
        INSERT INTO audit_log (entity_type, entity_id, operation, performed_by, details)
        VALUES ('product', NEW.product_id, 'update', NULL,
                format('Товар обновлен. Название: %s -> %s, Цена: %s -> %s',
                       OLD.name, NEW.name, OLD.price, NEW.price));
    ELSIF TG_OP = 'DELETE' THEN
        INSERT INTO audit_log (entity_type, entity_id, operation, performed_by, details)
        VALUES ('product', OLD.product_id, 'delete', NULL,
                format('Товар удален: %s', OLD.name));
END IF;

RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_audit_products
    AFTER INSERT OR UPDATE OR DELETE ON products
    FOR EACH ROW
    EXECUTE FUNCTION audit_product_changes();

-- 5. Триггер аудита для пользователей
CREATE OR REPLACE FUNCTION audit_user_changes()
RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'DELETE' THEN
        INSERT INTO audit_log (entity_type, entity_id, operation, performed_by, details)
        VALUES ('user', OLD.user_id, 'delete', NULL,
                format('Пользователь удален: %s (%s)', OLD.name, OLD.email));
END IF;

RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_audit_users
    AFTER DELETE ON users
    FOR EACH ROW
    EXECUTE FUNCTION audit_user_changes();

-- 6. Триггер аудита для заказов
CREATE OR REPLACE FUNCTION audit_order_changes()
RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'INSERT' THEN
        INSERT INTO audit_log (entity_type, entity_id, operation, performed_by, details)
        VALUES ('order', NEW.order_id, 'insert', NEW.user_id,
                'Создан новый заказ');
    ELSIF TG_OP = 'UPDATE' THEN
        IF OLD.status = 'completed' AND NEW.status = 'returned' THEN
            INSERT INTO audit_log (entity_type, entity_id, operation, performed_by, details)
            VALUES ('order', NEW.order_id, 'update', NEW.user_id,
                    'Заказ возвращен');
        ELSIF OLD.status = 'pending' AND NEW.status = 'canceled' THEN
            INSERT INTO audit_log (entity_type, entity_id, operation, performed_by, details)
            VALUES ('order', NEW.order_id, 'update', NEW.user_id,
                    'Заказ отменен');
END IF;
END IF;

RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_audit_orders
    AFTER INSERT OR UPDATE ON orders
                        FOR EACH ROW
                        EXECUTE FUNCTION audit_order_changes();