# API Documentation - Drogon Auth

This document provides a comprehensive list of all API endpoints available in the Drogon Auth Microservice.

## Authentication Endpoints (`/api/auth/v1`)

| Method | Endpoint | Description | Auth Required |
| :--- | :--- | :--- | :---: |
| POST | `/register` | Register a new user account. | No |
| POST | `/login` | Primary login endpoint. Returns `JSESSIONID` cookie. | No |
| POST | `/login/totp` | Second-step login for users with active MFA. | No |
| POST | `/logout` | Invalidate current session and clear cookies. | Yes |
| GET | `/me` | Get basic information about the current user. | Yes |
| GET | `/profile` | Get full profile data including communication channels. | Yes |
| PUT | `/profile` | Update personal profile data. | Yes |
| POST | `/totp/setup` | Generate a new TOTP secret for 2FA activation. | Yes |
| POST | `/totp/verify` | Verify code and activate 2FA for the account. | Yes |
| POST | `/password/change` | Change password for the current user. | Yes |
| POST | `/password/change-forced` | Workflow for mandatory password updates. | No* |
| POST | `/password/reset-request` | Request a password reset link via email. | No |
| POST | `/password/reset-confirm` | Set new password using a reset token. | No |

*\* Requires user_id and old password validation.*

## Administration Endpoints (`/api/auth/admin/v1`)

*All admin endpoints require a valid session with the 'admin' role.*

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| GET | `/users` | List all users with roles and status. |
| POST | `/users` | Create a new user account. |
| PUT | `/users/{id}` | Update an existing user's data or roles. |
| DELETE | `/users/{id}` | Deactivate/Delete a user account. |
| GET | `/roles` | List all defined system roles. |
| POST | `/roles` | Create a new system role. |
| PUT | `/roles/{id}` | Update role name or description. |
| DELETE | `/roles/{id}` | Remove a system role. |
| GET | `/audit/summary` | Get a summary of activities from the last 7 days. |

## System Endpoints (`/api/auth/system`)

| Method | Endpoint | Description | Auth Required |
| :--- | :--- | :--- | :---: |
| GET | `/getVersion` | Returns the current software version. | No |
| GET | `/health-check` | Live health status (Backend & Database). | No |
| GET | `/check-update` | Checks GitHub for newer releases. | No |
| GET | `/sys-info` | Technical metadata (Compiler, C++ Standard). | Yes (Admin) |

## gRPC API (`drogon_auth.grpc.AuthService`)

The microservice provides a native gRPC interface for server-to-server communication, allowing other services to verify sessions without going through HTTP.

### Authentication
All gRPC methods (except `CheckHealth`) require a valid API key provided in the metadata (headers).

| Header | Description |
| :--- | :--- |
| `x-api-key` | The secret key configured via `GRPC_API_KEY` in `.env`. |

### Method: `GetUserStatus`

**Service**: `drogon_auth.grpc.AuthService`  
**Port**: `50051` (Default, configurable via `GRPC_PORT`)

#### Request: `UserStatusRequest`
| Field | Type | Description |
| :--- | :--- | :--- |
| `session_id` | `string` | The value of the `JSESSIONID` cookie to verify. |

#### Response: `UserStatusResponse`
| Field | Type | Description |
| :--- | :--- | :--- |
| `is_authenticated` | `bool` | `true` if the session is valid and not expired. |
| `user_id` | `string` | The UUID of the user associated with the session. |
| `roles` | `repeated string` | List of roles assigned to the user (e.g., `["user", "admin"]`). |
| `is_active` | `bool` | `true` if the user account is enabled. |

### Method: `CheckHealth`

Performs a health check of the gRPC server and a "punch-through" verification of the database connectivity.

**Request**: `HealthCheckRequest` (Empty)

**Response**: `HealthCheckResponse`
| Field | Type | Description |
| :--- | :--- | :--- |
| `status` | `enum` | `SERVING` (1) if OK, `NOT_SERVING` (2) if failed. |
| `db_status` | `string` | `"OK"` or an error message if the database is unreachable. |

### Method: `GetUserProfile`

Retrieves personal profile data for a specific user.

**Request**: `UserRequest`
| Field | Type | Description |
| :--- | :--- | :--- |
| `user_id` | `string` | The UUID of the user. |

**Response**: `UserProfileResponse`
| Field | Type | Description |
| :--- | :--- | :--- |
| `first_name` | `string` | User's first name. |
| `last_name` | `string` | User's last name. |
| `preferred_language` | `string` | ISO language code (e.g., `en`, `de`). |
| `locale` | `string` | User's locale. |
| `timezone` | `string` | User's timezone. |
| `attributes_json` | `string` | Custom attributes stored as a JSON string. |

### Method: `GetUserCommunications`

Retrieves all communication channels (email, phone, etc.) for a specific user.

**Request**: `UserRequest`

**Response**: `UserCommunicationsResponse`
| Field | Type | Description |
| :--- | :--- | :--- |
| `communications` | `repeated` | List of `CommunicationEntry` objects. |

**CommunicationEntry**:
| Field | Type | Description |
| :--- | :--- | :--- |
| `channel` | `string` | The type of channel (e.g., `email`, `sms`). |
| `address` | `string` | The contact address (e.g., email address). |
| `is_active` | `bool` | Whether the channel is active. |
| `verified` | `bool` | Whether the channel has been verified. |

---
*SPDX-License-Identifier: Apache-2.0*
