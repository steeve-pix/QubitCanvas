#include "quantum_sim/math/ComplexVector.hpp"

#include <utility>

namespace quantum_sim::math {
    ComplexVector::ComplexVector(std::vector<Complex> values)
        : value_{std::move(values)} {
    }

    std::size_t ComplexVector::size() const noexcept {
        return value_.size();
    }

    const Complex &ComplexVector::at(std::size_t index) const {
        return value_.at(index);
    }
}
