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
constexpr u8 kWireVersion = 2;
constexpr u8 kPacketHello = 1;
constexpr u8 kPacketFrame = 2;
constexpr std::size_t kHeaderSize = 14;
constexpr std::size_t kMaximumFrame = 0x1000;

struct ReceivedPacket {
    std::vector<u8> bytes;
    u16 sender = 0;
};

struct Session {
    std::mutex mutex;
    bool interface_supported = false;
    bool active = false;
    bool incompatible = false;
    bool roster_logged = false;
    u16 local_id = 0;
    retro_netpacket_send_t send = nullptr;
    retro_netpacket_poll_receive_t poll_receive = nullptr;
    retro_log_printf_t log = nullptr;
    unsigned expected_cabinets = 2;
    u32 game_hash = 0;
    std::set<u16> clients;
    std::set<u16> peers;
    std::vector<u16> roster;
    std::deque<ReceivedPacket> packets;
    std::deque<std::vector<u8>> frames;
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

void log(enum retro_log_level level, const std::string& message)
{
    retro_log_printf_t logger = nullptr;
    {
        std::lock_guard lock(session.mutex);
        logger = session.log;
    }
    if (logger) logger(level, "[SM2-Emu] [NetBoard] %s\n", message.c_str());
}

std::vector<u8> build_packet(u8 type, std::span<const u8> payload)
{
    unsigned cabinets = 2;
    u32 game_hash = 0;
    {
        std::lock_guard lock(session.mutex);
        cabinets = session.expected_cabinets;
        game_hash = session.game_hash;
    }
    std::vector<u8> packet;
    packet.reserve(kHeaderSize + payload.size());
    packet.insert(packet.end(), kMagic.begin(), kMagic.end());
    packet.push_back(kWireVersion);
    packet.push_back(type);
    append_u16(packet, static_cast<u16>(cabinets));
    append_u32(packet, game_hash);
    append_u16(packet, static_cast<u16>(payload.size()));
    packet.insert(packet.end(), payload.begin(), payload.end());
    return packet;
}

bool send_packet(const std::vector<u8>& packet, u16 peer)
{
    retro_netpacket_send_t callback = nullptr;
    {
        std::lock_guard lock(session.mutex);
        if (session.active) callback = session.send;
    }
    if (!callback) return false;
    callback(RETRO_NETPACKET_RELIABLE | RETRO_NETPACKET_FLUSH_HINT,
             packet.data(), packet.size(), peer);
    return true;
}

void send_hello()
{
    send_packet(build_packet(kPacketHello, {}), RETRO_NETPACKET_BROADCAST);
}

void poll_packets()
{
    retro_netpacket_poll_receive_t callback = nullptr;
    {
        std::lock_guard lock(session.mutex);
        if (session.active) callback = session.poll_receive;
    }
    if (callback) callback();
}

void rebuild_roster_locked()
{
    if (session.incompatible || session.peers.size() != session.expected_cabinets - 1) {
        session.roster.clear();
        return;
    }
    session.roster.assign(session.peers.begin(), session.peers.end());
    session.roster.push_back(session.local_id);
    std::sort(session.roster.begin(), session.roster.end());
    if (session.roster.size() != session.expected_cabinets
        || session.roster.front() != 0
        || std::adjacent_find(session.roster.begin(), session.roster.end())
            != session.roster.end())
        session.roster.clear();
}

void drain_packets()
{
    std::deque<ReceivedPacket> pending;
    {
        std::lock_guard lock(session.mutex);
        pending.swap(session.packets);
    }

    bool reply = false;
    bool mismatch = false;
    bool became_ready = false;
    for (auto& received : pending) {
        const auto& bytes = received.bytes;
        if (bytes.size() < kHeaderSize
            || !std::equal(kMagic.begin(), kMagic.end(), bytes.begin())
            || bytes[4] != kWireVersion)
            continue;
        const u8 type = bytes[5];
        const unsigned cabinets = read_u16(bytes.data() + 6);
        const u32 game_hash = read_u32(bytes.data() + 8);
        const std::size_t payload_size = read_u16(bytes.data() + 12);
        if (payload_size > kMaximumFrame || bytes.size() != kHeaderSize + payload_size)
            continue;

        std::lock_guard lock(session.mutex);
        if (!session.active) continue;
        if (cabinets != session.expected_cabinets || game_hash != session.game_hash) {
            mismatch = !session.incompatible;
            session.incompatible = true;
            session.roster.clear();
            continue;
        }
        if (type == kPacketHello && payload_size == 0) {
            const bool inserted = session.peers.insert(received.sender).second;
            reply |= inserted;
            const bool was_ready = !session.roster.empty();
            rebuild_roster_locked();
            became_ready |= !was_ready && !session.roster.empty();
        } else if (type == kPacketFrame && !session.roster.empty()) {
            const auto local = std::find(session.roster.begin(), session.roster.end(),
                                         session.local_id);
            const auto local_index = static_cast<std::size_t>(local - session.roster.begin());
            const u16 predecessor = session.roster[
                (local_index + session.roster.size() - 1) % session.roster.size()];
            if (received.sender == predecessor)
                session.frames.emplace_back(bytes.begin() + kHeaderSize, bytes.end());
        }
    }

    if (mismatch)
        log(RETRO_LOG_ERROR,
            "Linked cabinets use different games or Linked Cabinets settings");
    if (reply) send_hello();
    if (became_ready) {
        unsigned cabinets = 0;
        u16 local_id = 0;
        {
            std::lock_guard lock(session.mutex);
            if (!session.roster_logged) {
                session.roster_logged = true;
                cabinets = session.expected_cabinets;
                local_id = session.local_id;
            }
        }
        if (cabinets)
            log(RETRO_LOG_INFO, "Linked-cabinet roster ready: participant "
                + std::to_string(local_id) + ", " + std::to_string(cabinets)
                + " cabinets");
    }
}

void pump()
{
    poll_packets();
    drain_packets();
    bool waiting = false;
    {
        std::lock_guard lock(session.mutex);
        waiting = session.active && session.send && !session.incompatible
               && session.roster.empty();
    }
    if (waiting) send_hello();
}

void RETRO_CALLCONV start(u16 client_id, retro_netpacket_send_t send,
                          retro_netpacket_poll_receive_t poll_receive)
{
    {
        std::lock_guard lock(session.mutex);
        session.active = true;
        session.incompatible = false;
        session.roster_logged = false;
        session.local_id = client_id;
        session.send = send;
        session.poll_receive = poll_receive;
        session.clients.clear();
        session.peers.clear();
        session.roster.clear();
        session.packets.clear();
        session.frames.clear();
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
    std::lock_guard lock(session.mutex);
    if (!session.active || client_id == session.local_id) return;
    if (session.local_id == 0 && session.clients.count(client_id) == 0) return;
    ReceivedPacket packet;
    packet.sender = client_id;
    packet.bytes.assign(bytes, bytes + size);
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
        session.peers.clear();
        session.roster.clear();
        session.packets.clear();
        session.frames.clear();
    }
    log(RETRO_LOG_INFO, "Session stopped");
}

bool RETRO_CALLCONV connected(u16 client_id)
{
    bool accepted = false;
    unsigned connected_cabinets = 1;
    unsigned expected_cabinets = 2;
    {
        std::lock_guard lock(session.mutex);
        expected_cabinets = session.expected_cabinets;
        accepted = session.active && session.local_id == 0 && client_id != 0
            && (session.clients.count(client_id) != 0
                || session.clients.size() + 1 < expected_cabinets);
        if (accepted) session.clients.insert(client_id);
        connected_cabinets = static_cast<unsigned>(session.clients.size() + 1);
    }
    log(accepted ? RETRO_LOG_INFO : RETRO_LOG_WARN,
        accepted ? "Slave cabinet connected (" + std::to_string(connected_cabinets)
                 + "/" + std::to_string(expected_cabinets) + ")"
                 : "Additional cabinet rejected");
    return accepted;
}

void RETRO_CALLCONV disconnected(u16 client_id)
{
    {
        std::lock_guard lock(session.mutex);
        session.clients.erase(client_id);
        session.peers.erase(client_id);
        session.roster.clear();
        session.frames.clear();
        session.roster_logged = false;
    }
    log(RETRO_LOG_WARN, "Cabinet disconnected");
}

constexpr retro_netpacket_callback callbacks{
    start, receive, stop, nullptr, connected, disconnected,
    "SM2-Emu Model 2 NetBoard v2"
};

}  // namespace

