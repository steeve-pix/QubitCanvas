#include "quantum_sim/gui/NativeFileDialog.hpp"

#include <algorithm>

#ifdef _WIN32
#include <Windows.h>
#include <commdlg.h>

#include <array>
#endif

namespace quantum_sim::gui {
    std::optional<std::filesystem::path>
    NativeFileDialog::openProject() {
#ifdef _WIN32
        std::array<wchar_t, 32768U> pathBuffer{};
        constexpr wchar_t filter[] =
                L"QubitCanvas project (*.qcanvas)\0*.qcanvas\0"
                L"All files (*.*)\0*.*\0\0";

        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.lpstrFilter = filter;
        dialog.lpstrFile = pathBuffer.data();
        dialog.nMaxFile =
                static_cast<DWORD>(pathBuffer.size());
        dialog.lpstrDefExt = L"qcanvas";
        dialog.Flags =
                OFN_EXPLORER |
                OFN_FILEMUSTEXIST |
                OFN_PATHMUSTEXIST |
                OFN_NOCHANGEDIR;

        if (GetOpenFileNameW(&dialog) == FALSE) {
            return std::nullopt;
        }

        return std::filesystem::path{pathBuffer.data()};
#else
        return std::nullopt;
#endif
    }

    std::optional<std::filesystem::path>
    NativeFileDialog::saveProject() {
#ifdef _WIN32
        std::array<wchar_t, 32768U> pathBuffer{};
        constexpr wchar_t defaultName[] =
                L"QubitCanvas.qcanvas";

        std::copy(
            std::begin(defaultName),
            std::end(defaultName),
            pathBuffer.begin()
        );

        constexpr wchar_t filter[] =
                L"QubitCanvas project (*.qcanvas)\0*.qcanvas\0"
                L"All files (*.*)\0*.*\0\0";

        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.lpstrFilter = filter;
        dialog.lpstrFile = pathBuffer.data();
        dialog.nMaxFile =
                static_cast<DWORD>(pathBuffer.size());
        dialog.lpstrDefExt = L"qcanvas";
        dialog.Flags =
                OFN_EXPLORER |
                OFN_OVERWRITEPROMPT |
                OFN_PATHMUSTEXIST |
                OFN_NOCHANGEDIR;

        if (GetSaveFileNameW(&dialog) == FALSE) {
            return std::nullopt;
        }

        std::filesystem::path path{pathBuffer.data()};

        if (!path.has_extension()) {
            path += L".qcanvas";
        }

        return path;
#else
        return std::nullopt;
#endif
    }
}
