// SPDX-License-Identifier: BSD-3-Clause
#include "netpacket.h"

#include <algorithm>
#include <array>
#include <deque>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace sm2::libretro {
namespace {

constexpr std::array<u8, 4> kMagic{'S', 'M', '2', 'N'};
constexpr u8 kWireVersion = 1;
constexpr std::size_t kHeaderSize = 11;
constexpr std::size_t kMaximumFrame = 0x1000;

struct ReceivedPacket {
    std::vector<u8> bytes;
};

struct Session {
    std::mutex mutex;
    bool interface_supported = false;
    bool active = false;
    u16 local_id = 0;
    retro_netpacket_send_t send = nullptr;
    retro_netpacket_poll_receive_t poll_receive = nullptr;
    retro_log_printf_t log = nullptr;
    unsigned expected_cabinets = 2;
    u32 game_hash = 0;
    std::set<u16> clients;
    std::deque<ReceivedPacket> packets;
};

Session session;

u32 hash_name(std::string_view name)
{
    u32 hash = 2166136261u;
    for (const unsigned char character : name) {
        hash ^= character;
        hash *= 16777619u;
    }
    return hash;
}

void append_u16(std::vector<u8>& bytes, u16 value)
{
    bytes.push_back(static_cast<u8>(value));
    bytes.push_back(static_cast<u8>(value >> 8));
}

void append_u32(std::vector<u8>& bytes, u32 value)
{
    bytes.push_back(static_cast<u8>(value));
    bytes.push_back(static_cast<u8>(value >> 8));
    bytes.push_back(static_cast<u8>(value >> 16));
    bytes.push_back(static_cast<u8>(value >> 24));
}

u16 read_u16(const u8* bytes)
{
    return static_cast<u16>(bytes[0]) | (static_cast<u16>(bytes[1]) << 8);
}

u32 read_u32(const u8* bytes)
{
    return static_cast<u32>(bytes[0]) | (static_cast<u32>(bytes[1]) << 8)
         | (static_cast<u32>(bytes[2]) << 16) | (static_cast<u32>(bytes[3]) << 24);
}

void log(enum retro_log_level level, const char* message)
{
    retro_log_printf_t logger = nullptr;
    {
        std::lock_guard lock(session.mutex);
        logger = session.log;
    }
    if (logger) logger(level, "[SM2-Emu] [NetBoard] %s\n", message);
}

void RETRO_CALLCONV start(u16 client_id, retro_netpacket_send_t send,
                          retro_netpacket_poll_receive_t poll_receive)
{
    {
        std::lock_guard lock(session.mutex);
        session.active = true;
        session.local_id = client_id;
        session.send = send;
        session.poll_receive = poll_receive;
        session.clients.clear();
        session.packets.clear();
        if (client_id != 0) session.clients.insert(0);
    }
    log(RETRO_LOG_INFO, client_id == 0 ? "Host session started"
                                      : "Client session started");
}

void RETRO_CALLCONV receive(const void* data, std::size_t size, u16 client_id)
{
    if (data == nullptr || size < kHeaderSize || size > kHeaderSize + kMaximumFrame)
        return;
    const auto* bytes = static_cast<const u8*>(data);
    if (!std::equal(kMagic.begin(), kMagic.end(), bytes) || bytes[4] != kWireVersion)
        return;
    const u32 game_hash = read_u32(bytes + 5);
    const u16 payload_size = read_u16(bytes + 9);
    if (payload_size > kMaximumFrame || size != kHeaderSize + payload_size) return;

    std::lock_guard lock(session.mutex);
    const bool expected_peer = session.local_id == 0
        ? session.clients.count(client_id) != 0 : client_id == 0;
    if (!session.active || game_hash != session.game_hash || !expected_peer) return;
    ReceivedPacket packet;
    packet.bytes.assign(bytes + kHeaderSize, bytes + size);
    session.packets.push_back(std::move(packet));
}

void RETRO_CALLCONV stop()
{
    {
        std::lock_guard lock(session.mutex);
        session.active = false;
        session.send = nullptr;
        session.poll_receive = nullptr;
        session.clients.clear();
        session.packets.clear();
    }
    log(RETRO_LOG_INFO, "Session stopped");
}

bool RETRO_CALLCONV connected(u16 client_id)
{
    bool accepted = false;
    {
        std::lock_guard lock(session.mutex);
        accepted = session.active && session.local_id == 0 && client_id != 0
            && (session.clients.count(client_id) != 0
                || session.clients.size() + 1 < session.expected_cabinets);
        if (accepted) session.clients.insert(client_id);
    }
    log(accepted ? RETRO_LOG_INFO : RETRO_LOG_WARN,
        accepted ? "Slave cabinet connected" : "Additional cabinet rejected");
    return accepted;
}

void RETRO_CALLCONV disconnected(u16 client_id)
{
    {
        std::lock_guard lock(session.mutex);
        session.clients.erase(client_id);
    }
    log(RETRO_LOG_WARN, "Cabinet disconnected");
}

constexpr retro_netpacket_callback callbacks{
    start, receive, stop, nullptr, connected, disconnected,
    "SM2-Emu Model 2 NetBoard v1"
};

}  // namespace

