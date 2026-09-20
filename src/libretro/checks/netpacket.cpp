// SPDX-License-Identifier: BSD-3-Clause
#include "netpacket.h"

#include "hw/m2comm.h"

#include <algorithm>
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

struct RingBus {
    explicit RingBus(unsigned cabinets) : incoming(cabinets) {}
    std::vector<std::deque<std::vector<u8>>> incoming;
};

class RingTransport final : public hw::CommTransport {
public:
    RingTransport(std::shared_ptr<RingBus> bus, unsigned cabinet)
        : m_bus(std::move(bus)), m_cabinet(cabinet) {}

    [[nodiscard]] bool ready() const override { return true; }
    void send(std::span<const u8> frame) override
    {
        const unsigned successor = (m_cabinet + 1) % m_bus->incoming.size();
        m_bus->incoming[successor].emplace_back(frame.begin(), frame.end());
    }
    [[nodiscard]] std::optional<std::vector<u8>> recv() override
    {
        auto& incoming = m_bus->incoming[m_cabinet];
        if (incoming.empty()) return std::nullopt;
        std::vector<u8> frame = std::move(incoming.front());
        incoming.pop_front();
        return frame;
    }
    [[nodiscard]] bool connected() const override { return true; }
    void reset() override { m_bus->incoming[m_cabinet].clear(); }

private:
    std::shared_ptr<RingBus> m_bus;
    unsigned m_cabinet;
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

void check_model2_ring(unsigned cabinet_count)
{
    auto bus = std::make_shared<RingBus>(cabinet_count);
    std::vector<std::unique_ptr<std::array<u8, hw::M2Comm::kSharedSize>>> ram;
    std::vector<std::unique_ptr<hw::M2Comm>> cabinets;
    for (unsigned cabinet = 0; cabinet < cabinet_count; ++cabinet) {
        ram.push_back(std::make_unique<std::array<u8, hw::M2Comm::kSharedSize>>());
        cabinets.push_back(std::make_unique<hw::M2Comm>());
        cabinets.back()->attach_shared(*ram.back());
        cabinets.back()->set_transport(std::make_unique<RingTransport>(bus, cabinet));
        cabinets.back()->reset();
        cabinets.back()->fg_write(cabinet == 0 ? 1 : 0);
        cabinets.back()->cn_write(1);
    }

    for (unsigned frame = 0; frame < 600; ++frame)
        for (auto& cabinet : cabinets) cabinet->vblank();

    std::vector<unsigned> ids;
    for (const auto& cabinet : cabinets) {
        check(cabinet->link_alive(), "Communication board did not establish the ring");
        check(cabinet->link_count() == cabinet_count,
              "Model 2 ring reported an unexpected cabinet count");
        ids.push_back(cabinet->link_id());
    }
    std::sort(ids.begin(), ids.end());
    for (unsigned cabinet = 0; cabinet < cabinet_count; ++cabinet)
        check(ids[cabinet] == cabinet + 1,
              "Model 2 ring assigned duplicate or missing cabinet IDs");

    for (unsigned cabinet = 0; cabinet < cabinet_count; ++cabinet) {
        (*ram[cabinet])[0x2000] = static_cast<u8>(0x40 + cabinet);
        (*ram[cabinet])[0x2001] = static_cast<u8>(0xa0 + cabinet);
    }
    for (unsigned frame = 0; frame < cabinet_count + 4; ++frame)
        for (auto& cabinet : cabinets) cabinet->vblank();
    for (unsigned cabinet = 0; cabinet < cabinet_count; ++cabinet) {
        const unsigned predecessor = (cabinet + cabinet_count - 1) % cabinet_count;
        check((*ram[cabinet])[0x21c0] == 0x40 + predecessor
                  && (*ram[cabinet])[0x21c1] == 0xa0 + predecessor,
              "A cabinet did not receive its predecessor's communication payload");
    }
}

std::vector<u8> begin_handshake(libretro::NetpacketTransport& transport,
                                u16 local_id, unsigned cabinet_count)
{
    transport.configure("daytona", cabinet_count);
    captured_callbacks->start(local_id, send_packet, nullptr);
    if (local_id == 0)
        for (u16 peer = 1; peer < cabinet_count; ++peer)
            check(captured_callbacks->connected(peer), "Configured cabinet was rejected");
    check(!transport.ready(), "Cabinet became ready before peer hellos arrived");
    check(sent_peer == RETRO_NETPACKET_BROADCAST,
          "Handshake hello was not broadcast to all cabinets");
    return sent_packet;
}

void feed_hello(const std::vector<u8>& hello, u16 sender)
{
    captured_callbacks->receive(hello.data(), hello.size(), sender);
}

void check_host_adapter()
{
    libretro::NetpacketTransport transport;
    const std::vector<u8> hello = begin_handshake(transport, 0, 4);
    feed_hello(hello, 1);
    feed_hello(hello, 2);
    check(!transport.ready(), "Host became ready with an incomplete roster");
    feed_hello(hello, 3);
    check(transport.ready(), "Host did not become ready with four cabinets");
    check(!captured_callbacks->connected(4), "A fifth cabinet was accepted");

    const std::array<u8, 4> payload{0xff, 4, 0, 0};
    transport.send(payload);
    check(sent_peer == 1, "Host frame was not addressed to its ring successor");
    check((sent_flags & RETRO_NETPACKET_RELIABLE) != 0,
          "Communication board frame was not reliable");
    const std::vector<u8> frame = sent_packet;

    captured_callbacks->receive(frame.data(), frame.size(), 2);
    check(!transport.recv(), "Host accepted a frame from a non-predecessor cabinet");
    captured_callbacks->receive(frame.data(), frame.size(), 3);
    const auto received = transport.recv();
    check(received && *received == std::vector<u8>(payload.begin(), payload.end()),
          "Host did not receive the predecessor's communication frame");

    captured_callbacks->disconnected(2);
    check(!transport.ready(), "Disconnected roster remained ready");
    captured_callbacks->stop();
}

void check_client_adapter()
{
    libretro::NetpacketTransport transport;
    const std::vector<u8> hello = begin_handshake(transport, 2, 4);
    feed_hello(hello, 0);
    feed_hello(hello, 1);
    feed_hello(hello, 3);
    check(transport.ready(), "Client did not build the four-cabinet roster");

    const std::array<u8, 3> payload{0xfc, 1, 0};
    transport.send(payload);
    check(sent_peer == 3, "Client frame was not addressed to its ring successor");
    const std::vector<u8> frame = sent_packet;
    captured_callbacks->receive(frame.data(), frame.size(), 1);
    const auto received = transport.recv();
    check(received && *received == std::vector<u8>(payload.begin(), payload.end()),
          "Client did not receive its predecessor's communication frame");
    captured_callbacks->stop();
}

void check_nine_cabinet_capacity()
{
    libretro::NetpacketTransport transport;
    const std::vector<u8> hello = begin_handshake(transport, 0, 9);
    for (u16 peer = 1; peer < 9; ++peer) feed_hello(hello, peer);
    check(transport.ready(), "Nine-cabinet roster did not become ready");
    check(!captured_callbacks->connected(9), "A tenth cabinet was accepted");
    captured_callbacks->stop();
}

void check_mismatched_settings()
{
    libretro::NetpacketTransport transport;
    std::vector<u8> hello = begin_handshake(transport, 0, 2);
    hello[6] = 3;
    captured_callbacks->receive(hello.data(), hello.size(), 1);
    check(!transport.ready(), "Mismatched cabinet count completed the handshake");
    captured_callbacks->stop();
}

void check_mismatched_game()
{
    libretro::NetpacketTransport transport;
    std::vector<u8> hello = begin_handshake(transport, 0, 2);
    hello[8] ^= 1;
    captured_callbacks->receive(hello.data(), hello.size(), 1);
    check(!transport.ready(), "Mismatched game completed the handshake");
    captured_callbacks->stop();
}

void check_netpacket_adapter()
{
    check(libretro::register_netpacket_interface(environment, nullptr),
          "Frontend rejected Netpacket registration");
    check(captured_callbacks != nullptr, "Netpacket callbacks were not registered");
    check_host_adapter();
    check_client_adapter();
    check_nine_cabinet_capacity();
    check_mismatched_settings();
    check_mismatched_game();
    libretro::shutdown_netpacket_interface();
}

}  // namespace

int main()
{
    try {
        for (unsigned cabinets : {2u, 3u, 4u, 8u, 9u}) check_model2_ring(cabinets);
        check_netpacket_adapter();
        std::puts("SM2 Libretro Netpacket checks passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "SM2 Libretro Netpacket check failed: %s\n", error.what());
        return 1;
    }
}
