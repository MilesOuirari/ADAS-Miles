/**
 * @file test_signal_bus.cpp
 * @brief Unit tests for the SignalBus / Port communication layer.
 */

#include "core/Port.hpp"
#include "core/SignalBus.hpp"
#include "data/DataModels.hpp"

#include <iostream>
#include <cassert>
#include <string>

// ─── Test Helpers ──────────────────────────────────────────────────────────

static int tests_passed = 0;
static int tests_total  = 0;

#define TEST(name) \
    do { tests_total++; std::cout << "  [TEST] " << name << "... "; } while(0)

#define PASS() \
    do { tests_passed++; std::cout << "PASS" << std::endl; } while(0)

#define ASSERT_EQ(a, b) \
    do { if ((a) != (b)) { std::cout << "FAIL (" << (a) << " != " << (b) << ")" << std::endl; return; } } while(0)

#define ASSERT_TRUE(expr) \
    do { if (!(expr)) { std::cout << "FAIL (assertion failed)" << std::endl; return; } } while(0)

// ─── Tests ─────────────────────────────────────────────────────────────────

void test_sender_receiver_basic() {
    TEST("SenderPort → ReceiverPort basic round-trip");

    adas::SenderPort<float> sender;
    adas::ReceiverPort<float> receiver;
    receiver.connect(sender.slot());

    sender.write(42.0f);

    ASSERT_EQ(receiver.read(), 42.0f);
    ASSERT_TRUE(receiver.isUpdated());

    receiver.clearUpdated();
    ASSERT_TRUE(!receiver.isUpdated());
    PASS();
}

void test_sender_multiple_receivers() {
    TEST("One sender → multiple receivers");

    adas::SenderPort<int> sender;
    adas::ReceiverPort<int> r1, r2, r3;
    r1.connect(sender.slot());
    r2.connect(sender.slot());
    r3.connect(sender.slot());

    sender.write(99);

    ASSERT_EQ(r1.read(), 99);
    ASSERT_EQ(r2.read(), 99);
    ASSERT_EQ(r3.read(), 99);
    PASS();
}

void test_signal_bus_registration() {
    TEST("SignalBus register + connect");

    adas::SignalBus bus;
    adas::SenderPort<float> sender;
    adas::ReceiverPort<float> receiver;

    bus.registerSender<float>("test/speed", sender);
    bool ok = bus.connectReceiver<float>("test/speed", receiver);

    ASSERT_TRUE(ok);
    ASSERT_TRUE(bus.hasSignal("test/speed"));
    ASSERT_EQ(bus.signalCount(), 1u);

    sender.write(120.5f);
    ASSERT_EQ(receiver.read(), 120.5f);
    PASS();
}

void test_signal_bus_missing_signal() {
    TEST("SignalBus connect to non-existent signal");

    adas::SignalBus bus;
    adas::ReceiverPort<int> receiver;
    bool ok = bus.connectReceiver<int>("nonexistent/signal", receiver);

    ASSERT_TRUE(!ok);
    ASSERT_TRUE(!receiver.isConnected());
    PASS();
}

void test_ego_state_roundtrip() {
    TEST("EgoState struct round-trip via bus");

    adas::SignalBus bus;
    adas::SenderPort<adas::EgoState> sender;
    adas::ReceiverPort<adas::EgoState> receiver;

    bus.registerSender<adas::EgoState>("ego/state", sender);
    bus.connectReceiver<adas::EgoState>("ego/state", receiver);

    adas::EgoState ego;
    ego.speed_kph = 85.5f;
    ego.autopilot_engaged = true;
    ego.world_y = 1234.5f;
    ego.lane_x = -3.5f;
    sender.write(ego);

    const auto& received = receiver.read();
    ASSERT_EQ(received.speed_kph, 85.5f);
    ASSERT_TRUE(received.autopilot_engaged);
    ASSERT_EQ(received.world_y, 1234.5f);
    ASSERT_EQ(received.lane_x, -3.5f);
    PASS();
}

void test_unconnected_receiver() {
    TEST("Unconnected receiver returns default");

    adas::ReceiverPort<float> receiver;
    ASSERT_TRUE(!receiver.isConnected());
    ASSERT_EQ(receiver.read(), 0.0f);   // Default-initialized
    PASS();
}

// ─── Main ──────────────────────────────────────────────────────────────────

int main() {
    std::cout << "═══ Signal Bus Unit Tests ═══" << std::endl;

    test_sender_receiver_basic();
    test_sender_multiple_receivers();
    test_signal_bus_registration();
    test_signal_bus_missing_signal();
    test_ego_state_roundtrip();
    test_unconnected_receiver();

    std::cout << "\n" << tests_passed << "/" << tests_total << " tests passed." << std::endl;
    return (tests_passed == tests_total) ? 0 : 1;
}
