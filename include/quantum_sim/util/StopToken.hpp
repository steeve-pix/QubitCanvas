#pragma once

#include <atomic>
#include <memory>
#include <utility>

namespace quantum_sim::util {
    /**
     * Copyable view of a cooperative cancellation request.
     *
     * A default token is never cancelled. Tokens created by StopSource share
     * one atomic flag, so worker and simulation code can poll without locks.
     */
    class StopToken final {
    public:
        /**
         * Creates a token with no source; it never reports cancellation.
         */
        StopToken() noexcept = default;

        /**
         * @return True after the associated source requests cancellation.
         */
        [[nodiscard]] bool stop_requested() const noexcept {
            return
                state_ != nullptr &&
                state_->load(std::memory_order_acquire);
        }

    private:
        friend class StopSource;

        /**
         * Attaches the token to a source-owned cancellation flag.
         *
         * @param state Shared atomic cancellation state.
         */
        explicit StopToken(
            std::shared_ptr<std::atomic_bool> state
        ) noexcept
            : state_{std::move(state)} {
        }

        std::shared_ptr<std::atomic_bool> state_;
    };

    /**
     * Owner that can request cancellation for every copied StopToken.
     */
    class StopSource final {
    public:
        /**
         * Creates a fresh source in the running state.
         */
        StopSource()
            : state_{
                std::make_shared<std::atomic_bool>(false)
            } {
        }

        /**
         * @return Token observing this source's shared cancellation state.
         */
        [[nodiscard]] StopToken get_token() const noexcept {
            return StopToken{state_};
        }

        /**
         * Requests cooperative cancellation.
         *
         * @return True only for the first request made through shared copies.
         */
        bool request_stop() noexcept {
            return
                !state_->exchange(
                    true,
                    std::memory_order_acq_rel
                );
        }

        /**
         * @return True after cancellation has been requested.
         */
        [[nodiscard]] bool stop_requested() const noexcept {
            return state_->load(std::memory_order_acquire);
        }

    private:
        std::shared_ptr<std::atomic_bool> state_;
    };
}
