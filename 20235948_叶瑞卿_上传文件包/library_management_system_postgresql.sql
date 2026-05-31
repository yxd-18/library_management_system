DROP TABLE IF EXISTS backup_records CASCADE;
DROP TABLE IF EXISTS operation_logs CASCADE;
DROP TABLE IF EXISTS user_settings CASCADE;
DROP TABLE IF EXISTS reservation_records CASCADE;
DROP TABLE IF EXISTS borrow_records CASCADE;
DROP TABLE IF EXISTS books CASCADE;
DROP TABLE IF EXISTS book_categories CASCADE;
DROP TABLE IF EXISTS users CASCADE;
DROP TABLE IF EXISTS roles CASCADE;

DROP TYPE IF EXISTS backup_type_enum CASCADE;
DROP TYPE IF EXISTS reservation_status_enum CASCADE;
DROP TYPE IF EXISTS borrow_status_enum CASCADE;
DROP TYPE IF EXISTS book_status_enum CASCADE;
DROP TYPE IF EXISTS user_status_enum CASCADE;
DROP TYPE IF EXISTS gender_enum CASCADE;

CREATE TYPE gender_enum AS ENUM ('男', '女', '其他');
CREATE TYPE user_status_enum AS ENUM ('正常', '禁用');
CREATE TYPE book_status_enum AS ENUM ('可借', '部分借出', '无库存', '下架', '损坏');
CREATE TYPE borrow_status_enum AS ENUM ('借阅中', '已归还', '已逾期', '已挂失');
CREATE TYPE reservation_status_enum AS ENUM ('排队中', '可取书', '已完成', '已取消');
CREATE TYPE backup_type_enum AS ENUM ('手动备份', '自动备份', '恢复');

CREATE TABLE roles (
    role_id INT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    role_name VARCHAR(50) NOT NULL UNIQUE,
    role_desc VARCHAR(255)
);

CREATE TABLE users (
    user_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    real_name VARCHAR(50) NOT NULL,
    student_no VARCHAR(30) UNIQUE,
    phone VARCHAR(20),
    email VARCHAR(100),
    gender gender_enum NOT NULL DEFAULT '其他',
    status user_status_enum NOT NULL DEFAULT '正常',
    role_id INT NOT NULL REFERENCES roles(role_id),
    register_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    last_login_time TIMESTAMP,
    remark VARCHAR(255)
);

