/**
 * SPDX-FileComment: Admin Controller Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#include "admin_ctrl.hpp"
#include "auth_srv.hpp"
#include "db/admin_repository.hpp"
#include "rbac/rbac_service.hpp"
#include <drogon/utils/Utilities.h>
#include <print>

namespace drogon_auth {

drogon::Task<bool> AdminCtrl::is_admin(const drogon::HttpRequestPtr& req) {
    auto session = req->session();
    if (!session || !session->find("user_id")) co_return false;
    
    std::string user_id = session->get<std::string>("user_id");
    co_return co_await rbac::RbacService::instance().has_role(user_id, "admin");
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::list_users(drogon::HttpRequestPtr req) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    auto users = co_await db::AdminRepository::get_all_users();
    auto resp = drogon::HttpResponse::newHttpJsonResponse(users);
    co_return resp;
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::create_user(drogon::HttpRequestPtr req) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("loginname") || !json->isMember("email") || !json->isMember("password")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }

    std::string loginname = (*json)["loginname"].asString();
    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();
    bool is_active = json->isMember("is_active") ? (*json)["is_active"].asBool() : true;
    bool must_pwd_change = json->isMember("must_pwd_change") ? (*json)["must_pwd_change"].asBool() : false;

    auto hash_result = AuthSrv::hash_password(password);
    if (!hash_result) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }

    std::string user_id = drogon::utils::getUuid();
    Json::Value roles = json->isMember("roles") ? (*json)["roles"] : Json::Value(Json::arrayValue);
    
    auto created_id = co_await db::AdminRepository::create_user(user_id, loginname, email, hash_result.value(), is_active, must_pwd_change, roles);

    if (created_id) {
        Json::Value ret;
        ret["status"] = "success";
        ret["id"] = created_id.value();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;
    } else {
        Json::Value ret;
        ret["status"] = "error";
        ret["message"] = "Database error or conflict (Login name or email might already exist)";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k409Conflict);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::update_user(drogon::HttpRequestPtr req, std::string user_id) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    auto json_ptr = req->getJsonObject();
    if (!json_ptr) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }

    Json::Value update_json = *json_ptr;
    
    if (update_json.isMember("password") && !update_json["password"].asString().empty()) {
        auto hash = AuthSrv::hash_password(update_json["password"].asString());
        if (hash) {
            update_json["password_hash"] = hash.value();
        }
    }

    if (co_await db::AdminRepository::update_user(user_id, update_json)) {
        if (update_json.isMember("roles")) {
            rbac::RbacService::instance().invalidate_cache(user_id);
        }
        Json::Value ret;
        ret["status"] = "success";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;
    } else {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::delete_user(drogon::HttpRequestPtr req, std::string user_id) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    if (co_await db::AdminRepository::delete_user(user_id)) {
        Json::Value ret;
        ret["status"] = "success";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;
    } else {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::list_roles(drogon::HttpRequestPtr req) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    auto roles = co_await db::AdminRepository::get_all_roles();
    auto resp = drogon::HttpResponse::newHttpJsonResponse(roles);
    co_return resp;
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::create_role(drogon::HttpRequestPtr req) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("name")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }

    std::string name = (*json)["name"].asString();
    std::string description = json->isMember("description") ? (*json)["description"].asString() : "";

    auto created_id = co_await db::AdminRepository::create_role(name, description);
    if (created_id) {
        Json::Value ret;
        ret["status"] = "success";
        ret["id"] = created_id.value();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;
    } else {
        Json::Value ret;
        ret["status"] = "error";
        ret["message"] = "Role name might already exist or Database error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k409Conflict);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::update_role(drogon::HttpRequestPtr req, std::string role_id) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }

    std::string name = json->isMember("name") ? (*json)["name"].asString() : "";
    std::string description = json->isMember("description") ? (*json)["description"].asString() : "";

    if (co_await db::AdminRepository::update_role(role_id, name, description)) {
        Json::Value ret;
        ret["status"] = "success";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;
    } else {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::delete_role(drogon::HttpRequestPtr req, std::string role_id) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    if (co_await db::AdminRepository::delete_role(role_id)) {
        Json::Value ret;
        ret["status"] = "success";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;
    } else {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::get_audit_summary(drogon::HttpRequestPtr req) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    auto summary = co_await db::AdminRepository::get_audit_summary();
    auto resp = drogon::HttpResponse::newHttpJsonResponse(summary);
    co_return resp;
}

} // namespace drogon_auth
