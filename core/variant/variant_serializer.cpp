#include "variant_serializer.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace nle {
namespace {

constexpr std::array<std::byte, 4> Magic{
    std::byte{'N'}, std::byte{'L'}, std::byte{'E'}, std::byte{'V'}
};
constexpr std::uint32_t FormatVersion = 1;
constexpr std::uint32_t MaxPayloadSize = 16U * 1024U * 1024U;
constexpr std::size_t HeaderSize = 12;
constexpr std::size_t ChecksumSize = 8;

template <typename T>
void append_integer(std::vector<std::byte>& output, const T value) {
    using U = std::make_unsigned_t<T>;
    const U bits = static_cast<U>(value);
    for (std::size_t index = 0; index < sizeof(T); ++index) {
        output.push_back(static_cast<std::byte>((bits >> (index * 8U)) & U{0xff}));
    }
}

template <typename T>
bool read_integer(
    const std::span<const std::byte> input,
    std::size_t& offset,
    T& value
) noexcept {
    if (offset > input.size() || input.size() - offset < sizeof(T)) {
        return false;
    }

    using U = std::make_unsigned_t<T>;
    U bits = 0;
    for (std::size_t index = 0; index < sizeof(T); ++index) {
        bits |= static_cast<U>(std::to_integer<unsigned char>(input[offset++]))
                << (index * 8U);
    }

    if constexpr (std::is_signed_v<T>) {
        value = std::bit_cast<T>(bits);
    } else {
        value = bits;
    }
    return true;
}

using RealBits = std::conditional_t<
    sizeof(real_t) == sizeof(std::uint32_t),
    std::uint32_t,
    std::uint64_t
>;

void append_real(std::vector<std::byte>& output, const real_t value) {
    append_integer(output, std::bit_cast<RealBits>(value));
}

bool read_real(
    const std::span<const std::byte> input,
    std::size_t& offset,
    real_t& value
) noexcept {
    RealBits bits = 0;
    if (!read_integer(input, offset, bits)) {
        return false;
    }
    value = std::bit_cast<real_t>(bits);
    return true;
}

std::uint64_t hash_bytes(const std::span<const std::byte> bytes) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const std::byte byte : bytes) {
        hash ^= std::to_integer<unsigned char>(byte);
        hash *= 1099511628211ULL;
    }
    return hash;
}

bool encode_payload(const Variant& value, std::vector<std::byte>& payload) {
    payload.push_back(static_cast<std::byte>(value.get_type()));
    switch (value.get_type()) {
        case Variant::Type::Nil:
            return true;
        case Variant::Type::Bool:
            payload.push_back(*value.get_if<bool>() ? std::byte{1} : std::byte{0});
            return true;
        case Variant::Type::Int:
            append_integer(payload, *value.get_if<std::int64_t>());
            return true;
        case Variant::Type::Float:
            append_real(payload, *value.get_if<real_t>());
            return true;
        case Variant::Type::String: {
            const std::string& string = *value.get_if<std::string>();
            if (string.size() > MaxPayloadSize) {
                return false;
            }
            append_integer(payload, static_cast<std::uint32_t>(string.size()));
            for (const unsigned char character : string) {
                payload.push_back(static_cast<std::byte>(character));
            }
            return true;
        }
        case Variant::Type::Vector2: {
            const Vector2 vector = *value.get_if<Vector2>();
            append_real(payload, vector.x);
            append_real(payload, vector.y);
            return true;
        }
        case Variant::Type::Vector3: {
            const Vector3 vector = *value.get_if<Vector3>();
            append_real(payload, vector.x);
            append_real(payload, vector.y);
            append_real(payload, vector.z);
            return true;
        }
        case Variant::Type::Vector4: {
            const Vector4 vector = *value.get_if<Vector4>();
            append_real(payload, vector.x);
            append_real(payload, vector.y);
            append_real(payload, vector.z);
            append_real(payload, vector.w);
            return true;
        }
        default:
            return false;
    }
}

