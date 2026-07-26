#include "quantum_sim/gui/QuantumNotation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <numbers>
#include <sstream>
#include <string>

namespace quantum_sim::gui::notation {
    namespace {
        constexpr double symbolicTolerance = 1e-6;
        constexpr const char *squareRoot = "\xE2\x88\x9A";
        constexpr const char *piSymbol = "\xCF\x80";

        struct NamedValue {
            double value;
            const char *text;
        };

        [[nodiscard]] bool approximatelyEqual(
            const double left,
            const double right
        ) noexcept {
            const double scale =
                    std::max(
                        {
                            1.0,
                            std::abs(left),
                            std::abs(right)
                        }
                    );

            return std::abs(left - right) <=
                   symbolicTolerance * scale;
        }

        [[nodiscard]] bool isPerfectSquare(
            const int value
        ) noexcept {
            const int root =
                    static_cast<int>(
                        std::sqrt(static_cast<double>(value))
                    );

            return root * root == value;
        }

        [[nodiscard]] std::string trimDecimal(
            const double value,
            const int decimalPlaces
        ) {
            std::ostringstream output;
            output
                << std::fixed
                << std::setprecision(
                    std::clamp(decimalPlaces, 0, 12)
                )
                << value;

            std::string text =
                    output.str();

            while (
                text.size() > 1U &&
                text.back() == '0'
            ) {
                text.pop_back();
            }

            if (!text.empty() && text.back() == '.') {
                text.pop_back();
            }

            return text == "-0"
                       ? "0"
                       : text;
        }

        [[nodiscard]] std::string signedText(
            const std::string &positiveText,
            const bool negative
        ) {
            return negative
                       ? "-" + positiveText
                       : positiveText;
        }

        [[nodiscard]] std::string formatRational(
            const double positiveValue
        ) {
            for (int denominator = 1; denominator <= 16; ++denominator) {
                const int numerator =
                        static_cast<int>(
                            std::llround(
                                positiveValue *
                                static_cast<double>(denominator)
                            )
                        );

                if (
                    numerator <= 0 ||
                    numerator > 32
                ) {
                    continue;
                }

                const double candidate =
                        static_cast<double>(numerator) /
                        static_cast<double>(denominator);

                if (!approximatelyEqual(positiveValue, candidate)) {
                    continue;
                }

                const int divisor =
                        std::gcd(numerator, denominator);

                const int reducedNumerator =
                        numerator / divisor;

                const int reducedDenominator =
                        denominator / divisor;

                if (reducedDenominator == 1) {
                    return std::to_string(reducedNumerator);
                }

                return
                        std::to_string(reducedNumerator) +
                        "/" +
                        std::to_string(reducedDenominator);
            }

            return {};
        }

        [[nodiscard]] std::string formatSquareRootRatio(
            const double positiveValue
        ) {
            for (int radicand = 2; radicand <= 16; ++radicand) {
                if (isPerfectSquare(radicand)) {
                    continue;
                }

                for (int denominator = 1; denominator <= 16; ++denominator) {
                    const double candidate =
                            std::sqrt(
                                static_cast<double>(radicand)
                            ) /
                            static_cast<double>(denominator);

                    if (!approximatelyEqual(positiveValue, candidate)) {
                        continue;
                    }

                    std::string text =
                            std::string{squareRoot} +
                            std::to_string(radicand);

                    if (denominator != 1) {
                        text +=
                                "/" +
                                std::to_string(denominator);
                    }

                    return text;
                }
            }

            return {};
        }

        [[nodiscard]] std::string formatNamedRadical(
            const double positiveValue
        ) {
            const double rootTwo =
                    std::numbers::sqrt2;

            const double rootSix =
                    std::sqrt(6.0);

            const std::array<NamedValue, 4U> namedValues{
                NamedValue{
                    std::sqrt(2.0 + rootTwo) / 2.0,
                    "\xE2\x88\x9A(2+\xE2\x88\x9A""2)/2"
                },
                NamedValue{
                    std::sqrt(2.0 - rootTwo) / 2.0,
                    "\xE2\x88\x9A(2-\xE2\x88\x9A""2)/2"
                },
                NamedValue{
                    (rootSix + rootTwo) / 4.0,
                    "(\xE2\x88\x9A""6+\xE2\x88\x9A""2)/4"
                },
                NamedValue{
                    (rootSix - rootTwo) / 4.0,
                    "(\xE2\x88\x9A""6-\xE2\x88\x9A""2)/4"
                }
            };

            for (const NamedValue &namedValue : namedValues) {
                if (
                    approximatelyEqual(
                        positiveValue,
                        namedValue.value
                    )
                ) {
                    return namedValue.text;
                }
            }

            return {};
        }
    }

