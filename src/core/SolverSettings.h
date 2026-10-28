#pragma once

#include "core/Basics.h"

#include <string>

namespace ofgui::core {

/// The `system/controlDict` values the first version edits: solver application,
/// time control and write interval.
///
/// Transport/turbulence dictionaries have their own models; keeping them out of
/// this class avoids a god object and lets the exporter pick fields per writer.
class SolverSettings
{
public:
    SolverSettings() = default;

    [[nodiscard]] const std::string& application() const noexcept;
    void setApplication(std::string application);

    [[nodiscard]] Real startTime() const noexcept;
    void setStartTime(Real startTime) noexcept;

    [[nodiscard]] Real endTime() const noexcept;
    void setEndTime(Real endTime) noexcept;

    [[nodiscard]] Real deltaT() const noexcept;
    void setDeltaT(Real deltaT) noexcept;

    [[nodiscard]] int writeInterval() const noexcept;
    void setWriteInterval(int writeInterval) noexcept;

    /// Finite times with `endTime > startTime`.
    [[nodiscard]] bool hasValidTimeRange() const noexcept;

    /// Time range plus `deltaT > 0` and `writeInterval >= 1`.
    [[nodiscard]] bool isValid() const noexcept;

private:
    std::string application_;
    Real startTime_ = 0.0;
    Real endTime_ = 1.0;
    Real deltaT_ = 0.001;
    int writeInterval_ = 100;
};

} // namespace ofgui::core