void NetpacketTransport::configure(std::string_view game, unsigned cabinets)
{
    std::lock_guard lock(session.mutex);
    session.game_hash = hash_name(game);
    session.expected_cabinets = std::clamp(cabinets, 2u, 2u);
    session.packets.clear();
}

bool NetpacketTransport::ready() const
{
    std::lock_guard lock(session.mutex);
    return session.active && session.send != nullptr
        && (session.local_id != 0 || session.clients.size() + 1 >= session.expected_cabinets);
}

bool NetpacketTransport::send(std::span<const u8> frame)
{
    if (frame.empty() || frame.size() > kMaximumFrame) return false;
    retro_netpacket_send_t send_callback = nullptr;
    u16 peer = 0;
    u32 game_hash = 0;
    {
        std::lock_guard lock(session.mutex);
        if (!session.active || session.send == nullptr) return false;
        if (session.local_id == 0) {
            if (session.clients.empty()) return false;
            peer = *session.clients.begin();
        }
        send_callback = session.send;
        game_hash = session.game_hash;
    }

    std::vector<u8> packet;
    packet.reserve(kHeaderSize + frame.size());
    packet.insert(packet.end(), kMagic.begin(), kMagic.end());
    packet.push_back(kWireVersion);
    append_u32(packet, game_hash);
    append_u16(packet, static_cast<u16>(frame.size()));
    packet.insert(packet.end(), frame.begin(), frame.end());
    send_callback(RETRO_NETPACKET_RELIABLE | RETRO_NETPACKET_FLUSH_HINT,
                  packet.data(), packet.size(), peer);
    return true;
}

bool NetpacketTransport::receive(std::vector<u8>& frame)
{
    retro_netpacket_poll_receive_t poll = nullptr;
    {
        std::lock_guard lock(session.mutex);
        if (session.active) poll = session.poll_receive;
    }
    if (poll) poll();

    std::lock_guard lock(session.mutex);
    if (session.packets.empty()) return false;
    frame = std::move(session.packets.front().bytes);
    session.packets.pop_front();
    return true;
}

bool register_netpacket_interface(retro_environment_t environment,
                                  retro_log_printf_t logger)
{
    {
        std::lock_guard lock(session.mutex);
        session.log = logger;
    }
    const bool supported = environment
        && environment(RETRO_ENVIRONMENT_SET_NETPACKET_INTERFACE,
                       const_cast<retro_netpacket_callback*>(&callbacks));
    {
        std::lock_guard lock(session.mutex);
        session.interface_supported = supported;
    }
    return supported;
}

bool netpacket_interface_supported()
{
    std::lock_guard lock(session.mutex);
    return session.interface_supported;
}

void shutdown_netpacket_interface()
{
    std::lock_guard lock(session.mutex);
    session.interface_supported = false;
    session.active = false;
    session.local_id = 0;
    session.send = nullptr;
    session.poll_receive = nullptr;
    session.log = nullptr;
    session.clients.clear();
    session.packets.clear();
}

}  // namespace sm2::libretro
