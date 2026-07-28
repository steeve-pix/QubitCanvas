#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

namespace quantum_sim::project {
    /**
     * Owns small pieces of editor state that outlive one application run.
     *
     * ProjectWorkspace does not serialize quantum data. ProjectFile remains
     * the single authority for `.qcanvas` contents; this class only chooses
     * durable paths, marks sessions, and maintains the recent-project index.
     */
    class ProjectWorkspace final {
    public:
        /**
         * Uses the platform-local QubitCanvas data directory.
         */
        ProjectWorkspace();

        /**
         * Uses an explicit root, primarily for deterministic tests.
         *
         * @param root Directory containing session and recent-project files.
         */
        explicit ProjectWorkspace(std::filesystem::path root);

        /**
         * Marks this process as active.
         *
         * @return True when an earlier process left an unclean-session marker.
         * @throws std::runtime_error when the workspace cannot be created.
         */
        [[nodiscard]] bool beginSession();

        /**
         * Removes the active-session marker after an orderly shutdown.
         */
        void endSession() noexcept;

        /**
         * @return Dedicated project-file path used for recoverable edits.
         */
        [[nodiscard]] const std::filesystem::path &autosavePath() const noexcept;

        /**
         * @return True when a recovery project currently exists.
         */
        [[nodiscard]] bool recoveryAvailable() const noexcept;

        /**
         * Removes a stale or accepted recovery project.
         */
        void discardRecovery() noexcept;

        /**
         * Reads existing recent project paths, newest first.
         *
         * Missing files are omitted so the menu heals itself over time.
         */
        [[nodiscard]] std::vector<std::filesystem::path>
        recentProjects() const;

        /**
         * Moves one project to the front of the bounded recent list.
         *
         * @param projectPath Successfully opened or saved project.
         * @param maximumEntries Maximum number of paths to retain.
         */
        void recordRecentProject(
            const std::filesystem::path &projectPath,
            std::size_t maximumEntries = 8U
        );

        /**
         * @return Platform-local directory used by the default constructor.
         */
        [[nodiscard]] static std::filesystem::path defaultRoot();

    private:
        std::filesystem::path root_;
        std::filesystem::path sessionMarkerPath_;
        std::filesystem::path autosavePath_;
        std::filesystem::path recentProjectsPath_;
        bool sessionActive_{false};
    };
}
