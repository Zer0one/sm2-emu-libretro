// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "hw/m2comm.h"
#include "libretro.h"

#include <string_view>

namespace sm2::libretro {

/// Official Libretro Netpacket wire for two linked Model 2 cabinets.
/// The emulated ring remains in hw::M2Comm; this class only carries complete
/// communication-board frames between two frontend instances.
class NetpacketTransport final : public hw::M2CommTransport {
public:
    void configure(std::string_view game, unsigned cabinets);

    [[nodiscard]] bool ready() const override;
    [[nodiscard]] bool send(std::span<const u8> frame) override;
    [[nodiscard]] bool receive(std::vector<u8>& frame) override;
};

[[nodiscard]] bool register_netpacket_interface(retro_environment_t environment,
                                                retro_log_printf_t logger);
[[nodiscard]] bool netpacket_interface_supported();
void shutdown_netpacket_interface();

}  // namespace sm2::libretro
