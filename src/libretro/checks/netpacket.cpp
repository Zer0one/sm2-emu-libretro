// SPDX-License-Identifier: BSD-3-Clause
#include "netpacket.h"

#include "hw/m2comm.h"

#include <array>
#include <cstdio>
#include <deque>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

using namespace sm2;

namespace {

void check(bool okay, const char* message)
{
    if (!okay) throw std::runtime_error(message);
}

class PairTransport final : public hw::CommTransport {
public:
    PairTransport* peer = nullptr;
    std::deque<std::vector<u8>> incoming;

    [[nodiscard]] bool ready() const override { return peer != nullptr; }
    void send(std::span<const u8> frame) override
    {
        if (!peer) return;
        peer->incoming.emplace_back(frame.begin(), frame.end());
    }
    [[nodiscard]] std::optional<std::vector<u8>> recv() override
    {
        if (incoming.empty()) return std::nullopt;
        std::vector<u8> frame = std::move(incoming.front());
        incoming.pop_front();
        return frame;
    }
    [[nodiscard]] bool connected() const override { return peer != nullptr; }
    void reset() override { incoming.clear(); }
};

retro_netpacket_callback* captured_callbacks = nullptr;
std::vector<u8> sent_packet;
int sent_flags = 0;
u16 sent_peer = 0xffff;

bool environment(unsigned command, void* data)
{
    if (command != RETRO_ENVIRONMENT_SET_NETPACKET_INTERFACE) return false;
    captured_callbacks = static_cast<retro_netpacket_callback*>(data);
    return true;
}

void RETRO_CALLCONV send_packet(int flags, const void* data, std::size_t size,
                                u16 client_id)
{
    sent_flags = flags;
    sent_peer = client_id;
    const auto* bytes = static_cast<const u8*>(data);
    sent_packet.assign(bytes, bytes + size);
}

void check_model2_ring()
{
    std::array<u8, hw::M2Comm::kSharedSize> master_ram{};
    std::array<u8, hw::M2Comm::kSharedSize> slave_ram{};
    auto master_wire = std::make_unique<PairTransport>();
    auto slave_wire = std::make_unique<PairTransport>();
    PairTransport* const master_wire_ptr = master_wire.get();
    PairTransport* const slave_wire_ptr = slave_wire.get();
    master_wire_ptr->peer = slave_wire_ptr;
    slave_wire_ptr->peer = master_wire_ptr;

    hw::M2Comm master;
    hw::M2Comm slave;
    master.attach_shared(master_ram);
    slave.attach_shared(slave_ram);
    master.set_transport(std::move(master_wire));
    slave.set_transport(std::move(slave_wire));
    master.reset();
    slave.reset();
    master.fg_write(1);
    slave.fg_write(0);
    master.cn_write(1);
    slave.cn_write(1);

    for (unsigned frame = 0; frame < 300; ++frame) {
        master.vblank();
        slave.vblank();
    }
    check(master.link_alive(), "Master communication board did not establish the ring");
    check(slave.link_alive(), "Slave communication board did not establish the ring");
    check(master.link_id() == 1 && slave.link_id() == 2,
          "Model 2 ring assigned unexpected cabinet IDs");
    check(master.link_count() == 2 && slave.link_count() == 2,
          "Model 2 ring reported an unexpected cabinet count");

    master_ram[0x2000] = 0x5a;
    master_ram[0x2001] = 0xa5;
    for (unsigned frame = 0; frame < 4; ++frame) {
        master.vblank();
        slave.vblank();
    }
    check(slave_ram[0x21c0] == 0x5a && slave_ram[0x21c1] == 0xa5,
          "Slave did not receive the master's communication payload");
}

void check_netpacket_adapter()
{
    check(libretro::register_netpacket_interface(environment, nullptr),
          "Frontend rejected Netpacket registration");
    check(captured_callbacks != nullptr, "Netpacket callbacks were not registered");

    libretro::NetpacketTransport transport;
    transport.configure("daytona", 2);
    captured_callbacks->start(0, send_packet, nullptr);
    check(!transport.ready(), "Host became ready before a client connected");
    check(captured_callbacks->connected(1), "First slave client was rejected");
    check(!captured_callbacks->connected(2), "A third cabinet was accepted");
    check(transport.ready(), "Host did not become ready with two cabinets");

    const std::array<u8, 4> payload{0xff, 2, 0, 0};
    transport.send(payload);
    check(sent_peer == 1, "Host packet was not addressed to the slave");
    check((sent_flags & RETRO_NETPACKET_RELIABLE) != 0,
          "Communication board frame was not reliable");

    captured_callbacks->receive(sent_packet.data(), sent_packet.size(), 1);
    std::optional<std::vector<u8>> received = transport.recv();
    check(received.has_value(), "Received Netpacket frame was not queued");
    check(*received == std::vector<u8>(payload.begin(), payload.end()),
          "Netpacket changed the communication board frame");
    captured_callbacks->receive(sent_packet.data(), sent_packet.size(), 1);
    transport.reset();
    check(transport.ready(), "Communication-board reset disconnected Netpacket");
    check(!transport.recv(), "Communication-board reset retained a stale frame");
    captured_callbacks->receive(sent_packet.data(), sent_packet.size(), 2);
    check(!transport.recv(), "Host accepted a packet from a rejected client");

    captured_callbacks->stop();
    check(!transport.ready(), "Stopped Netpacket session remained ready");
    captured_callbacks->start(1, send_packet, nullptr);
    transport.configure("daytona", 2);
    check(transport.ready(), "Connected slave did not recognize its host");
    transport.send(payload);
    check(sent_peer == 0, "Slave packet was not addressed to the host");
    captured_callbacks->receive(sent_packet.data(), sent_packet.size(), 0);
    received = transport.recv();
    check(received && *received == std::vector<u8>(payload.begin(), payload.end()),
          "Slave did not receive the host's communication-board frame");
    captured_callbacks->stop();
    libretro::shutdown_netpacket_interface();
}

}  // namespace

int main()
{
    try {
        check_model2_ring();
        check_netpacket_adapter();
        std::puts("SM2 Libretro Netpacket checks passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "SM2 Libretro Netpacket check failed: %s\n", error.what());
        return 1;
    }
}
