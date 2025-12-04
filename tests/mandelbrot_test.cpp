#include "mandelbrot.hpp"
#include "sfml_events_handler.hpp"
#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST(sfml_events_handler, on_zoom) {
    auto window = sf::RenderWindow{sf::VideoMode({800, 600}), "Mandelbrot"};
    auto settings = RenderSettings{.width = 800, .height = 600};
    auto state = AppState{.left_mouse_pressed = true};
    auto clock = sf::Clock{};

    sf::Mouse::setPosition({400, 300}, window);
    std::this_thread::sleep_for(150ms);

    auto handler = SfmlEventHandler{window, settings, state, clock};
    stdexec::sync_wait(std::move(handler));

    EXPECT_TRUE(state.need_rerender);
    EXPECT_LT(state.viewport.width(), mandelbrot::ViewPort{}.width());
}

TEST(mandelbrot_sender, num_iterations_for_pixels) {
    const auto viewport = mandelbrot::ViewPort{.x_min = -1.0, .x_max = 1.0, .y_min = -1.0, .y_max = 1.0};
    const auto settings = RenderSettings{.width = 4, .height = 4, .max_iterations = 100, .escape_radius = 4.0};
    const auto region = PixelRegion{.start_row = 0, .end_row = 8, .start_col = 0, .end_col = 5};

    auto sender = MandelbrotSender{viewport, settings, region};
    auto [matrix] = stdexec::sync_wait(std::move(sender)).value();

    {
        const auto expected = region.end_row - region.start_row;
        const auto actual = matrix.size();
        ASSERT_EQ(actual, expected);
    }

    {
        const auto expected = region.end_col - region.start_col;
        const auto actual = matrix[0].size();
        ASSERT_EQ(actual, expected);
    }

    {
        const auto expected = settings.max_iterations;
        const auto actual = matrix[1][1];
        EXPECT_EQ(actual, expected);
    }
}

namespace mandelbrot {
// helper
inline bool operator==(const RgbColor &lhs, const RgbColor &rhs) {
    return (lhs.r == rhs.r) && (lhs.g == rhs.g) && (lhs.b == rhs.b);
}
}  // namespace mandelbrot

TEST(mandelbrot_renderer, render_async) {
    const auto viewport = mandelbrot::ViewPort{};
    const auto settings = RenderSettings{.width = 16, .height = 16, .max_iterations = 50};

    auto renderer = MandelbrotRenderer{4};
    auto sender = renderer.RenderAsync<4>(viewport, settings);
    auto [matrix] = stdexec::sync_wait(std::move(sender)).value();

    ASSERT_EQ(matrix.color_data.size(), settings.height);
    ASSERT_EQ(matrix.color_data[0].size(), settings.width);
    EXPECT_EQ(matrix.settings.width, settings.width);
    EXPECT_DOUBLE_EQ(matrix.viewport.x_min, viewport.x_min);
    EXPECT_DOUBLE_EQ(matrix.viewport.y_min, viewport.y_min);
    EXPECT_DOUBLE_EQ(matrix.viewport.x_max, viewport.x_max);
    EXPECT_DOUBLE_EQ(matrix.viewport.y_max, viewport.y_max);

    const auto y_center = matrix.color_data.size() / 2;
    const auto x_center = matrix.color_data[0].size() / 2;

    const auto expected = mandelbrot::RgbColor{0, 0, 0};
    const auto actual = matrix.color_data[y_center][x_center];
    EXPECT_EQ(actual, expected);
}