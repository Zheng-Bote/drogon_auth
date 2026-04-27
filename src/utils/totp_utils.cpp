/**
 * SPDX-FileComment: Time-based One-Time Password (TOTP) Utilities
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file totp_utils.cpp
 * @brief Time-based One-Time Password (TOTP) Utilities
 * @version 0.16.0
 * @date 2026-04-27
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license Apache-2.0
 */

#include "utils/totp_utils.hpp"
#include <chrono>
#include <cstring>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <random>
#include <sstream>

static const char *B32_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

namespace drogon_auth {
namespace utils {

std::string TotpUtils::generateSecret() {
  std::random_device rd;
  std::uniform_int_distribution<int> dist(0, 31);

  std::string secret;
  for (int i = 0; i < 32; ++i) {
    secret += B32_CHARS[dist(rd)];
  }
  return secret;
}

std::string TotpUtils::getProvisioningUri(const std::string &userEmail,
                                          const std::string &secret,
                                          const std::string &issuer) {
  std::ostringstream ss;
  ss << "otpauth://totp/"
     << issuer << ":" << userEmail
     << "?secret=" << secret
     << "&issuer=" << issuer
     << "&algorithm=SHA1&digits=6&period=30";
  return ss.str();
}

int64_t TotpUtils::getCurrentTimeStep() {
  auto now = std::chrono::system_clock::now();
  auto epoch = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::seconds>(epoch).count() / 30;
}

std::vector<uint8_t> TotpUtils::base32Decode(const std::string &secret) {
  std::vector<uint8_t> result;
  unsigned int buffer = 0;
  int bitsLeft = 0;

  for (char c : secret) {
    char upper = (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c;
    const char *p = std::strchr(B32_CHARS, upper);
    if (!p)
      continue;

    buffer = (buffer << 5) | (p - B32_CHARS);
    bitsLeft += 5;

    if (bitsLeft >= 8) {
      result.push_back((buffer >> (bitsLeft - 8)) & 0xFF);
      bitsLeft -= 8;
    }
  }
  return result;
}

std::string TotpUtils::generateCodeForStep(const std::vector<uint8_t> &keyBytes,
                                           int64_t timeStep) {
  uint8_t timeData[8];
  int64_t ts = timeStep;
  for (int i = 7; i >= 0; --i) {
    timeData[i] = ts & 0xFF;
    ts >>= 8;
  }

  unsigned int len = 0;
  unsigned char *hash = HMAC(EVP_sha1(), keyBytes.data(), keyBytes.size(),
                             timeData, 8, nullptr, &len);

  if (!hash)
    return "";

  int offset = hash[len - 1] & 0x0F;
  int binary = ((hash[offset] & 0x7F) << 24) |
               ((hash[offset + 1] & 0xFF) << 16) |
               ((hash[offset + 2] & 0xFF) << 8) | (hash[offset + 3] & 0xFF);

  int otp = binary % 1000000;

  std::ostringstream ss;
  ss << std::setw(6) << std::setfill('0') << otp;
  return ss.str();
}

bool TotpUtils::validateCode(const std::string &secret, const std::string &code) {
  if (secret.empty() || code.length() != 6)
    return false;

  std::vector<uint8_t> keyBytes = base32Decode(secret);
  int64_t currentStep = getCurrentTimeStep();

  for (int i = -1; i <= 1; ++i) {
    if (generateCodeForStep(keyBytes, currentStep + i) == code) {
      return true;
    }
  }
  return false;
}

} // namespace utils
} // namespace drogon_auth