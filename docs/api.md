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

---
*SPDX-License-Identifier: Apache-2.0*
