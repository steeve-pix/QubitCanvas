#include "quantum_sim/gui/NativeFileDialog.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <commdlg.h>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#endif

namespace quantum_sim::gui {
    namespace {
#ifdef _WIN32
        std::optional<std::filesystem::path> openFile(
            const wchar_t *filter,
            const wchar_t *defaultExtension
        ) {
            std::array<wchar_t, 32768U> pathBuffer{};

            OPENFILENAMEW dialog{};
            dialog.lStructSize = sizeof(dialog);
            dialog.lpstrFilter = filter;
            dialog.lpstrFile = pathBuffer.data();
            dialog.nMaxFile =
                    static_cast<DWORD>(
                        pathBuffer.size()
                    );
            dialog.lpstrDefExt = defaultExtension;
            dialog.Flags =
                    OFN_EXPLORER |
                    OFN_FILEMUSTEXIST |
                    OFN_PATHMUSTEXIST |
                    OFN_NOCHANGEDIR;

            if (GetOpenFileNameW(&dialog) == FALSE) {
                return std::nullopt;
            }

            return std::filesystem::path{
                pathBuffer.data()
            };
        }

        std::optional<std::filesystem::path> saveFile(
            const wchar_t *filter,
            const std::wstring_view defaultName,
            const std::wstring_view defaultExtension
        ) {
            std::array<wchar_t, 32768U> pathBuffer{};

            std::copy(
                defaultName.begin(),
                defaultName.end(),
                pathBuffer.begin()
            );

            OPENFILENAMEW dialog{};
            dialog.lStructSize = sizeof(dialog);
            dialog.lpstrFilter = filter;
            dialog.lpstrFile = pathBuffer.data();
            dialog.nMaxFile =
                    static_cast<DWORD>(
                        pathBuffer.size()
                    );
            dialog.lpstrDefExt =
                    defaultExtension.data();
            dialog.Flags =
                    OFN_EXPLORER |
                    OFN_OVERWRITEPROMPT |
                    OFN_PATHMUSTEXIST |
                    OFN_NOCHANGEDIR;

            if (GetSaveFileNameW(&dialog) == FALSE) {
                return std::nullopt;
            }

            std::filesystem::path path{
                pathBuffer.data()
            };

            if (!path.has_extension()) {
                path +=
                        std::wstring{L"."} +
                        std::wstring{defaultExtension};
            }

            return path;
        }
#endif
    }

    std::optional<std::filesystem::path>
    NativeFileDialog::openProject() {
#ifdef _WIN32
        constexpr wchar_t filter[] =
                L"QubitCanvas project (*.qcanvas)\0*.qcanvas\0"
                L"All files (*.*)\0*.*\0\0";

        return openFile(filter, L"qcanvas");
#else
        return std::nullopt;
#endif
    }

    std::optional<std::filesystem::path>
    NativeFileDialog::saveProject() {
#ifdef _WIN32
        constexpr wchar_t filter[] =
                L"QubitCanvas project (*.qcanvas)\0*.qcanvas\0"
                L"All files (*.*)\0*.*\0\0";

        return saveFile(
            filter,
            L"QubitCanvas.qcanvas",
            L"qcanvas"
        );
#else
        return std::nullopt;
#endif
    }

    std::optional<std::filesystem::path>
    NativeFileDialog::openQasm() {
#ifdef _WIN32
        constexpr wchar_t filter[] =
                L"OpenQASM 3 source (*.qasm)\0*.qasm\0"
                L"All files (*.*)\0*.*\0\0";

        return openFile(filter, L"qasm");
#else
        return std::nullopt;
#endif
    }

    std::optional<std::filesystem::path>
    NativeFileDialog::saveQasm() {
#ifdef _WIN32
        constexpr wchar_t filter[] =
                L"OpenQASM 3 source (*.qasm)\0*.qasm\0"
                L"All files (*.*)\0*.*\0\0";

        return saveFile(
            filter,
            L"circuit.qasm",
            L"qasm"
        );
#else
        return std::nullopt;
#endif
    }

    std::optional<std::filesystem::path>
    NativeFileDialog::saveCircuitSvg() {
#ifdef _WIN32
        constexpr wchar_t filter[] =
                L"SVG circuit diagram (*.svg)\0*.svg\0"
                L"All files (*.*)\0*.*\0\0";

        return saveFile(
            filter,
            L"circuit.svg",
            L"svg"
        );
#else
        return std::nullopt;
#endif
    }

    std::optional<std::filesystem::path>
    NativeFileDialog::saveStateCsv() {
#ifdef _WIN32
        constexpr wchar_t filter[] =
                L"CSV state data (*.csv)\0*.csv\0"
                L"All files (*.*)\0*.*\0\0";

        return saveFile(
            filter,
            L"state.csv",
            L"csv"
        );
#else
        return std::nullopt;
#endif
    }

    std::optional<std::filesystem::path>
    NativeFileDialog::saveDensityCsv() {
#ifdef _WIN32
        constexpr wchar_t filter[] =
                L"CSV density data (*.csv)\0*.csv\0"
                L"All files (*.*)\0*.*\0\0";

        return saveFile(
            filter,
            L"density.csv",
            L"csv"
        );
#else
        return std::nullopt;
#endif
    }
}