bool decode_payload(
    const std::span<const std::byte> payload,
    Variant& value
) {
    if (payload.empty()) {
        return false;
    }

    std::size_t offset = 1;
    const auto type = static_cast<Variant::Type>(
        std::to_integer<std::uint8_t>(payload.front())
    );

    switch (type) {
        case Variant::Type::Nil:
            if (offset != payload.size()) return false;
            value = Variant{};
            return true;
        case Variant::Type::Bool: {
            if (offset >= payload.size()) return false;
            const std::uint8_t boolean = std::to_integer<std::uint8_t>(payload[offset++]);
            if (boolean > 1 || offset != payload.size()) return false;
            value = Variant(boolean != 0);
            return true;
        }
        case Variant::Type::Int: {
            std::int64_t integer = 0;
            if (!read_integer(payload, offset, integer) || offset != payload.size()) {
                return false;
            }
            value = Variant(integer);
            return true;
        }
        case Variant::Type::Float: {
            real_t number = 0;
            if (!read_real(payload, offset, number) || offset != payload.size()) {
                return false;
            }
            value = Variant(number);
            return true;
        }
        case Variant::Type::String: {
            std::uint32_t length = 0;
            if (!read_integer(payload, offset, length) ||
                length > MaxPayloadSize || offset > payload.size() ||
                payload.size() - offset != length) {
                return false;
            }
            value = Variant(std::string(
                reinterpret_cast<const char*>(payload.data() + offset),
                length
            ));
            return true;
        }
        case Variant::Type::Vector2:
        case Variant::Type::Vector3:
        case Variant::Type::Vector4: {
            real_t components[4]{};
            const std::size_t count = type == Variant::Type::Vector2 ? 2 :
                                      type == Variant::Type::Vector3 ? 3 : 4;
            for (std::size_t index = 0; index < count; ++index) {
                if (!read_real(payload, offset, components[index])) {
                    return false;
                }
            }
            if (offset != payload.size()) return false;
            if (count == 2) value = Variant(Vector2{components[0], components[1]});
            else if (count == 3) value = Variant(Vector3{components[0], components[1], components[2]});
            else value = Variant(Vector4{components[0], components[1], components[2], components[3]});
            return true;
        }
        default:
            return false;
    }
}

} // namespace

ErrorCode VariantSerializer::serialize(
    const Variant& value,
    std::vector<std::byte>& output
) {
    output.clear();
    try {
        std::vector<std::byte> payload;
        if (!encode_payload(value, payload) || payload.size() > MaxPayloadSize) {
            return ErrorCode::InvalidParameter;
        }

        output.insert(output.end(), Magic.begin(), Magic.end());
        append_integer(output, FormatVersion);
        append_integer(output, static_cast<std::uint32_t>(payload.size()));
        output.insert(output.end(), payload.begin(), payload.end());
        append_integer(output, hash_bytes(output));
        return ErrorCode::Ok;
    } catch (const std::bad_alloc&) {
        output.clear();
        return ErrorCode::OutOfMemory;
    } catch (const std::length_error&) {
        output.clear();
        return ErrorCode::OutOfMemory;
    }
}

ErrorCode VariantSerializer::deserialize(
    const std::span<const std::byte> input,
    Variant& value
) {
    if (input.size() < HeaderSize + 1 + ChecksumSize ||
        input.size() > static_cast<std::size_t>(MaxPayloadSize) + HeaderSize + ChecksumSize ||
        !std::equal(Magic.begin(), Magic.end(), input.begin())) {
        return ErrorCode::InvalidParameter;
    }

    const std::size_t checksum_offset = input.size() - ChecksumSize;
    std::size_t checksum_cursor = checksum_offset;
    std::uint64_t stored_checksum = 0;
    if (!read_integer(input, checksum_cursor, stored_checksum) ||
        hash_bytes(input.first(checksum_offset)) != stored_checksum) {
        return ErrorCode::InvalidParameter;
    }

    std::size_t offset = Magic.size();
    std::uint32_t version = 0;
    std::uint32_t payload_size = 0;
    if (!read_integer(input, offset, version) || version != FormatVersion ||
        !read_integer(input, offset, payload_size) ||
        payload_size > MaxPayloadSize || offset > checksum_offset ||
        checksum_offset - offset != payload_size) {
        return ErrorCode::InvalidParameter;
    }

    try {
        Variant decoded;
        if (!decode_payload(input.subspan(offset, payload_size), decoded)) {
            return ErrorCode::InvalidParameter;
        }
        value = std::move(decoded);
        return ErrorCode::Ok;
    } catch (const std::bad_alloc&) {
        return ErrorCode::OutOfMemory;
    } catch (const std::length_error&) {
        return ErrorCode::OutOfMemory;
    }
}

} // namespace nle
