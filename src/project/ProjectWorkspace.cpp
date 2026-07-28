#include "quantum_sim/project/ProjectWorkspace.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <system_error>

namespace quantum_sim::project {
    namespace {
        std::filesystem::path normalizedPath(
            const std::filesystem::path &path
        ) {
            std::error_code error;
            const std::filesystem::path canonical =
                    std::filesystem::weakly_canonical(path, error);

            return error
                ? std::filesystem::absolute(path)
                : canonical;
        }

        /**
         * Reads a non-empty filesystem path from an environment variable.
         *
         * MSVC uses its allocating secure CRT helper; other compilers use the
         * standard environment API. Callers receive the same owned path.
         */
        std::optional<std::filesystem::path> environmentPath(
            const char *name
        ) {
#ifdef _MSC_VER
            char *value = nullptr;
            std::size_t valueLength{};

            if (
                _dupenv_s(
                    &value,
                    &valueLength,
                    name
                ) != 0 ||
                value == nullptr ||
                value[0] == '\0'
            ) {
                std::free(value);
                return std::nullopt;
            }

            const std::filesystem::path path{value};
            std::free(value);
            return path;
#else
            const char *value = std::getenv(name);

            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            return std::filesystem::path{value};
#endif
        }
    }

    ProjectWorkspace::ProjectWorkspace()
        : ProjectWorkspace{defaultRoot()} {
    }

    ProjectWorkspace::ProjectWorkspace(
        std::filesystem::path root
    )
        : root_{std::move(root)},
          sessionMarkerPath_{root_ / "session.active"},
          autosavePath_{root_ / "recovery.qcanvas"},
          recentProjectsPath_{root_ / "recent-projects.txt"} {
    }

    bool ProjectWorkspace::beginSession() {
        std::error_code error;
        std::filesystem::create_directories(root_, error);

        if (error) {
            throw std::runtime_error{
                "Could not create the QubitCanvas workspace: " +
                error.message()
            };
        }

        const bool previousSessionWasUnclean =
                std::filesystem::exists(sessionMarkerPath_);

        std::ofstream marker{
            sessionMarkerPath_,
            std::ios::trunc
        };

        if (!marker) {
            throw std::runtime_error{
                "Could not create the QubitCanvas session marker."
            };
        }

        marker << "QubitCanvas active session\n";
        sessionActive_ = true;
        return previousSessionWasUnclean;
    }

    void ProjectWorkspace::endSession() noexcept {
        if (!sessionActive_) {
            return;
        }

        std::error_code error;
        std::filesystem::remove(sessionMarkerPath_, error);
        sessionActive_ = false;
    }

    const std::filesystem::path &
    ProjectWorkspace::autosavePath() const noexcept {
        return autosavePath_;
    }

    bool ProjectWorkspace::recoveryAvailable() const noexcept {
        std::error_code error;
        return
            std::filesystem::is_regular_file(autosavePath_, error) &&
            !error;
    }

    void ProjectWorkspace::discardRecovery() noexcept {
        std::error_code error;
        std::filesystem::remove(autosavePath_, error);
    }

    std::vector<std::filesystem::path>
    ProjectWorkspace::recentProjects() const {
        std::ifstream input{recentProjectsPath_};
        std::vector<std::filesystem::path> projects;
        std::string serializedPath;

        while (
            input >>
            std::quoted(serializedPath)
        ) {
            const std::filesystem::path path{
                serializedPath
            };

            std::error_code error;
            if (
                std::filesystem::is_regular_file(path, error) &&
                !error
            ) {
                projects.push_back(normalizedPath(path));
            }
        }

        return projects;
    }

    void ProjectWorkspace::recordRecentProject(
        const std::filesystem::path &projectPath,
        const std::size_t maximumEntries
    ) {
        if (maximumEntries == 0U) {
            return;
        }

        std::filesystem::create_directories(root_);

        const std::filesystem::path normalized =
                normalizedPath(projectPath);

        std::vector<std::filesystem::path> projects =
                recentProjects();

        projects.erase(
            std::remove(
                projects.begin(),
                projects.end(),
                normalized
            ),
            projects.end()
        );

        projects.insert(projects.begin(), normalized);

        if (projects.size() > maximumEntries) {
            projects.resize(maximumEntries);
        }

        const std::filesystem::path temporaryPath =
                recentProjectsPath_.string() + ".tmp";

        {
            std::ofstream output{
                temporaryPath,
                std::ios::trunc
            };

            if (!output) {
                throw std::runtime_error{
                    "Could not update the recent-project list."
                };
            }

            for (const auto &path : projects) {
                output
                    << std::quoted(path.string())
                    << '\n';
            }
        }

        std::error_code error;
        std::filesystem::remove(recentProjectsPath_, error);
        error.clear();
        std::filesystem::rename(
            temporaryPath,
            recentProjectsPath_,
            error
        );

        if (error) {
            throw std::runtime_error{
                "Could not install the recent-project list: " +
                error.message()
            };
        }
    }

    std::filesystem::path ProjectWorkspace::defaultRoot() {
#ifdef _WIN32
        if (const auto root = environmentPath("LOCALAPPDATA")) {
            return root.value() / "QubitCanvas";
        }

        if (const auto root = environmentPath("APPDATA")) {
            return root.value() / "QubitCanvas";
        }
#elif defined(__APPLE__)
        if (const auto home = environmentPath("HOME")) {
            return
                home.value() /
                "Library" /
                "Application Support" /
                "QubitCanvas";
        }
#else
        if (const auto root = environmentPath("XDG_DATA_HOME")) {
            return root.value() / "QubitCanvas";
        }

        if (const auto home = environmentPath("HOME")) {
            return
                home.value() /
                ".local" /
                "share" /
                "QubitCanvas";
        }
#endif

        std::error_code error;
        const std::filesystem::path temporary =
                std::filesystem::temp_directory_path(error);

        return
            (error ? std::filesystem::current_path() : temporary) /
            "QubitCanvas";
    }
}
