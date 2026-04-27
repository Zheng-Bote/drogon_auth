/**
 * SPDX-FileComment: TOTP Utilities (Google Authenticator compatible)
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file totp_utils.hpp
 * @brief TOTP Utilities (Google Authenticator compatible)
 * @version 0.16.0
 * @date 2026-04-27
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license Apache-2.0
 */

#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace drogon_auth {
namespace utils {

class TotpUtils {
public:
  static std::string generateSecret();

  static std::string getProvisioningUri(const std::string &userEmail,
                                        const std::string &secret,
                                        const std::string &issuer = "DrogonAuth");

  static bool validateCode(const std::string &secret, const std::string &code);

private:
  static std::vector<uint8_t> base32Decode(const std::string &secret);
  static int64_t getCurrentTimeStep();
  static std::string generateCodeForStep(const std::vector<uint8_t> &keyBytes,
                                         int64_t timeStep);
};

} // namespace utils
} // namespace drogon_auth