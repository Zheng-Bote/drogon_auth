/**
 * SPDX-FileComment: Login Filter
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file login_filter.hpp
 * @brief Login Filter for protected routes
 * @version 0.3.0
 * @date 2026-04-27
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#pragma once

#include <drogon/HttpFilter.h>

namespace drogon {
namespace filter {

class LoginFilter : public drogon::HttpFilter<LoginFilter, false> {
public:
    LoginFilter() = default;

    void doFilter(const HttpRequestPtr& req,
                  FilterCallback&& fcb,
                  FilterChainCallback&& fccb) override;

    constexpr bool isAutoCreation() const { return false; }

private:
    bool isPublicPath(const std::string& path) const;
    bool isAuthenticated(const HttpRequestPtr& req) const;
};

} // namespace filter
} // namespace drogon