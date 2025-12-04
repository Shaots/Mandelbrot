#pragma once

#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include "mandelbrot_sender.hpp"
#include "types.hpp"

class MandelbrotRenderer {
private:
    exec::static_thread_pool thread_pool_;

public:
    explicit MandelbrotRenderer(std::uint32_t num_threads = std::thread::hardware_concurrency())
        : thread_pool_{num_threads} {}

    template <size_t N>
    [[nodiscard]] auto RenderAsync(mandelbrot::ViewPort viewport, RenderSettings settings) {
        static_assert(N > 0);
        const std::uint32_t rows_per_task = settings.height / N;

        auto strips = [=, this]<std::size_t... Is>(std::index_sequence<Is...>) {
            auto strip_sender = [=, this](size_t i) {
                PixelRegion region{
                    .start_row = static_cast<uint32_t>(i * rows_per_task),
                    .end_row = static_cast<uint32_t>((i == N - 1) ? settings.height : (i + 1) * rows_per_task),
                    .start_col = 0,
                    .end_col = settings.width
                };

                return stdexec::schedule(thread_pool_.get_scheduler()) |
                        stdexec::let_value([=]() { return MandelbrotSender{viewport, settings, region}; }) |
                        stdexec::then([=](PixelMatrix &&pmatrix) {
                            auto cmatrix = ColorMatrix{region.end_row - region.start_row,
                                                    std::vector<mandelbrot::RgbColor>(settings.width)};

                            const auto ys = std::views::iota(0ul, cmatrix.size());
                            const auto xs = std::views::iota(0ul, cmatrix[0].size());

                            for (auto &&[y, x] : std::views::cartesian_product(ys, xs)) {
                                cmatrix[y][x] = mandelbrot::IterationsToColor(
                                    pmatrix[y][x], settings.max_iterations);
                            }

                            return std::make_pair(region, std::move(cmatrix));
                        });
            };
            return stdexec::when_all(strip_sender(Is)...);
        }(std::make_index_sequence<N>{});

        return std::move(strips) | stdexec::then([=](auto &&...results) {
                   const auto start_time = std::chrono::steady_clock::now();
                   auto img = ColorMatrix{settings.height, std::vector<mandelbrot::RgbColor>(settings.width)};
                   (
                    [&](const auto &result) {
                           const auto &[region, colors] = result;
                           for (uint32_t y = 0; y < colors.size(); ++y) {
                               if ((region.start_row + y) < img.size()) {
                                   std::ranges::copy(colors[y], img[region.start_row + y].begin());
                               }
                           }
                       }(results),
                       ...
                    );

                   const auto end_time = std::chrono::steady_clock::now();
                   const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

                   return RenderResult{.color_data = std::move(img),
                                       .viewport = viewport,
                                       .settings = settings,
                                       .render_time = duration};
               });
    }
};
