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
#include "argon2.h"
#include <cstring>
#include <random>
#include <vector>

const uint32_t T_COST = 3;
const uint32_t M_COST = 65536;
const uint32_t PARALLELISM = 4;
const uint32_t SALT_LEN = 16;
const uint32_t HASH_LEN = 32;

namespace drogon_auth {
namespace utils {

std::string PasswordUtils::hashPassword(const std::string &plainText) {
  uint8_t salt[SALT_LEN];
  std::random_device rd;
  std::uniform_int_distribution<uint8_t> dist(0, 255);
  for (uint32_t i = 0; i < SALT_LEN; ++i) {
    salt[i] = dist(rd);
  }

  size_t encodedLen = argon2_encodedlen(T_COST, M_COST, PARALLELISM, SALT_LEN,
                                        HASH_LEN, Argon2_id);
  std::vector<char> encoded(encodedLen);

  int result = argon2id_hash_encoded(
      T_COST, M_COST, PARALLELISM, plainText.data(), plainText.size(), salt,
      SALT_LEN, HASH_LEN, encoded.data(), encodedLen);

  if (result != ARGON2_OK) {
    return "";
  }

  return std::string(encoded.data());
}

bool PasswordUtils::verifyPassword(const std::string &plainText,
                                    const std::string &encodedHash) {
  if (encodedHash.empty())
    return false;

  int result = argon2id_verify(encodedHash.c_str(), plainText.data(), plainText.size());
  return result == ARGON2_OK;
}

std::string PasswordUtils::generateRandomPassword(int length) {
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const size_t max_index = sizeof(charset) - 1;

  std::random_device rd;
  std::mt19937 generator(rd());
  std::uniform_int_distribution<size_t> distribution(0, max_index - 1);

  std::string password;
  password.reserve(length);
  for (int i = 0; i < length; ++i) {
    password += charset[distribution(generator)];
  }
  return password;
}

} // namespace utils
} // namespace drogon_auth