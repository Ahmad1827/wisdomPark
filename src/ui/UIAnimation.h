#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <algorithm>

namespace WisdomUI {

    class Animation {
    public:
        static float Lerp(float a, float b, float t) {
            return a + (b - a) * t;
        }

        static float SmoothStep(float edge0, float edge1, float x) {
            float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        static float EaseOutCubic(float t) {
            t = std::clamp(t, 0.0f, 1.0f);
            float f = t - 1.0f;
            return f * f * f + 1.0f;
        }

        static float EaseOutBack(float t, float overshoot = 1.70158f) {
            t = std::clamp(t, 0.0f, 1.0f);
            float f = t - 1.0f;
            return f * f * ((overshoot + 1.0f) * f + overshoot) + 1.0f;
        }

        static float EaseInOutQuad(float t) {
            t = std::clamp(t, 0.0f, 1.0f);
            return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
        }

        static float Pulse(float time, float speed, float minVal = 0.0f, float maxVal = 1.0f) {
            float wave = (std::sin(time * speed) + 1.0f) * 0.5f;
            return minVal + wave * (maxVal - minVal);
        }

        static sf::Color InterpolateColor(const sf::Color& a, const sf::Color& b, float t) {
            t = std::clamp(t, 0.0f, 1.0f);
            return sf::Color(
                static_cast<sf::Uint8>(a.r + (b.r - a.r) * t),
                static_cast<sf::Uint8>(a.g + (b.g - a.g) * t),
                static_cast<sf::Uint8>(a.b + (b.b - a.b) * t),
                static_cast<sf::Uint8>(a.a + (b.a - a.a) * t)
            );
        }
    };

    struct SpringFloat {
        float current{ 0.0f };
        float target{ 0.0f };
        float velocity{ 0.0f };
        float stiffness{ 180.0f };
        float damping{ 14.0f };

        void Update(float dt) {
            float force = -stiffness * (current - target);
            float loss = -damping * velocity;
            velocity += (force + loss) * dt;
            current += velocity * dt;
        }

        void SetImmediate(float val) {
            current = val;
            target = val;
            velocity = 0.0f;
        }
    };

}