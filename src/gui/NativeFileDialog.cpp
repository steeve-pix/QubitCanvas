#include "quantum_sim/gui/NativeFileDialog.hpp"

#include <array>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <Windows.h>
#include <commdlg.h>

#include <algorithm>
#else
#include <cstdio>
#include <cstdlib>
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
#else
        /**
         * Runs a desktop file-chooser command and converts its standard output
         * into a filesystem path. Cancellation and missing tools return null.
         */
        std::optional<std::filesystem::path> selectedCommandPath(
            const std::string &command
        ) {
            FILE *pipe = ::popen(command.c_str(), "r");

            if (pipe == nullptr) {
                return std::nullopt;
            }

            std::array<char, 4096U> buffer{};
            std::string output;

            while (
                std::fgets(
                    buffer.data(),
                    static_cast<int>(buffer.size()),
                    pipe
                ) != nullptr
            ) {
                output += buffer.data();
            }

            const int status = ::pclose(pipe);

            while (
                !output.empty() &&
                (
                    output.back() == '\n' ||
                    output.back() == '\r'
                )
            ) {
                output.pop_back();
            }

            if (status != 0 || output.empty()) {
                return std::nullopt;
            }

            return std::filesystem::path{output};
        }

#ifdef __APPLE__
        std::optional<std::filesystem::path> openFile(
            const std::string_view prompt,
            const std::string_view
        ) {
            return selectedCommandPath(
                "osascript -e 'POSIX path of "
                "(choose file with prompt \"" +
                std::string{prompt} +
                "\")' 2>/dev/null"
            );
        }

        std::optional<std::filesystem::path> saveFile(
            const std::string_view prompt,
            const std::string_view defaultName,
            const std::string_view defaultExtension,
            const std::string_view
        ) {
            std::optional<std::filesystem::path> path =
                    selectedCommandPath(
                        "osascript -e 'POSIX path of "
                        "(choose file name with prompt \"" +
                        std::string{prompt} +
                        "\" default name \"" +
                        std::string{defaultName} +
                        "\")' 2>/dev/null"
                    );

            if (path.has_value() && !path->has_extension()) {
                path.value() +=
                        "." +
                        std::string{defaultExtension};
            }

            return path;
        }
#else
        bool commandAvailable(const std::string_view command) {
            return
                std::system(
                    (
                        "command -v " +
                        std::string{command} +
                        " >/dev/null 2>&1"
                    ).c_str()
                ) == 0;
        }

        std::optional<std::filesystem::path> openFile(
            const std::string_view title,
            const std::string_view pattern
        ) {
            if (commandAvailable("zenity")) {
                return selectedCommandPath(
                    "zenity --file-selection --title=\"" +
                    std::string{title} +
                    "\" --file-filter=\"Supported files | " +
                    std::string{pattern} +
                    "\" 2>/dev/null"
                );
            }

            if (commandAvailable("kdialog")) {
                return selectedCommandPath(
                    "kdialog --getopenfilename \"$HOME\" \"" +
                    std::string{pattern} +
                    "|Supported files\" 2>/dev/null"
                );
            }

            return std::nullopt;
        }

        std::optional<std::filesystem::path> saveFile(
            const std::string_view title,
            const std::string_view defaultName,
            const std::string_view defaultExtension,
            const std::string_view pattern
        ) {
            std::optional<std::filesystem::path> path;

            if (commandAvailable("zenity")) {
                path =
                        selectedCommandPath(
                            "zenity --file-selection --save "
                            "--confirm-overwrite --title=\"" +
                            std::string{title} +
                            "\" --filename=\"" +
                            std::string{defaultName} +
                            "\" --file-filter=\"Supported files | " +
                            std::string{pattern} +
                            "\" 2>/dev/null"
                        );
            } else if (commandAvailable("kdialog")) {
                path =
                        selectedCommandPath(
                            "kdialog --getsavefilename "
                            "\"$HOME/" +
                            std::string{defaultName} +
                            "\" \"" +
                            std::string{pattern} +
                            "|Supported files\" 2>/dev/null"
                        );
            }

            if (path.has_value() && !path->has_extension()) {
                path.value() +=
                        "." +
                        std::string{defaultExtension};
            }

            return path;
        }
#endif
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
        return openFile(
            "Open QubitCanvas project",
            "*.qcanvas"
        );
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
        return saveFile(
            "Save QubitCanvas project",
            "QubitCanvas.qcanvas",
            "qcanvas",
            "*.qcanvas"
        );
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
        return openFile(
            "Open OpenQASM 3 source",
            "*.qasm"
        );
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
        return saveFile(
            "Export OpenQASM 3 source",
            "circuit.qasm",
            "qasm",
            "*.qasm"
        );
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
        return saveFile(
            "Export circuit diagram",
            "circuit.svg",
            "svg",
            "*.svg"
        );
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
        return saveFile(
            "Export state-vector data",
            "state.csv",
            "csv",
            "*.csv"
        );
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
        return saveFile(
            "Export density-matrix data",
            "density.csv",
            "csv",
            "*.csv"
        );
#endif
    }
}
