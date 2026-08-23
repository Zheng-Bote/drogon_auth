/**
 * SPDX-FileComment: RBAC Service
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <drogon/utils/coroutine.h>

namespace drogon_auth::rbac {

class RbacService {
public:
    static RbacService& instance();
    
    drogon::Task<bool> has_permission(const std::string& user_id, const std::string& permission);
    drogon::Task<bool> has_role(const std::string& user_id, const std::string& role);
    
    void invalidate_cache(const std::string& user_id);

private:
    RbacService() = default;
    
    std::mutex mutex_;
    std::unordered_map<std::string, std::vector<std::string>> user_permissions_cache_;
    std::unordered_map<std::string, std::vector<std::string>> user_roles_cache_;
};

} // namespace drogon_auth::rbac
