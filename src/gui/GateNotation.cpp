#include "quantum_sim/gui/GateNotation.hpp"

namespace quantum_sim::gui::gate_notation {
    std::string_view displayName(
        const std::string_view gateName
    ) noexcept {
        if (gateName == "Sdg") {
            return "S\xE2\x80\xA0";
        }
        if (gateName == "Tdg") {
            return "T\xE2\x80\xA0";
        }
        if (gateName == "SX") {
            return "\xE2\x88\x9A"
                   "X";
        }
        if (gateName == "SXdg") {
            return "\xE2\x88\x9A"
                   "X\xE2\x80\xA0";
        }
        if (gateName == "CSdg") {
            return "CS\xE2\x80\xA0";
        }
        if (gateName == "CTdg") {
            return "CT\xE2\x80\xA0";
        }
        if (gateName == "sqrtSWAP") {
            return "\xE2\x88\x9A"
                   "SW";
        }

        return gateName;
    }

    std::string_view exchangeBadge(
        const std::string_view gateName
    ) noexcept {
        if (gateName == "SWAP") {
            return "SW";
        }
        if (gateName == "iSWAP") {
            return "iSW";
        }
        if (gateName == "sqrtSWAP") {
            return "\xE2\x88\x9A"
                   "SW";
        }
        if (gateName == "CSWAP") {
            return "CSW";
        }

        return displayName(gateName);
    }

    std::string_view circuitLabel(
        const std::string_view gateName
    ) noexcept {
        if (
            gateName == "SWAP" ||
            gateName == "iSWAP" ||
            gateName == "sqrtSWAP" ||
            gateName == "CSWAP"
        ) {
            return exchangeBadge(gateName);
        }

        return displayName(gateName);
    }
}
