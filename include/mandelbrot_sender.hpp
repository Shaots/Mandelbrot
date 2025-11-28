#pragma once

#include <stdexec/execution.hpp>

#include "types.hpp"

template <typename Receiver>
struct MandelbrotOperationState {

    Receiver receiver_;
    mandelbrot::ViewPort viewport_;
    RenderSettings settings_;
    PixelRegion region_;
};

template <typename Receiver>
struct MandelbrotSender {
    using sender_concept = stdexec::sender_t;
    
    mandelbrot::ViewPort viewport_;
    RenderSettings settings_;
    PixelRegion region_;
};

[[nodiscard]] inline auto MakeMandelbrotSender(mandelbrot::ViewPort viewport, RenderSettings settings,
                                               PixelRegion region) {
    return MandelbrotSender<void>{viewport, settings, region};
}
