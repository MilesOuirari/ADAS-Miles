/**
 * @file test_e2e.cpp
 * @brief Unit tests for E2E (End-to-End) Protection mechanism.
 */

#include "safety/E2EProtection.hpp"
#include <iostream>
#include <cassert>

static int tests_passed = 0;
static int tests_total  = 0;

#define TEST(name) \
    do { tests_total++; std::cout << "  [TEST] " << name << "... "; } while(0)
#define PASS() \
    do { tests_passed++; std::cout << "PASS" << std::endl; } while(0)
#define ASSERT_TRUE(expr) \
    do { if (!(expr)) { std::cout << "FAIL" << std::endl; return; } } while(0)
#define ASSERT_EQ(a, b) \
    do { if ((a) != (b)) { std::cout << "FAIL (" << (int)(a) << " != " << (int)(b) << ")" << std::endl; return; } } while(0)

void test_crc_deterministic() {
    TEST("CRC-8 is deterministic");
    adas::uint8 data[] = {1, 2, 3};
    adas::uint8 crc1 = adas::E2EChecker::compute_crc8(data, 3);
    adas::uint8 crc2 = adas::E2EChecker::compute_crc8(data, 3);
    ASSERT_EQ(crc1, crc2);
    PASS();
}

void test_crc_changes_with_data() {
    TEST("CRC-8 changes with different data");
    adas::uint8 data1[] = {1, 2, 3};
    adas::uint8 data2[] = {1, 2, 4};
    adas::uint8 crc1 = adas::E2EChecker::compute_crc8(data1, 3);
    adas::uint8 crc2 = adas::E2EChecker::compute_crc8(data2, 3);
    ASSERT_TRUE(crc1 != crc2);
    PASS();
}

void test_e2e_first_message_accepted() {
    TEST("E2E accepts first message");
    adas::E2EChecker checker;
    adas::uint8 data[] = {10, 20, 30};
    adas::uint8 crc = adas::E2EChecker::compute_crc8(data, 3);
    ASSERT_TRUE(checker.check(0, data, 3, crc));
    PASS();
}

void test_e2e_sequential_messages() {
    TEST("E2E accepts sequential messages");
    adas::E2EChecker checker;
    adas::uint8 data[] = {10, 20, 30};
    adas::uint8 crc = adas::E2EChecker::compute_crc8(data, 3);
    ASSERT_TRUE(checker.check(0, data, 3, crc));   // First
    ASSERT_TRUE(checker.check(1, data, 3, crc));   // Second
    ASSERT_TRUE(checker.check(2, data, 3, crc));   // Third
    PASS();
}

void test_e2e_wrong_crc_rejected() {
    TEST("E2E rejects wrong CRC");
    adas::E2EChecker checker;
    adas::uint8 data[] = {10, 20, 30};
    ASSERT_TRUE(!checker.check(0, data, 3, 0xAA));  // Wrong CRC
    PASS();
}

void test_e2e_counter_jump_rejected() {
    TEST("E2E rejects counter jump");
    adas::E2EChecker checker;
    adas::uint8 data[] = {10, 20, 30};
    adas::uint8 crc = adas::E2EChecker::compute_crc8(data, 3);
    ASSERT_TRUE(checker.check(0, data, 3, crc));
    ASSERT_TRUE(!checker.check(5, data, 3, crc));   // Counter jump
    PASS();
}

void test_e2e_reset() {
    TEST("E2E reset works");
    adas::E2EChecker checker;
    adas::uint8 data[] = {10, 20, 30};
    adas::uint8 crc = adas::E2EChecker::compute_crc8(data, 3);
    checker.check(0, data, 3, crc);
    checker.check(1, data, 3, crc);
    checker.reset();
    ASSERT_TRUE(checker.check(0, data, 3, crc));   // Should accept as first message
    PASS();
}

int main() {
    std::cout << "═══ E2E Protection Unit Tests ═══" << std::endl;

    test_crc_deterministic();
    test_crc_changes_with_data();
    test_e2e_first_message_accepted();
    test_e2e_sequential_messages();
    test_e2e_wrong_crc_rejected();
    test_e2e_counter_jump_rejected();
    test_e2e_reset();

    std::cout << "\n" << tests_passed << "/" << tests_total << " tests passed." << std::endl;
    return (tests_passed == tests_total) ? 0 : 1;
}
