#pragma once

#include "mandelbrot_renderer.hpp"

class CalculateMandelbrotAsyncSender {
public:
    using sender_concept = stdexec::sender_t;
    using completion_signatures = stdexec::completion_signatures<
        stdexec::set_value_t(RenderResult),
        stdexec::set_error_t(std::exception_ptr),
        stdexec::set_stopped_t()
    >;
    
    template <typename Receiver>
    struct OperationState {
        Receiver receiver_;
        AppState &state_;
        RenderSettings render_settings_;
        MandelbrotRenderer &renderer_;

        friend void tag_invoke(stdexec::start_t, OperationState &self) noexcept {
            try {
                if (self.state_.need_rerender) {
                    self.state_.need_rerender = false;

                    const auto sched = stdexec::get_scheduler(stdexec::get_env(self.receiver_));
                    const auto snd1 = self.renderer_.template RenderAsync<THREAD_POOL_SIZE>(
                        self.state_.viewport, self.render_settings_);
                    const auto snd2 = stdexec::continues_on(std::move(snd1), sched);
                    auto op = stdexec::connect(std::move(snd2), std::move(self.receiver_));
                    stdexec::start(op);
                } else {
                    stdexec::set_value(std::move(self.receiver_), RenderResult{});
                }
            } catch (...) {
                stdexec::set_error(std::move(self.receiver_), std::current_exception());
            }
        }
    };

    explicit CalculateMandelbrotAsyncSender(AppState &state, RenderSettings render_settings,
                                            MandelbrotRenderer &renderer)
        : state_(state), render_settings_{render_settings}, renderer_{renderer} {}

    template <stdexec::receiver Receiver>
    auto connect(Receiver &&receiver) && -> OperationState<std::decay_t<Receiver>> {
        return {std::forward<Receiver>(receiver), state_, render_settings_, renderer_};
    }

private:
    RenderSettings render_settings_;
    MandelbrotRenderer &renderer_;
    AppState &state_;
};
