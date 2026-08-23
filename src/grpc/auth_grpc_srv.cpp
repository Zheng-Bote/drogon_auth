/**
 * SPDX-FileComment: gRPC Authentication Service Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */

// TODO: audit log

#include "grpc/auth_grpc_srv.hpp"
#include "utils/config_utils.hpp"
#include <drogon/drogon.h>
#include <trantor/utils/Logger.h>

namespace drogon_auth::grpc {

static bool is_authorized(::grpc::ServerContext *context) {
  std::string expected_key =
      drogon_auth::utils::ConfigUtil::get_string("GRPC_API_KEY", "");
  if (expected_key.empty())
    return true; // Security bypass if not configured, or could be strict

  auto metadata = context->client_metadata();
  auto it = metadata.find("x-api-key");
  if (it == metadata.end())
    return false;

  std::string provided_key(it->second.data(), it->second.size());
  return provided_key == expected_key;
}

#define CHECK_GRPC_AUTH()                                                      \
  if (!is_authorized(context)) {                                               \
    return ::grpc::Status(::grpc::StatusCode::UNAUTHENTICATED,                 \
                          "Invalid or missing x-api-key");                     \
  }

::grpc::Status
AuthGrpcServiceImpl::GetUserStatus(::grpc::ServerContext *context,
                                   const UserStatusRequest *request,
                                   UserStatusResponse *response) {
  CHECK_GRPC_AUTH();
  const std::string &session_id = request->session_id();
  auto db = drogon::app().getDbClient();

  try {
    // 1. Check session
    auto res =
        db->execSqlSync("SELECT user_id FROM sessions WHERE session_token = $1 "
                        "AND expires_at > CURRENT_TIMESTAMP",
                        session_id);

    if (res.empty()) {
      response->set_is_authenticated(false);
      return ::grpc::Status::OK;
    }

    std::string user_id = res[0]["user_id"].as<std::string>();
    response->set_is_authenticated(true);
    response->set_user_id(user_id);

    // 2. Check if user is active
    auto user_res =
        db->execSqlSync("SELECT is_active FROM users WHERE id = $1", user_id);
    if (!user_res.empty()) {
      response->set_is_active(user_res[0]["is_active"].as<bool>());
    } else {
      response->set_is_active(false);
    }

    // 3. Fetch roles
    auto roles_res = db->execSqlSync("SELECT r.name FROM roles r "
                                     "JOIN user_roles ur ON r.id = ur.role_id "
                                     "WHERE ur.user_id = $1",
                                     user_id);

    for (const auto &row : roles_res) {
      response->add_roles(row["name"].as<std::string>());
    }

    return ::grpc::Status::OK;

  } catch (const std::exception &e) {
    LOG_ERROR << "gRPC GetUserStatus Error: " << e.what();
    return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Database error");
  }
}

::grpc::Status
AuthGrpcServiceImpl::CheckHealth(::grpc::ServerContext *context,
                                 const HealthCheckRequest *request,
                                 HealthCheckResponse *response) {
  // Health check usually bypasses auth to allow orchestrators to check status,
  // but if specifically requested, we can add it. Leaving open for now.

  auto db = drogon::app().getDbClient();
  try {
    // DB Punch-through (Durchstich)
    db->execSqlSync("SELECT 1");

    response->set_status(HealthCheckResponse::SERVING);
    response->set_db_status("OK");
  } catch (const std::exception &e) {
    LOG_ERROR << "gRPC Health Check DB Error: " << e.what();
    response->set_status(HealthCheckResponse::NOT_SERVING);
    response->set_db_status(std::string("Error: ") + e.what());
  }
  return ::grpc::Status::OK;
}

::grpc::Status
AuthGrpcServiceImpl::GetUserProfile(::grpc::ServerContext *context,
                                    const UserRequest *request,
                                    UserProfileResponse *response) {
  CHECK_GRPC_AUTH();
  auto db = drogon::app().getDbClient();
  try {
    auto res =
        db->execSqlSync("SELECT first_name, last_name, preferred_language, "
                        "locale, timezone, attributes "
                        "FROM user_profiles WHERE user_id = $1",
                        request->user_id());

    if (res.empty()) {
      return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Profile not found");
    }

    response->set_first_name(res[0]["first_name"].as<std::string>());
    response->set_last_name(res[0]["last_name"].as<std::string>());
    response->set_preferred_language(
        res[0]["preferred_language"].as<std::string>());
    response->set_locale(res[0]["locale"].as<std::string>());
    response->set_timezone(res[0]["timezone"].as<std::string>());
    response->set_attributes_json(res[0]["attributes"].as<std::string>());

    return ::grpc::Status::OK;
  } catch (const std::exception &e) {
    LOG_ERROR << "gRPC GetUserProfile Error: " << e.what();
    return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Database error");
  }
}

::grpc::Status AuthGrpcServiceImpl::GetUserCommunications(
    ::grpc::ServerContext *context, const UserRequest *request,
    UserCommunicationsResponse *response) {
  CHECK_GRPC_AUTH();
  auto db = drogon::app().getDbClient();
  try {
    auto res = db->execSqlSync("SELECT channel, address, is_active, verified "
                               "FROM user_communications WHERE user_id = $1",
                               request->user_id());

    for (const auto &row : res) {
      auto *entry = response->add_communications();
      entry->set_channel(row["channel"].as<std::string>());
      entry->set_address(row["address"].as<std::string>());
      entry->set_is_active(row["is_active"].as<bool>());
      entry->set_verified(row["verified"].as<bool>());
    }

    return ::grpc::Status::OK;
  } catch (const std::exception &e) {
    LOG_ERROR << "gRPC GetUserCommunications Error: " << e.what();
    return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Database error");
  }
}

} // namespace drogon_auth::grpc
