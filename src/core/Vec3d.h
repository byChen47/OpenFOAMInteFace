#pragma once

#include "core/Basics.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace ofgui::core {

/// Three dimensional value type shared by the domain, mesh and field models.
///
/// Header only by design: a plain value with component-wise arithmetic and no
/// invariants beyond the explicit `isFinite()` / `isApprox()` helpers. It never
/// depends on Qt or VTK so it can be reused by `core`, `io` and the tests.
struct Vec3d
{
    Real x = 0.0;
    Real y = 0.0;
    Real z = 0.0;

    constexpr Vec3d() noexcept = default;

    constexpr Vec3d(Real xValue, Real yValue, Real zValue) noexcept
        : x(xValue)
        , y(yValue)
        , z(zValue)
    {
    }

    [[nodiscard]] static constexpr Vec3d zero() noexcept
    {
        return Vec3d{0.0, 0.0, 0.0};
    }

    [[nodiscard]] static constexpr Vec3d filled(Real value) noexcept
    {
        return Vec3d{value, value, value};
    }

    [[nodiscard]] constexpr Vec3d operator+(const Vec3d& other) const noexcept
    {
        return Vec3d{x + other.x, y + other.y, z + other.z};
    }

    [[nodiscard]] constexpr Vec3d operator-(const Vec3d& other) const noexcept
    {
        return Vec3d{x - other.x, y - other.y, z - other.z};
    }

    [[nodiscard]] constexpr Vec3d operator-() const noexcept
    {
        return Vec3d{-x, -y, -z};
    }

    [[nodiscard]] constexpr Vec3d operator*(Real factor) const noexcept
    {
        return Vec3d{x * factor, y * factor, z * factor};
    }

    /// Precondition: `divisor != 0`. Callers validate divisors before scaling.
    [[nodiscard]] constexpr Vec3d operator/(Real divisor) const noexcept
    {
        return Vec3d{x / divisor, y / divisor, z / divisor};
    }

    constexpr Vec3d& operator+=(const Vec3d& other) noexcept
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    constexpr Vec3d& operator-=(const Vec3d& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    constexpr Vec3d& operator*=(Real factor) noexcept
    {
        x *= factor;
        y *= factor;
        z *= factor;
        return *this;
    }

    constexpr Vec3d& operator/=(Real divisor) noexcept
    {
        x /= divisor;
        y /= divisor;
        z /= divisor;
        return *this;
    }

    /// Component access by axis (0 = x, 1 = y, 2 = z).
    /// @throws std::out_of_range when `axis` is not 0, 1 or 2.
    [[nodiscard]] constexpr Real component(std::size_t axis) const
    {
        return asArray().at(axis);
    }

    [[nodiscard]] constexpr std::array<Real, 3> asArray() const noexcept
    {
        return {x, y, z};
    }

    [[nodiscard]] constexpr Real dot(const Vec3d& other) const noexcept
    {
        return x * other.x + y * other.y + z * other.z;
    }

    [[nodiscard]] constexpr Real squaredLength() const noexcept
    {
        return dot(*this);
    }

    [[nodiscard]] Real length() const noexcept
    {
        return std::sqrt(squaredLength());
    }

    /// Unit vector with the same direction. The zero vector stays zero.
    [[nodiscard]] Vec3d normalized() const noexcept
    {
        const Real squared = squaredLength();
        if (!(squared > 0.0))
        {
            return zero();
        }

        const Real inverse = 1.0 / std::sqrt(squared);
        return Vec3d{x * inverse, y * inverse, z * inverse};
    }

    [[nodiscard]] bool isFinite() const noexcept
    {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
    }

    /// Component-wise comparison with absolute plus relative tolerance.
    [[nodiscard]] bool isApprox(const Vec3d& other, Real tolerance = kRealTolerance) const noexcept
    {
        return closeEnough(x, other.x, tolerance) && closeEnough(y, other.y, tolerance)
               && closeEnough(z, other.z, tolerance);
    }

private:
    [[nodiscard]] static bool closeEnough(Real lhs, Real rhs, Real tolerance) noexcept
    {
        const Real scale = std::max<Real>(1.0, std::max(std::abs(lhs), std::abs(rhs)));
        return std::abs(lhs - rhs) <= tolerance * scale;
    }
};

[[nodiscard]] constexpr Vec3d componentwiseMin(const Vec3d& lhs, const Vec3d& rhs) noexcept
{
    return Vec3d{
        lhs.x < rhs.x ? lhs.x : rhs.x,
        lhs.y < rhs.y ? lhs.y : rhs.y,
        lhs.z < rhs.z ? lhs.z : rhs.z};
}

[[nodiscard]] constexpr Vec3d componentwiseMax(const Vec3d& lhs, const Vec3d& rhs) noexcept
{
    return Vec3d{
        lhs.x > rhs.x ? lhs.x : rhs.x,
        lhs.y > rhs.y ? lhs.y : rhs.y,
        lhs.z > rhs.z ? lhs.z : rhs.z};
}

} // namespace ofgui::core
