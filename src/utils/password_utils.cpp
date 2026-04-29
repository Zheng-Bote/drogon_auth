/**
 * SPDX-FileComment: Password Hashing Utilities
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file password_utils.cpp
 * @brief Password Hashing Utilities
 * @version 0.16.0
 * @date 2026-04-27
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license Apache-2.0
 */

#include "utils/password_utils.hpp"
#include "utils/config_utils.hpp"
#include <sodium.h>
#include <random>
#include <vector>
#include <stdexcept>

namespace drogon_auth {
namespace utils {

std::string PasswordUtils::hashPassword(const std::string &plainText) {
  if (sodium_init() < 0) {
    return "";
  }

  // Map old Argon2 config to libsodium parameters
  // crypto_pwhash_OPSLIMIT_INTERACTIVE is 2
  // crypto_pwhash_MEMLIMIT_INTERACTIVE is 67108864 (64MB)
  unsigned long long opslimit = static_cast<unsigned long long>(
      ConfigUtil::get_int("ARGON2_ITERATIONS", crypto_pwhash_OPSLIMIT_INTERACTIVE));
  
  // ARGON2_MEMORY is usually in KB, libsodium expects bytes
  int mem_kb = ConfigUtil::get_int("ARGON2_MEMORY", 0);
  size_t memlimit;
  if (mem_kb > 0) {
      memlimit = static_cast<size_t>(mem_kb) * 1024;
  } else {
      memlimit = crypto_pwhash_MEMLIMIT_INTERACTIVE;
  }

  char hashed_password[crypto_pwhash_STRBYTES];

  if (crypto_pwhash_str(
          hashed_password, plainText.c_str(), plainText.length(),
          opslimit, memlimit) != 0) {
    return "";
  }

  return std::string(hashed_password);
}

bool PasswordUtils::verifyPassword(const std::string &plainText,
                                    const std::string &encodedHash) {
  if (encodedHash.empty())
    return false;

  if (sodium_init() < 0) {
    return false;
  }

  return crypto_pwhash_str_verify(encodedHash.c_str(), plainText.c_str(),
                                  plainText.length()) == 0;
}

std::string PasswordUtils::generateRandomPassword(int length) {
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const size_t max_index = sizeof(charset) - 1;

  std::random_device rd;
  std::mt19937 generator(rd());
  std::uniform_int_distribution<size_t> distribution(0, max_index - 1);

  std::string password;
  password.reserve(static_cast<size_t>(length));
  for (int i = 0; i < length; ++i) {
    password += charset[distribution(generator)];
  }
  return password;
}

} // namespace utils
} // namespace drogon_auth