void NetpacketTransport::configure(std::string_view game, unsigned cabinets)
{
    std::lock_guard lock(session.mutex);
    session.game_hash = hash_name(game);
    session.expected_cabinets = std::clamp(cabinets, 2u, 9u);
    session.incompatible = false;
    session.roster_logged = false;
    session.peers.clear();
    session.roster.clear();
    session.packets.clear();
    session.frames.clear();
}

bool NetpacketTransport::ready() const
{
    pump();
    std::lock_guard lock(session.mutex);
    return session.active && session.send != nullptr && !session.incompatible
        && session.roster.size() == session.expected_cabinets;
}

void NetpacketTransport::send(std::span<const u8> frame)
{
    if (frame.empty() || frame.size() > kMaximumFrame) return;
    u16 successor = 0;
    {
        std::lock_guard lock(session.mutex);
        if (!session.active || !session.send || session.incompatible
            || session.roster.size() != session.expected_cabinets)
            return;
        const auto local = std::find(session.roster.begin(), session.roster.end(),
                                     session.local_id);
        const auto local_index = static_cast<std::size_t>(local - session.roster.begin());
        successor = session.roster[(local_index + 1) % session.roster.size()];
    }
    send_packet(build_packet(kPacketFrame, frame), successor);
}

std::optional<std::vector<u8>> NetpacketTransport::recv()
{
    pump();
    std::lock_guard lock(session.mutex);
    if (session.frames.empty()) return std::nullopt;
    std::vector<u8> frame = std::move(session.frames.front());
    session.frames.pop_front();
    return frame;
}

bool NetpacketTransport::connected() const
{
    return ready();
}

void NetpacketTransport::reset()
{
    std::lock_guard lock(session.mutex);
    session.frames.clear();
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
    session.incompatible = false;
    session.roster_logged = false;
    session.local_id = 0;
    session.send = nullptr;
    session.poll_receive = nullptr;
    session.log = nullptr;
    session.clients.clear();
    session.peers.clear();
    session.roster.clear();
    session.packets.clear();
    session.frames.clear();
}

}  // namespace sm2::libretro
