// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "hw/comm_transport.h"
#include "libretro.h"

#include <optional>
#include <string_view>

namespace sm2::libretro {

/// Official Libretro Netpacket wire for two linked Model 2 cabinets.
/// The emulated ring remains in hw::M2Comm; this class only carries complete
/// communication-board frames between two frontend instances.
class NetpacketTransport final : public hw::CommTransport {
public:
    void configure(std::string_view game, unsigned cabinets);

    [[nodiscard]] bool ready() const override;
    void send(std::span<const u8> frame) override;
    [[nodiscard]] std::optional<std::vector<u8>> recv() override;
    [[nodiscard]] bool connected() const override;
    void reset() override;
};

[[nodiscard]] bool register_netpacket_interface(retro_environment_t environment,
                                                retro_log_printf_t logger);
[[nodiscard]] bool netpacket_interface_supported();
void shutdown_netpacket_interface();

}  // namespace sm2::libretro
