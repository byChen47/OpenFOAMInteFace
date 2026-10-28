#pragma once

#include "core/CaseSourceInfo.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace ofgui::io {

/// Minimal streaming SHA-256 (FIPS 180-4).
///
/// `docs/CASE_IMPORT_SPEC.md` section 6.9 and `docs/DECISIONS.md` D-025 require a
/// hash per imported file so a later export can prove the source did not change.
/// The project does not link OpenSSL or another crypto library, so the ~120 lines
/// live here; only the digest matters, not constant-time behaviour.
class Sha256Hasher
{
public:
    Sha256Hasher();

    void update(std::string_view bytes);
    void update(const char* data, std::size_t size);

    /// Finishes the digest; the hasher must not be reused afterwards.
    [[nodiscard]] core::Sha256Digest finish();

    /// One shot helpers.
    [[nodiscard]] static core::Sha256Digest hash(std::string_view bytes);
    [[nodiscard]] static std::string hex(std::string_view bytes);

private:
    void transform(const std::uint8_t* block);

    std::array<std::uint32_t, 8> state_{};
    std::array<std::uint8_t, 64> buffer_{};
    std::size_t bufferSize_ = 0;
    std::uint64_t totalBytes_ = 0;
};

} // namespace ofgui::io