CREATE TABLE book_categories (
    category_id INT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    category_name VARCHAR(100) NOT NULL UNIQUE,
    parent_id INT REFERENCES book_categories(category_id),
    category_desc VARCHAR(255),
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE books (
    book_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    isbn VARCHAR(20) NOT NULL UNIQUE,
    book_title VARCHAR(200) NOT NULL,
    author VARCHAR(100) NOT NULL,
    publisher VARCHAR(100),
    publish_date DATE,
    category_id INT REFERENCES book_categories(category_id),
    price NUMERIC(10, 2) NOT NULL DEFAULT 0.00,
    total_stock INT NOT NULL DEFAULT 0,
    available_stock INT NOT NULL DEFAULT 0,
    lost_count INT NOT NULL DEFAULT 0,
    location VARCHAR(100),
    book_status book_status_enum NOT NULL DEFAULT '可借',
    description TEXT,
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT chk_books_stock CHECK (
        total_stock >= 0 AND
        available_stock >= 0 AND
        available_stock <= total_stock
    )
);

CREATE TABLE borrow_records (
    record_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(user_id),
    book_id BIGINT NOT NULL REFERENCES books(book_id),
    borrow_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    due_time TIMESTAMP NOT NULL,
    return_time TIMESTAMP,
    renew_count INT NOT NULL DEFAULT 0,
    max_renew_count INT NOT NULL DEFAULT 1,
    record_status borrow_status_enum NOT NULL DEFAULT '借阅中',
    overdue_days INT NOT NULL DEFAULT 0,
    fine_amount NUMERIC(10, 2) NOT NULL DEFAULT 0.00,
    operator_id BIGINT REFERENCES users(user_id),
    remark VARCHAR(255),
    CONSTRAINT chk_borrow_renew CHECK (
        renew_count >= 0 AND renew_count <= max_renew_count
    )
);

CREATE TABLE reservation_records (
    reservation_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(user_id),
    book_id BIGINT NOT NULL REFERENCES books(book_id),
    reservation_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    ready_time TIMESTAMP,
    pickup_deadline TIMESTAMP,
    fulfill_time TIMESTAMP,
    cancel_time TIMESTAMP,
    reservation_status reservation_status_enum NOT NULL DEFAULT '排队中',
    operator_id BIGINT REFERENCES users(user_id),
    remark VARCHAR(255)
);

CREATE TABLE operation_logs (
    log_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    operator_id BIGINT NOT NULL REFERENCES users(user_id),
    operation_type VARCHAR(50) NOT NULL,
    target_table VARCHAR(50),
    target_id BIGINT,
    operation_content VARCHAR(500) NOT NULL,
    ip_address VARCHAR(50),
    operation_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE user_settings (
    setting_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    user_id BIGINT NOT NULL UNIQUE REFERENCES users(user_id),
    font_size VARCHAR(20) NOT NULL DEFAULT 'medium',
    theme VARCHAR(20) NOT NULL DEFAULT 'light',
    language_pref VARCHAR(20) NOT NULL DEFAULT 'zh-CN',
    page_size INT NOT NULL DEFAULT 10,
    enable_notification BOOLEAN NOT NULL DEFAULT TRUE,
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE backup_records (
    backup_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    backup_name VARCHAR(100) NOT NULL,
    file_path VARCHAR(255) NOT NULL,
    backup_type backup_type_enum NOT NULL,
    operator_id BIGINT NOT NULL REFERENCES users(user_id),
    backup_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    remark VARCHAR(255)
);

CREATE INDEX idx_users_student_no ON users(student_no);
CREATE INDEX idx_users_real_name ON users(real_name);
CREATE INDEX idx_books_title ON books(book_title);
CREATE INDEX idx_books_author ON books(author);
CREATE INDEX idx_books_category_id ON books(category_id);
CREATE INDEX idx_borrow_user_id ON borrow_records(user_id);
CREATE INDEX idx_borrow_book_id ON borrow_records(book_id);
CREATE INDEX idx_borrow_status ON borrow_records(record_status);
CREATE INDEX idx_borrow_due_time ON borrow_records(due_time);
CREATE INDEX idx_reservation_user_id ON reservation_records(user_id);
CREATE INDEX idx_reservation_book_status ON reservation_records(book_id, reservation_status);
CREATE INDEX idx_logs_operator_time ON operation_logs(operator_id, operation_time);

CREATE OR REPLACE FUNCTION update_books_update_time()
RETURNS TRIGGER AS $$
BEGIN
    NEW.update_time := CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_books_update_time
BEFORE UPDATE ON books
FOR EACH ROW
EXECUTE FUNCTION update_books_update_time();

INSERT INTO roles (role_name, role_desc) VALUES
('admin', '管理员'),
('reader', '普通读者');

INSERT INTO users (username, password_hash, real_name, student_no, phone, email, gender, status, role_id, remark) VALUES
('admin01', 'admin123', '系统管理员', NULL, '13800000000', 'admin01@library.com', '男', '正常', 1, '默认管理员'),
('admin02', 'admin123', '图书馆老师', NULL, '13800000001', 'admin02@library.com', '女', '正常', 1, '值班管理员'),
('reader01', 'reader123', '张三', '20230001', '13900000001', 'reader01@library.com', '男', '正常', 2, '计算机学院'),
('reader02', 'reader123', '李四', '20230002', '13900000002', 'reader02@library.com', '女', '正常', 2, '文学学院'),
('reader03', 'reader123', '王五', '20230003', '13900000003', 'reader03@library.com', '男', '正常', 2, '历史学院'),
('reader04', 'reader123', '赵六', '20230004', '13900000004', 'reader04@library.com', '女', '禁用', 2, '测试禁用用户');

INSERT INTO book_categories (category_name, parent_id, category_desc) VALUES
('计算机', NULL, '计算机类图书'),
('文学', NULL, '文学类图书'),
('历史', NULL, '历史类图书'),
('数据库', 1, '数据库与数据管理'),
('程序设计', 1, '程序设计语言与开发'),
('中国文学', 2, '中国古典与现代文学'),
('世界文学', 2, '外国文学作品'),
('中国史', 3, '中国历史与文化');

INSERT INTO books (isbn, book_title, author, publisher, publish_date, category_id, price, total_stock, available_stock, location, book_status, description) VALUES
('9787302511854', 'Java程序设计', '张三', '清华大学出版社', '2022-01-01', 5, 59.00, 10, 9, 'A-01-01', '部分借出', 'Java 入门与实战'),
('9787111128069', 'C++ Primer', 'Stanley Lippman', '机械工业出版社', '2019-06-01', 5, 88.00, 6, 5, 'A-01-02', '部分借出', '经典 C++ 教材'),
('9787121456789', 'PostgreSQL实战', '李老师', '电子工业出版社', '2023-03-15', 4, 79.00, 8, 7, 'A-01-03', '部分借出', '适合课程设计的 PostgreSQL 实战书'),
('9787020002207', '红楼梦', '曹雪芹', '人民文学出版社', '2018-05-01', 6, 39.00, 5, 4, 'B-02-03', '部分借出', '中国古典文学名著'),
('9787020008735', '三国演义', '罗贯中', '人民文学出版社', '2017-08-01', 6, 42.00, 5, 5, 'B-02-04', '可借', '中国古典四大名著之一'),
('9787532754687', '百年孤独', '加西亚·马尔克斯', '上海译文出版社', '2019-09-01', 7, 55.00, 4, 4, 'B-03-01', '可借', '拉美魔幻现实主义代表作'),
('9787101148204', '明朝那些事儿', '当年明月', '中华书局', '2020-04-01', 8, 128.00, 7, 6, 'C-01-01', '部分借出', '通俗历史读物'),
('9787505745660', '史记', '司马迁', '中国友谊出版公司', '2016-07-01', 8, 68.00, 3, 3, 'C-01-02', '可借', '中国纪传体通史'),
('9787302600008', '软件工程导论', '王道', '清华大学出版社', '2022-09-01', 1, 66.00, 5, 0, 'A-02-01', '无库存', '课程设计常用教材'),
('9787110001111', '算法设计与分析', '严蔚敏', '清华大学出版社', '2021-02-01', 1, 72.00, 4, 4, 'A-02-02', '可借', '算法基础与设计');

INSERT INTO borrow_records (user_id, book_id, borrow_time, due_time, return_time, renew_count, max_renew_count, record_status, overdue_days, operator_id, remark) VALUES
(3, 1, CURRENT_TIMESTAMP - INTERVAL '3 day', CURRENT_TIMESTAMP + INTERVAL '27 day', NULL, 0, 1, '借阅中', 0, 1, '正常借阅'),
(4, 2, CURRENT_TIMESTAMP - INTERVAL '15 day', CURRENT_TIMESTAMP + INTERVAL '15 day', NULL, 0, 1, '借阅中', 0, 1, '课程作业借阅'),
(5, 4, CURRENT_TIMESTAMP - INTERVAL '40 day', CURRENT_TIMESTAMP - INTERVAL '10 day', NULL, 0, 1, '已逾期', 10, 2, '逾期未还'),
(3, 7, CURRENT_TIMESTAMP - INTERVAL '50 day', CURRENT_TIMESTAMP - INTERVAL '20 day', CURRENT_TIMESTAMP - INTERVAL '15 day', 1, 1, '已归还', 5, 2, '已归还测试'),
(4, 9, CURRENT_TIMESTAMP - INTERVAL '12 day', CURRENT_TIMESTAMP + INTERVAL '18 day', NULL, 1, 1, '借阅中', 0, 1, '库存借空测试'),
(5, 3, CURRENT_TIMESTAMP - INTERVAL '5 day', CURRENT_TIMESTAMP + INTERVAL '25 day', NULL, 0, 1, '借阅中', 0, 2, '数据库课程借阅');

INSERT INTO reservation_records (user_id, book_id, reservation_time, ready_time, pickup_deadline, fulfill_time, cancel_time, reservation_status, operator_id, remark) VALUES
(3, 9, CURRENT_TIMESTAMP - INTERVAL '2 day', NULL, NULL, NULL, NULL, '排队中', 1, '等待教材归还'),
(5, 9, CURRENT_TIMESTAMP - INTERVAL '1 day', NULL, NULL, NULL, NULL, '排队中', 2, '课程复习预约'),
(4, 1, CURRENT_TIMESTAMP - INTERVAL '4 day', CURRENT_TIMESTAMP - INTERVAL '1 day', CURRENT_TIMESTAMP + INTERVAL '2 day', NULL, NULL, '可取书', 1, '已到馆待取');

INSERT INTO operation_logs (operator_id, operation_type, target_table, target_id, operation_content, ip_address) VALUES
(1, '初始化数据', 'users', 1, '创建默认管理员和测试读者', '127.0.0.1'),
(1, '初始化数据', 'books', 1, '创建图书基础测试数据', '127.0.0.1'),
(2, '手动修正库存', 'books', 9, '将软件工程导论库存调整为 0，用于无库存测试', '127.0.0.1'),
(2, '补充借阅数据', 'borrow_records', 3, '增加逾期记录和已归还记录测试数据', '127.0.0.1');

INSERT INTO user_settings (user_id, font_size, theme, language_pref, page_size, enable_notification) VALUES
(1, 'large', 'dark', 'zh-CN', 20, TRUE),
(2, 'medium', 'light', 'zh-CN', 15, TRUE),
(3, 'medium', 'dark', 'zh-CN', 10, TRUE),
(4, 'small', 'light', 'en-US', 12, FALSE);

INSERT INTO backup_records (backup_name, file_path, backup_type, operator_id, remark) VALUES
('daily_backup_20260420', 'D:/backup/library_20260420.sql', '自动备份', 1, '每日定时备份'),
('manual_before_demo', 'D:/backup/library_before_demo.sql', '手动备份', 2, '答辩前手动备份'),
('restore_test_20260421', 'D:/backup/library_restore_20260421.sql', '恢复', 1, '恢复演示测试');
