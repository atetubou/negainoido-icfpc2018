CREATE TABLE IF NOT EXISTS test(
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    message TEXT,
    created_at TIMESTAMP NOT NULL default current_timestamp
);

CREATE TABLE IF NOT EXISTS problems(
    name VARCHAR(20) NOT NULL PRIMARY KEY,
    filepath TEXT,
    created_at TIMESTAMP NOT NULL default current_timestamp
);

CREATE TABLE IF NOT EXISTS solutions(
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    problem_id VARCHAR(20) NOT NULL,
    solver_id VARCHAR(100) NOT NULL,
    score BIGINT,
    comment TEXT,
    created_at TIMESTAMP NOT NULL default current_timestamp
);

