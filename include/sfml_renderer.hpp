#pragma once

#include <SFML/Graphics.hpp>
#include <print>
#include <stdexec/execution.hpp>

#include "types.hpp"

class SFMLRender {
public:
    using sender_concept = stdexec::sender_t;
    using completion_signatures = stdexec::completion_signatures<
        stdexec::set_value_t(),
        stdexec::set_error_t(std::exception_ptr),
        stdexec::set_stopped_t()
    >;

    template <typename Receiver>
    struct OperationState {
        Receiver receiver_;
        RenderResult render_result_;
        sf::Image &image_;
        sf::Texture &texture_;
        sf::Sprite &sprite_;
        sf::RenderWindow &window_;
        RenderSettings render_settings_;

        friend void tag_invoke(stdexec::start_t, OperationState &self) noexcept {
            try {
                if (!self.render_result_.color_data.empty()) {
                    const auto &colors = self.render_result_.color_data;
                    const auto height = colors.size();
                    const auto width = colors[0].size();

                    const auto ys = std::views::iota(0u, (uint32_t)height);
                    const auto xs = std::views::iota(0u, (uint32_t)width);

                    for (auto &&[y, x] : std::views::cartesian_product(ys, xs)) {
                        const auto &color = colors[y][x];
                        self.image_.setPixel(x, y, sf::Color(color.r, color.g, color.b));
                    }
                }

                self.texture_.update(self.image_);
                self.sprite_.setTexture(self.texture_);

                self.window_.clear();
                self.window_.draw(self.sprite_);
                self.window_.display();

                stdexec::set_value(std::move(self.receiver_));
            } catch (...) {
                stdexec::set_error(std::move(self.receiver_), std::current_exception());
            }
        }
    };

    RenderResult render_result_;
    sf::Image &image_;
    sf::Texture &texture_;
    sf::Sprite &sprite_;
    sf::RenderWindow &window_;
    RenderSettings render_settings_;

    SFMLRender(RenderResult render_result, sf::Image &image, sf::Texture &texture, sf::Sprite &sprite,
               sf::RenderWindow &window, RenderSettings render_settings)
        : render_result_(render_result), image_{image}, texture_{texture}, sprite_{sprite}, window_{window},
          render_settings_{render_settings} {}

    template <stdexec::receiver_of<completion_signatures> Receiver>
    auto connect(Receiver &&receiver) && -> OperationState<std::decay_t<Receiver>> {
        return OperationState<std::decay_t<Receiver>>{
            std::forward<Receiver>(receiver), std::move(render_result_), image_, texture_, sprite_, window_};
    }
};
