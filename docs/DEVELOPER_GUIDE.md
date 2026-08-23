# Drogon Auth - Developer Guide

## 1. C++23 Concepts & Patterns

This project heavily utilizes modern C++23 features to maximize performance and safety.

### 1.1 Coroutines
Drogon controllers and database repositories return `drogon::Task<T>`.
- **Rule**: Never block a thread. Always use `co_await` for database operations (`execSqlCoro`).
- **Middleware**: To run async database queries inside middleware, wrap the logic in `drogon::async_run([]() mutable -> drogon::Task<void> { ... })`.

### 1.2 Error Handling (`std::expected`)
We avoid exceptions in the hot-path (e.g., parsing logic, cryptographic validation). Instead, use `std::optional` or `std::expected` to gracefully return errors without stack-unwinding overhead.

### 1.3 `std::print` and Formatting
Use `<print>` (`std::print` / `std::println`) for direct console output. For actual application logs, use Drogon's `LOG_DEBUG`, `LOG_INFO`, and `LOG_ERROR` macros.

## 2. Code Structure

- `src/db/`: Contains all SQL queries. Controllers must never contain raw SQL.
- `src/utils/`: Pure, stateless utility functions (e.g., password hashing, UUID generation).
- `include/`: Header files corresponding to the `.cpp` files in `src/`.

## 3. Adding a New Endpoint

1. **Header (`include/my_ctrl.hpp`)**:
   Declare your controller class inheriting from `drogon::HttpController`. Map the paths using `METHOD_LIST_BEGIN`.
2. **Source (`src/my_ctrl.cpp`)**:
   Implement the handler returning `drogon::Task<drogon::HttpResponsePtr>`.
3. **Database**:
   If you need data, add an asynchronous static method to a repository in `src/db/`.
4. **Registration**:
   Drogon automatically discovers controllers via C++ reflection at startup. No manual registration is needed!
