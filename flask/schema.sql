CREATE TABLE IF NOT EXISTS test(
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    message TEXT,
    created_at TIMESTAMP NOT NULL default current_timestamp
);
