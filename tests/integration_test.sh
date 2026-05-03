#!/usr/bin/bash

# Configuration
API_BASE_URL="http://localhost:8849/api/auth/v1"
ADMIN_USER="admin"
GRPC_ADDRESS="localhost:50051"

# Parameters
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <ADMIN_PASSWORD> <GRPC_API_KEY>"
    exit 1
fi

ADMIN_PASSWORD=$1
GRPC_API_KEY=$2

echo "--- 1. REST API Test ---"
echo "Authenticating as admin..."

# 1. Login to get JSESSIONID
COOKIE_FILE=$(mktemp)
LOGIN_RESPONSE=$(curl -s -c "$COOKIE_FILE" -X POST "$API_BASE_URL/login" \
     -H "Content-Type: application/json" \
     -d "{\"loginname\": \"$ADMIN_USER\", \"password\": \"$ADMIN_PASSWORD\"}")

echo "Login Response: $LOGIN_RESPONSE"

# Check if login was successful
if [[ "$LOGIN_RESPONSE" == *"success"* ]]; then
    echo "Login successful. Fetching 'me' info..."
    # 2. Call /me using the cookie
    ME_RESPONSE=$(curl -s -b "$COOKIE_FILE" -X GET "$API_BASE_URL/me")
    echo "REST /me Response: $ME_RESPONSE"
    
    # 3. Extract session id for gRPC test
    # The cookie file format is: domain  TRUE  path  FALSE  expiry  name  value
    # We use awk to find the line where the 6th field is exactly 'JSESSIONID'
    SESSION_ID=$(awk '$6 == "JSESSIONID" {print $7}' "$COOKIE_FILE" | head -n 1)
    echo "Extracted Session ID: $SESSION_ID"
else
    echo "Login failed. Check your .env credentials and port 8849."
    SESSION_ID="invalid-dummy"
fi
rm "$COOKIE_FILE"

echo -e "\n--- 2. gRPC API Test ---"
echo "Testing gRPC GetUserStatus with actual Session ID..."

# Note: This requires grpcurl installed.
if command -v grpcurl &> /dev/null; then
    # We pass the real session id to see if gRPC finds it in the DB
    grpcurl -plaintext -H "x-api-key: $GRPC_API_KEY" \
        -d "{\"session_id\": \"$SESSION_ID\"}" \
        $GRPC_ADDRESS drogon_auth.grpc.AuthService/GetUserStatus
else
    echo "grpcurl not found. Please install it to run gRPC tests."
fi

echo -e "\n--- Tests Finished ---"
