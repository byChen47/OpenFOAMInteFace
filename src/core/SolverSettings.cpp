#include "core/SolverSettings.h"

#include <cmath>
#include <utility>

namespace ofgui::core {

const std::string& SolverSettings::application() const noexcept
{
    return application_;
}

void SolverSettings::setApplication(std::string application)
{
    application_ = std::move(application);
}

Real SolverSettings::startTime() const noexcept
{
    return startTime_;
}

void SolverSettings::setStartTime(Real startTime) noexcept
{
    startTime_ = startTime;
}

Real SolverSettings::endTime() const noexcept
{
    return endTime_;
}

void SolverSettings::setEndTime(Real endTime) noexcept
{
    endTime_ = endTime;
}

Real SolverSettings::deltaT() const noexcept
{
    return deltaT_;
}

void SolverSettings::setDeltaT(Real deltaT) noexcept
{
    deltaT_ = deltaT;
}

int SolverSettings::writeInterval() const noexcept
{
    return writeInterval_;
}

void SolverSettings::setWriteInterval(int writeInterval) noexcept
{
    writeInterval_ = writeInterval;
}

bool SolverSettings::hasValidTimeRange() const noexcept
{
    return std::isfinite(startTime_) && std::isfinite(endTime_) && endTime_ > startTime_;
}

bool SolverSettings::isValid() const noexcept
{
    return hasValidTimeRange() && std::isfinite(deltaT_) && deltaT_ > 0.0 && writeInterval_ >= 1;
}

} // namespace ofgui::core
