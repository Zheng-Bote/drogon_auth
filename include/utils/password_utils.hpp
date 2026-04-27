/**
 * SPDX-FileComment: Password Hashing Utilities
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file password_utils.hpp
 * @brief Password Hashing Utilities
 * @version 0.16.0
 * @date 2026-04-27
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license Apache-2.0
 */

#pragma once
#include <string>

namespace drogon_auth {
namespace utils {

class PasswordUtils {
public:
  static std::string hashPassword(const std::string &plainText);
  static bool verifyPassword(const std::string &plainText, const std::string &encodedHash);
  static std::string generateRandomPassword(int length = 12);
};

} // namespace utils
} // namespace drogon_auth