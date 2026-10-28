#include "core/BoundaryPatch.h"

#include <algorithm>
#include <utility>

namespace ofgui::core {
namespace {

constexpr bool isAsciiLetter(char character) noexcept
{
    return (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
}

constexpr bool isAsciiDigit(char character) noexcept
{
    return character >= '0' && character <= '9';
}

} // namespace

std::string_view patchTypeName(PatchType type) noexcept
{
    switch (type)
    {
    case PatchType::Patch:
        return "patch";
    case PatchType::Wall:
        return "wall";
    case PatchType::Empty:
        return "empty";
    case PatchType::Symmetry:
        return "symmetry";
    case PatchType::SymmetryPlane:
        return "symmetryPlane";
    }

    return "patch";
}

std::optional<PatchType> patchTypeFromName(std::string_view name) noexcept
{
    if (name == "patch")
    {
        return PatchType::Patch;
    }
    if (name == "wall")
    {
        return PatchType::Wall;
    }
    if (name == "empty")
    {
        return PatchType::Empty;
    }
    if (name == "symmetry")
    {
        return PatchType::Symmetry;
    }
    if (name == "symmetryPlane")
    {
        return PatchType::SymmetryPlane;
    }

    return std::nullopt;
}

BoundaryPatch::BoundaryPatch(std::string name, PatchType type)
    : name_(std::move(name))
    , type_(type)
{
}

BoundaryPatch BoundaryPatch::create(std::string name, PatchType type)
{
    BoundaryPatch patch(std::move(name), type);
    patch.id_ = ObjectId::create();
    return patch;
}

const ObjectId& BoundaryPatch::id() const noexcept
{
    return id_;
}

void BoundaryPatch::setId(const ObjectId& id) noexcept
{
    id_ = id;
}

const std::string& BoundaryPatch::name() const noexcept
{
    return name_;
}

void BoundaryPatch::setName(std::string name)
{
    name_ = std::move(name);
}

PatchType BoundaryPatch::type() const noexcept
{
    return type_;
}

void BoundaryPatch::setType(PatchType type) noexcept
{
    type_ = type;
}

const std::set<FaceId>& BoundaryPatch::faces() const noexcept
{
    return faces_;
}

bool BoundaryPatch::addFace(FaceId face)
{
    return faces_.insert(face).second;
}

bool BoundaryPatch::removeFace(FaceId face)
{
    return faces_.erase(face) > 0;
}

void BoundaryPatch::setFaces(std::set<FaceId> faces)
{
    faces_ = std::move(faces);
}

bool BoundaryPatch::hasFace(FaceId face) const noexcept
{
    return faces_.count(face) > 0;
}

std::size_t BoundaryPatch::faceCount() const noexcept
{
    return faces_.size();
}

const std::array<Real, 3>& BoundaryPatch::color() const noexcept
{
    return color_;
}

void BoundaryPatch::setColor(const std::array<Real, 3>& color) noexcept
{
    color_ = color;
}

bool BoundaryPatch::isVisible() const noexcept
{
    return visible_;
}

void BoundaryPatch::setVisible(bool visible) noexcept
{
    visible_ = visible;
}

bool BoundaryPatch::isValidName(std::string_view name) noexcept
{
    if (name.empty() || !isAsciiLetter(name.front()))
    {
        return false;
    }

    return std::all_of(name.begin() + 1, name.end(), [](char character) {
        return isAsciiLetter(character) || isAsciiDigit(character) || character == '_';
    });
}

} // namespace ofgui::core
