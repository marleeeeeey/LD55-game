#pragma once
#include <box2d/box2d.h>
#include <entt/entt.hpp>
#include <my_cpp_utils/logger.h>

template <>
struct fmt::formatter<entt::entity> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const entt::entity& entity, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "{}", static_cast<uint32_t>(entity));
    }
};

template <>
struct fmt::formatter<b2Vec2> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const b2Vec2& vec, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "({}, {})", vec.x, vec.y);
    }
};

template <>
struct fmt::formatter<glm::vec2> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const glm::vec2& vec, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "({}, {})", vec.x, vec.y);
    }
};

template <>
struct fmt::formatter<std::filesystem::path> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const std::filesystem::path& path, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "{}", path.string());
    }
};