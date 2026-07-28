#pragma once

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/debug/DebuggerSession.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeModel.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>

namespace quantum_sim::gui {
    /**
     * Completed debugger trace and density history produced by the worker.
     */
    struct SimulationHistoryResult {
        std::uint64_t requestId{};
        std::optional<debug::DebuggerSession> session;
        density_volume::DensityStack densityStack;
        std::optional<std::size_t> preferredStep;
        bool followPreferredStep{false};
        std::string error;
    };

    /**
     * Builds simulation histories away from the rendering thread.
     *
     * Submitting a new request cooperatively cancels active work and replaces
     * any queued request. Only the newest generation may publish a result.
     */
    class SimulationHistoryWorker final {
    public:
        /**
         * Starts the persistent worker thread.
         */
        SimulationHistoryWorker();

        /**
         * Stops active work and joins the worker thread.
         */
        ~SimulationHistoryWorker();

        SimulationHistoryWorker(const SimulationHistoryWorker &) = delete;
        SimulationHistoryWorker &operator=(const SimulationHistoryWorker &) = delete;
        SimulationHistoryWorker(SimulationHistoryWorker &&) = delete;
        SimulationHistoryWorker &operator=(SimulationHistoryWorker &&) = delete;

        /**
         * Queues the newest circuit state for trace and density construction.
         *
         * @param circuit Immutable circuit snapshot for the build.
         * @param initialState Immutable initial register snapshot.
         * @param previousSession Existing trace used by suffix rebuilds.
         * @param previousDensity Existing density history used by suffix rebuilds.
         * @param firstChangedInstruction Earliest changed instruction, if known.
         * @param preferredStep Timeline step to reveal after adoption.
         * @param followPreferredStep Whether preferredStep should be applied.
         * @return Monotonically increasing request generation.
         */
        std::uint64_t request(
            circuit::QuantumCircuit circuit,
            quantum::QuantumRegister initialState,
            debug::DebuggerSession previousSession,
            density_volume::DensityStack previousDensity,
            std::optional<std::size_t> firstChangedInstruction,
            std::optional<std::size_t> preferredStep,
            bool followPreferredStep
        );

        /**
         * Returns and clears the newest completed result.
         */
        [[nodiscard]] std::optional<SimulationHistoryResult> takeCompleted();

        /**
         * @return True while a request is queued or executing.
         */
        [[nodiscard]] bool busy() const noexcept;

        /**
         * Cancels queued and active work without stopping the worker itself.
         */
        void cancel() noexcept;

    private:
        struct Request {
            std::uint64_t id{};
            circuit::QuantumCircuit circuit;
            quantum::QuantumRegister initialState;
            debug::DebuggerSession previousSession;
            density_volume::DensityStack previousDensity;
            std::optional<std::size_t> firstChangedInstruction;
            std::optional<std::size_t> preferredStep;
            bool followPreferredStep{false};
            std::stop_source stopSource;
        };

        mutable std::mutex mutex_;
        std::condition_variable_any wakeCondition_;
        std::optional<Request> pendingRequest_;
        std::optional<SimulationHistoryResult> completedResult_;
        std::optional<std::stop_source> activeStopSource_;
        std::atomic_bool busy_{false};
        std::uint64_t nextRequestId_{1U};
        std::uint64_t newestRequestId_{};
        // Declared last so destruction joins the thread before shared state dies.
        std::jthread worker_;

        /**
         * Waits for requests and builds only the newest available generation.
         */
        void run(std::stop_token stopToken);
    };
}
