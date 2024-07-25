
-- create table
CREATE TABLE IF NOT EXISTS test_table (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    value INT
);

-- create insert stored procedure
DELIMITER //

CREATE PROCEDURE InsertTestData(IN p_name VARCHAR(255), IN p_value INT)
BEGIN
    INSERT INTO test_table (name, value) VALUES (p_name, p_value);
END //

DELIMITER ;

-- create select stored procedure
DELIMITER //

CREATE PROCEDURE SelectTestData(IN p_name VARCHAR(255), OUT p_value INT)
BEGIN
    SELECT value INTO p_value FROM test_table WHERE name = p_name;
END //

DELIMITER ;

-- create select stored procedure2
DELIMITER //

CREATE PROCEDURE SelectTestData2(IN p_value INT, OUT p_name VARCHAR(255))
BEGIN
    SELECT name INTO p_name FROM test_table WHERE value = p_value;
END //

DELIMITER ;
