/**
 * SPDX-FileComment: Main entry point
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file main.cpp
 * @brief Main entry point
 * @version 0.1.3
 * @date 2026-04-28
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#include "utils/config_utils.hpp"
#include "utils/seeder_utils.hpp"
#include "utils/seeder_utils.hpp"
#include "cache/session_cache.hpp"
#include "grpc/auth_grpc_srv.hpp"
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <string>
#include <trantor/utils/Logger.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <thread>
#include <chrono>
#include "metrics_registry.hpp"
int main(int argc, char *argv[]) {
  // 1. Determine environment
  std::string env_path = ".env";
  if (argc > 1) {
    env_path = argv[1];
  }
  drogon_auth::utils::ConfigUtil::load_env(env_path);

  // 2. Load configuration
  std::string drogon_config =
      drogon_auth::utils::ConfigUtil::get_string("DROGON_CONFIG_FILE", "");
  if (!drogon_config.empty()) {
    drogon::app().loadConfigFile(drogon_config);
  }

  // Collect variables for Logger-Callback
  int port = drogon_auth::utils::ConfigUtil::get_int("SERVER_PORT", 8848);
  int grpc_port = drogon_auth::utils::ConfigUtil::get_int("GRPC_PORT", 50051);
  std::string db_type =
      drogon_auth::utils::ConfigUtil::get_string("DB_TYPE", "postgres");

  int server_port = drogon_auth::utils::ConfigUtil::get_int("SERVER_PORT", 8080);
  std::string server_host = drogon_auth::utils::ConfigUtil::get_string("SERVER_HOST", "0.0.0.0");
  std::string ssl_cert = drogon_auth::utils::ConfigUtil::get_string("SERVER_SSL_CERT", "");
  std::string ssl_key = drogon_auth::utils::ConfigUtil::get_string("SERVER_SSL_KEY", "");

  auto& app = drogon::app();
  if (!ssl_cert.empty() && !ssl_key.empty()) {
      app.addListener(server_host, server_port, true, ssl_cert, ssl_key);
  } else {
      app.addListener(server_host, server_port);
  }
  
  app.setThreadNum(drogon_auth::utils::ConfigUtil::get_int("SERVER_THREADS", 4));

  // 3. Beginning Advice: Ensures logs end up in the FILE
  drogon::app().registerBeginningAdvice(
      [env_path, drogon_config, port, grpc_port, db_type]() {
        LOG_INFO << "--- Drogon Auth Microservice starting ---";
        LOG_INFO << "Environment file: " << env_path;
        if (!drogon_config.empty()) {
          LOG_INFO << "Drogon configuration: " << drogon_config;
        } else {
          LOG_INFO << "Drogon configuration: (internal defaults)";
        }
        LOG_INFO << "Server port: " << port;
        LOG_INFO << "gRPC port: " << grpc_port;
        LOG_INFO << "Database type: " << db_type;

        // Seeder: Ensure at least one admin exists
        drogon::async_run([]() -> drogon::Task<void> {
          co_await drogon_auth::utils::Seeder::ensureAdminExists();
        });

        // Sprint 6: Sync Session-Status from PostgreSQL to In-Memory-Cache
        drogon::app().getLoop()->runEvery(60.0, []() {
            drogon::async_run([]() -> drogon::Task<void> {
                co_await drogon_auth::cache::SessionCache::instance().sync_from_db();
                LOG_DEBUG << "Background-Worker: Synced Session-Cache from DB.";
            });
        });
        
        // Initial sync
        drogon::async_run([]() -> drogon::Task<void> {
            co_await drogon_auth::cache::SessionCache::instance().sync_from_db();
            LOG_INFO << "Initial Session-Cache sync complete.";
        });

        // Sprint 4: Background Worker for Session-Timeout
        drogon::app().getLoop()->runEvery(300.0, []() {
            drogon::async_run([]() -> drogon::Task<void> {
                try {
                    auto db = drogon::app().getDbClient();
                    co_await db->execSqlCoro("DELETE FROM sessions WHERE expires_at < CURRENT_TIMESTAMP");
                    LOG_DEBUG << "Background-Worker: Cleaned up expired sessions.";
                } catch (const std::exception& e) {
                    LOG_ERROR << "Background-Worker Error: " << e.what();
                }
            });
        });

        LOG_INFO << "--- Framework ready ---";
      });

  // 4. Initialize Database
  if (db_type == "postgres") {
    std::string db_host =
        drogon_auth::utils::ConfigUtil::get_string("DB_HOST", "127.0.0.1");
    int db_port = drogon_auth::utils::ConfigUtil::get_int("DB_PORT", 5432);
    std::string db_name =
        drogon_auth::utils::ConfigUtil::get_string("DB_NAME", "postgres");
    std::string db_user =
        drogon_auth::utils::ConfigUtil::get_string("DB_USER", "postgres");
    std::string db_password =
        drogon_auth::utils::ConfigUtil::get_string("DB_PASSWORD", "");

    drogon::app().createDbClient("postgresql", db_host, db_port, db_name,
                                 db_user, db_password, 1, "", "default");
  } else if (db_type == "sqlite3") {
    std::string sqlite_file = drogon_auth::utils::ConfigUtil::get_string(
        "SQLITE_FILE", "gallery.sqlite3");
    drogon::app().createDbClient("sqlite3", "", 0, "", "", "", 1, sqlite_file,
                                 "default");
  }

  LOG_INFO << "Drogon Auth Microservice starting on port " << port;
  drogon::app().addListener("0.0.0.0", port);

  // 5. Start gRPC server in a separate thread
  std::thread grpc_thread([grpc_port]() {
    std::string server_address("0.0.0.0:" + std::to_string(grpc_port));
    drogon_auth::grpc::AuthGrpcServiceImpl service;

    ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address,
                             ::grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());
    if (server) {
      LOG_INFO << "gRPC Server listening on " << server_address;
      server->Wait();
    } else {
      LOG_ERROR << "Failed to start gRPC Server on " << server_address;
    }
  });
  grpc_thread.detach();

  // 6. Global Latency Tracking (Sprint 4)
  drogon::app().registerPreHandlingAdvice(
      [](const drogon::HttpRequestPtr &req,
         drogon::AdviceCallback &&,
         drogon::AdviceChainCallback &&next) {
          req->attributes()->insert("start_time", std::chrono::steady_clock::now());
          next();
      });

  drogon::app().registerPostHandlingAdvice(
      [](const drogon::HttpRequestPtr &req, const drogon::HttpResponsePtr &) {
          if (req->attributes()->find("start_time")) {
              auto start = req->attributes()->get<std::chrono::steady_clock::time_point>("start_time");
              auto end = std::chrono::steady_clock::now();
              double latency_ms = std::chrono::duration<double, std::milli>(end - start).count();
              drogon_auth::metrics::MetricsRegistry::instance().record_latency(req->path(), latency_ms);
          }
      });

  // 7. Run the application
  drogon::app().run();

  LOG_INFO << "--- Drogon Auth Microservice stopped ---";

  return 0;
}
