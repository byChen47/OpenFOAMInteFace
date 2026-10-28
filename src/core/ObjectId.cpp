#include "core/ObjectId.h"

#include <array>
#include <atomic>
#include <charconv>
#include <random>

namespace ofgui::core {
namespace {

constexpr std::size_t kHexDigitCount = 16;

std::uint64_t makeRandomSeed() noexcept
{
    try
    {
        std::random_device device;
        const auto high = static_cast<std::uint64_t>(device()) << 32U;
        const auto low = static_cast<std::uint64_t>(device());
        return (high ^ low) | 1U;
    }
    catch (...)
    {
        // Deterministic fallback: ids stay unique inside one process because of
        // the atomic counter below.
        return 0x0F6F2C4B9A17D3E1ULL;
    }
}

} // namespace

ObjectId ObjectId::create()
{
    static const std::uint64_t seed = makeRandomSeed();
    static std::atomic<std::uint64_t> counter{0};
    return ObjectId{seed + counter.fetch_add(1, std::memory_order_relaxed)};
}

std::optional<ObjectId> ObjectId::fromString(std::string_view text)
{
    if (text.size() >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
    {
        text.remove_prefix(2);
    }

    if (text.empty() || text.size() > kHexDigitCount)
    {
        return std::nullopt;
    }

    std::uint64_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value, 16);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || value == 0)
    {
        return std::nullopt;
    }

    return ObjectId{value};
}

std::string ObjectId::toString() const
{
    std::array<char, kHexDigitCount> digits{};
    const auto result = std::to_chars(digits.data(), digits.data() + digits.size(), value_, 16);
    std::string text(digits.data(), result.ptr);

    if (text.size() < kHexDigitCount)
    {
        text.insert(0, kHexDigitCount - text.size(), '0');
    }

    return text;
}

} // namespace ofgui::core
