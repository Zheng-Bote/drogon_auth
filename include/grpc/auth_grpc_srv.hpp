/**
 * SPDX-FileComment: gRPC Authentication Service Implementation Header
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <grpcpp/grpcpp.h>
#include "auth_service.grpc.pb.h"

namespace drogon_auth::grpc {

class AuthGrpcServiceImpl final : public AuthService::Service {
public:
    ::grpc::Status GetUserStatus(::grpc::ServerContext* context,
                                const UserStatusRequest* request,
                                UserStatusResponse* response) override;

    ::grpc::Status CheckHealth(::grpc::ServerContext* context,
                              const HealthCheckRequest* request,
                              HealthCheckResponse* response) override;

    ::grpc::Status GetUserProfile(::grpc::ServerContext* context,
                                 const UserRequest* request,
                                 UserProfileResponse* response) override;

    ::grpc::Status GetUserCommunications(::grpc::ServerContext* context,
                                        const UserRequest* request,
                                        UserCommunicationsResponse* response) override;
};

} // namespace drogon_auth::grpc