    std::string formatReal(
        const double value,
        const int decimalPlaces
    ) {
        if (!std::isfinite(value)) {
            return trimDecimal(value, decimalPlaces);
        }

        if (approximatelyEqual(value, 0.0)) {
            return "0";
        }

        const bool negative =
                value < 0.0;

        const double positiveValue =
                std::abs(value);

        const std::string rational =
                formatRational(positiveValue);

        if (!rational.empty()) {
            return signedText(rational, negative);
        }

        const std::string squareRootRatio =
                formatSquareRootRatio(positiveValue);

        if (!squareRootRatio.empty()) {
            return signedText(squareRootRatio, negative);
        }

        const std::string namedRadical =
                formatNamedRadical(positiveValue);

        if (!namedRadical.empty()) {
            return signedText(namedRadical, negative);
        }

        return trimDecimal(value, decimalPlaces);
    }

    std::string formatComplex(
        const math::Complex &value,
        const double displayMultiplier,
        const int decimalPlaces
    ) {
        double real =
                value.real() * displayMultiplier;

        double imaginary =
                value.imaginary() * displayMultiplier;

        if (approximatelyEqual(real, 0.0)) {
            real = 0.0;
        }

        if (approximatelyEqual(imaginary, 0.0)) {
            imaginary = 0.0;
        }

        if (imaginary == 0.0) {
            return formatReal(real, decimalPlaces);
        }

        const double imaginaryMagnitude =
                std::abs(imaginary);

        const std::string imaginaryCoefficient =
                approximatelyEqual(
                    imaginaryMagnitude,
                    1.0
                )
                    ? ""
                    : formatReal(
                        imaginaryMagnitude,
                        decimalPlaces
                    );

        const std::string imaginaryText =
                imaginaryCoefficient + "i";

        if (real == 0.0) {
            return imaginary < 0.0
                       ? "-" + imaginaryText
                       : imaginaryText;
        }

        return
                formatReal(real, decimalPlaces) +
                (imaginary < 0.0 ? "-" : "+") +
                imaginaryText;
    }

    std::string formatRadians(
        const double radians,
        const int decimalPlaces,
        const bool includeUnit
    ) {
        std::string text;

        if (approximatelyEqual(radians, 0.0)) {
            text = "0";
        } else {
            const double piRatio =
                    radians /
                    std::numbers::pi;

            bool formattedAsPi = false;

            for (int denominator = 1; denominator <= 32; ++denominator) {
                const int numerator =
                        static_cast<int>(
                            std::llround(
                                piRatio *
                                static_cast<double>(denominator)
                            )
                        );

                if (
                    numerator == 0 ||
                    std::abs(numerator) > 64
                ) {
                    continue;
                }

                const double candidate =
                        static_cast<double>(numerator) /
                        static_cast<double>(denominator);

                if (!approximatelyEqual(piRatio, candidate)) {
                    continue;
                }

                const int divisor =
                        std::gcd(
                            std::abs(numerator),
                            denominator
                        );

                const int reducedNumerator =
                        numerator / divisor;

                const int reducedDenominator =
                        denominator / divisor;

                if (reducedNumerator == -1) {
                    text = "-";
                } else if (reducedNumerator != 1) {
                    text =
                            std::to_string(reducedNumerator);
                }

                text += piSymbol;

                if (reducedDenominator != 1) {
                    text +=
                            "/" +
                            std::to_string(reducedDenominator);
                }

                formattedAsPi = true;
                break;
            }

            if (!formattedAsPi) {
                text =
                        trimDecimal(
                            radians,
                            decimalPlaces
                        );
            }
        }

        if (includeUnit) {
            text += " rad";
        }

        return text;
    }

    std::string formatPolarAmplitude(
        const double magnitude,
        const double phaseRadians,
        const int decimalPlaces
    ) {
        if (approximatelyEqual(magnitude, 0.0)) {
            return "0";
        }

        std::string text =
                formatReal(
                    magnitude,
                    decimalPlaces
                );

        if (approximatelyEqual(phaseRadians, 0.0)) {
            return text;
        }

        const bool negativePhase =
                phaseRadians < 0.0;

        text +=
                negativePhase
                    ? " e^(-i"
                    : " e^(i";

        text +=
                formatRadians(
                    std::abs(phaseRadians),
                    decimalPlaces,
                    false
                );

        text += ")";
        return text;
    }
}
