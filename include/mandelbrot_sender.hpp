#pragma once

#include <stdexec/execution.hpp>

#include "types.hpp"

template <typename Receiver>
struct MandelbrotOperationState {

    Receiver receiver_;
    mandelbrot::ViewPort viewport_;
    RenderSettings settings_;
    PixelRegion region_;

    friend void tag_invoke(stdexec::start_t, MandelbrotOperationState &self) noexcept {
            try {
                const auto start_row = self.region_.start_row;
                const auto start_col = self.region_.start_col;
                const auto end_row = self.region_.end_row;
                const auto end_col = self.region_.end_col;

                PixelMatrix matrix(end_row - start_row, std::vector<std::uint32_t>(end_col - start_col));

                auto ys = std::views::iota(start_row, end_row);
                auto xs = std::views::iota(start_col, end_col);

                for (auto &&[y, x] : std::views::cartesian_product(ys, xs)) {
                    const auto c = mandelbrot::Pixel2DToComplex(
                        x, y, self.viewport_, self.settings_.width, self.settings_.height);
                    matrix[y - start_row][x - start_col] = mandelbrot::CalculateIterationsForPoint(
                        c, self.settings_.max_iterations, self.settings_.escape_radius);
                }

                stdexec::set_value(std::move(self.receiver_), std::move(matrix));
            } catch (...) {
                stdexec::set_error(std::move(self.receiver_), std::current_exception());
            }
        }
};

struct MandelbrotSender {
    using sender_concept = stdexec::sender_t;
    using completion_signatures = stdexec::completion_signatures<
        stdexec::set_value_t(PixelMatrix &&),
        stdexec::set_error_t(std::exception_ptr),
        stdexec::set_stopped_t()
    >;

    mandelbrot::ViewPort viewport_;
    RenderSettings settings_;
    PixelRegion region_;
};

[[nodiscard]] inline auto MakeMandelbrotSender(mandelbrot::ViewPort viewport, RenderSettings settings,
                                               PixelRegion region) {
    return MandelbrotSender<void>{viewport, settings, region};
}
