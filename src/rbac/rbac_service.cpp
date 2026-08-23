/**
 * SPDX-FileComment: RBAC Service Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#include "rbac/rbac_service.hpp"
#include "db/auth_repository.hpp"
#include <algorithm>

namespace drogon_auth::rbac {

RbacService& RbacService::instance() {
    static RbacService instance;
    return instance;
}

drogon::Task<bool> RbacService::has_permission(const std::string& user_id, const std::string& permission) {
    std::vector<std::string> perms;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (user_permissions_cache_.find(user_id) != user_permissions_cache_.end()) {
            perms = user_permissions_cache_[user_id];
        }
    }
    
    if (perms.empty()) {
        perms = co_await db::AuthRepository::get_user_permissions(user_id);
        std::lock_guard<std::mutex> lock(mutex_);
        user_permissions_cache_[user_id] = perms;
    }
    
    co_return std::find(perms.begin(), perms.end(), permission) != perms.end();
}

drogon::Task<bool> RbacService::has_role(const std::string& user_id, const std::string& role) {
    std::vector<std::string> roles;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (user_roles_cache_.find(user_id) != user_roles_cache_.end()) {
            roles = user_roles_cache_[user_id];
        }
    }
    
    if (roles.empty()) {
        roles = co_await db::AuthRepository::get_user_roles(user_id);
        std::lock_guard<std::mutex> lock(mutex_);
        user_roles_cache_[user_id] = roles;
    }
    
    co_return std::find(roles.begin(), roles.end(), role) != roles.end();
}

void RbacService::invalidate_cache(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    user_permissions_cache_.erase(user_id);
    user_roles_cache_.erase(user_id);
}

} // namespace drogon_auth::rbac
