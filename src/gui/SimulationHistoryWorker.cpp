#include "quantum_sim/gui/SimulationHistoryWorker.hpp"

#include <exception>
#include <utility>

namespace quantum_sim::gui {
    SimulationHistoryWorker::SimulationHistoryWorker()
        : worker_{
            [this](const std::stop_token stopToken) {
                run(stopToken);
            }
        } {
    }

    SimulationHistoryWorker::~SimulationHistoryWorker() {
        cancel();
        worker_.request_stop();
        wakeCondition_.notify_all();
    }

    std::uint64_t SimulationHistoryWorker::request(
        circuit::QuantumCircuit circuit,
        quantum::QuantumRegister initialState,
        debug::DebuggerSession previousSession,
        density_volume::DensityStack previousDensity,
        std::optional<std::size_t> firstChangedInstruction,
        std::optional<std::size_t> preferredStep,
        const bool followPreferredStep
    ) {
        std::scoped_lock lock{mutex_};

        if (activeStopSource_.has_value()) {
            activeStopSource_->request_stop();
        }

        if (pendingRequest_.has_value()) {
            pendingRequest_->stopSource.request_stop();
        }

        const std::uint64_t requestId =
                nextRequestId_++;

        newestRequestId_ = requestId;
        completedResult_.reset();
        pendingRequest_.emplace(
            Request{
                .id = requestId,
                .circuit = std::move(circuit),
                .initialState = std::move(initialState),
                .previousSession = std::move(previousSession),
                .previousDensity = std::move(previousDensity),
                .firstChangedInstruction = firstChangedInstruction,
                .preferredStep = preferredStep,
                .followPreferredStep = followPreferredStep
            }
        );

        busy_.store(true, std::memory_order_release);
        wakeCondition_.notify_all();
        return requestId;
    }

    std::optional<SimulationHistoryResult>
    SimulationHistoryWorker::takeCompleted() {
        std::scoped_lock lock{mutex_};

        if (!completedResult_.has_value()) {
            return std::nullopt;
        }

        std::optional<SimulationHistoryResult> result =
                std::move(completedResult_);

        completedResult_.reset();
        return result;
    }

    bool SimulationHistoryWorker::busy() const noexcept {
        return busy_.load(std::memory_order_acquire);
    }

    void SimulationHistoryWorker::cancel() noexcept {
        std::scoped_lock lock{mutex_};

        if (activeStopSource_.has_value()) {
            activeStopSource_->request_stop();
        }

        if (pendingRequest_.has_value()) {
            pendingRequest_->stopSource.request_stop();
            pendingRequest_.reset();
        }

        completedResult_.reset();
        busy_.store(false, std::memory_order_release);
        wakeCondition_.notify_all();
    }

    void SimulationHistoryWorker::run(
        const std::stop_token stopToken
    ) {
        while (!stopToken.stop_requested()) {
            std::optional<Request> request;

            {
                std::unique_lock lock{mutex_};
                wakeCondition_.wait(
                    lock,
                    stopToken,
                    [this] {
                        return pendingRequest_.has_value();
                    }
                );

                if (stopToken.stop_requested()) {
                    return;
                }

                request = std::move(pendingRequest_);
                pendingRequest_.reset();
                activeStopSource_ = request->stopSource;
            }

            SimulationHistoryResult result{
                .requestId = request->id,
                .preferredStep = request->preferredStep,
                .followPreferredStep = request->followPreferredStep
            };

            try {
                debug::DebuggerSession rebuiltSession =
                        std::move(request->previousSession);

                density_volume::DensityStack rebuiltDensity =
                        std::move(request->previousDensity);

                const std::stop_token requestStopToken =
                        request->stopSource.get_token();

                if (request->firstChangedInstruction.has_value()) {
                    rebuiltSession.rebuildFrom(
                        request->circuit,
                        request->initialState,
                        request->firstChangedInstruction.value(),
                        requestStopToken
                    );

                    density_volume::DensityModel::rebuildFrom(
                        rebuiltDensity,
                        rebuiltSession,
                        request->firstChangedInstruction.value(),
                        16U,
                        requestStopToken
                    );
                } else {
                    rebuiltSession.rebuild(
                        request->circuit,
                        request->initialState,
                        requestStopToken
                    );

                    rebuiltDensity =
                            density_volume::DensityModel::build(
                                rebuiltSession,
                                16U,
                                requestStopToken
                            );
                }

                if (requestStopToken.stop_requested()) {
                    throw circuit::TraceBuildCancelled{};
                }

                result.session =
                        std::move(rebuiltSession);

                result.densityStack =
                        std::move(rebuiltDensity);
            } catch (const circuit::TraceBuildCancelled &) {
                continue;
            } catch (const std::exception &error) {
                result.error = error.what();
            } catch (...) {
                result.error =
                        "Unknown simulation history build failure.";
            }

            {
                std::scoped_lock lock{mutex_};
                activeStopSource_.reset();

                if (
                    request->id == newestRequestId_ &&
                    !request->stopSource.stop_requested()
                ) {
                    completedResult_ =
                            std::move(result);

                    busy_.store(
                        false,
                        std::memory_order_release
                    );
                }
            }
        }
    }
}
