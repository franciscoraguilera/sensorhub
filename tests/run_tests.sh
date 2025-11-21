#!/bin/bash

# Run all unit tests

set -e  # Exit on error

echo "================================"
echo "Running SensorHub Unit Tests"
echo "================================"

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

TESTS_PASSED=0
TESTS_FAILED=0

# Function to run a test
run_test() {
    TEST_NAME=$1
    echo ""
    echo "Running $TEST_NAME..."
    if ./$TEST_NAME; then
        echo -e "${GREEN}✓ $TEST_NAME PASSED${NC}"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}✗ $TEST_NAME FAILED${NC}"
        ((TESTS_FAILED++))
    fi
}

# Run tests
run_test "test_utils"
run_test "test_config"
run_test "test_queue"

echo ""
echo "================================"
echo "Test Results:"
echo "  Passed: $TESTS_PASSED"
echo "  Failed: $TESTS_FAILED"
echo "================================"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
