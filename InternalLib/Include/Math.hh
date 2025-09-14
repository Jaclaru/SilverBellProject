#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "InternalLibMarco.hh"

#include <cstdint>

namespace SilverBell::Math
{

    // 注意:    Eigen的矩阵是行优先 row
    //          glm的矩阵是列优先 column

    typedef Eigen::Matrix2f Mat2;
    typedef Eigen::Matrix3f Mat3;
    typedef Eigen::Matrix4f Mat4;

    typedef Eigen::Vector2f Vec2;
    typedef Eigen::Vector3f Vec3;
    typedef Eigen::Vector4f Vec4;

    typedef Eigen::Vector2i IVec2;
    typedef Eigen::Vector3i IVec3;
    typedef Eigen::Vector4i IVec4;

    typedef Eigen::Vector2<uint32_t> UVec2;
    typedef Eigen::Vector3<uint32_t> UVec3;
    typedef Eigen::Vector4<uint32_t> UVec4;

    typedef Eigen::Quaternion<float> Quaternion;

    constexpr double PI = 3.14159265358979323846;

    __FORCEINLINE float ToRadians(const float Angle)  { return Angle * PI / 180.0f; }
    __FORCEINLINE Vec2  ToRadians(const Vec2& Angle)  { return Angle * PI / 180.0f; }
    __FORCEINLINE Vec3  ToRadians(const Vec3& Angle)  { return Angle * PI / 180.0f; }
    __FORCEINLINE Vec4  ToRadians(const Vec4& Angle)  { return Angle * PI / 180.0f; }

    __FORCEINLINE float ToAngle(const float  Radians) { return Radians * 180.0f / PI; }
    __FORCEINLINE Vec2  ToAngle(const Vec2&  Radians) { return Radians * 180.0f / PI; }
    __FORCEINLINE Vec3  ToAngle(const Vec3&  Radians) { return Radians * 180.0f / PI; }
    __FORCEINLINE Vec4  ToAngle(const Vec4&  Radians) { return Radians * 180.0f / PI; }

    __FORCEINLINE uint32_t Align(uint32_t value, uint32_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    //UE5 MicrosoftPlatformMath.h

    __FORCEINLINE bool IsNaN(float A) { return _isnan(A) != 0; }
    __FORCEINLINE bool IsNaN(double A) { return _isnan(A) != 0; }
    __FORCEINLINE bool IsFinite(float A) { return _finite(A) != 0; }
    __FORCEINLINE bool IsFinite(double A) { return _finite(A) != 0; }

    __FORCEINLINE uint64_t CountLeadingZeros64(uint64_t Value)
    {
        //https://godbolt.org/z/Ejh5G4vPK	
        // return 64 if value if was 0
        unsigned long BitIndex;
        if (!_BitScanReverse64(&BitIndex, Value)) BitIndex = -1;
        return 63 - BitIndex;
    }

    __FORCEINLINE uint64_t CountTrailingZeros64(uint64_t Value)
    {
        // return 64 if Value is 0
        unsigned long BitIndex;	// 0-based, where the LSB is 0 and MSB is 63
        return _BitScanForward64(&BitIndex, Value) ? BitIndex : 64;
    }

    __FORCEINLINE uint32_t CountLeadingZeros(uint32_t Value)
    {
        // return 32 if value is zero
        unsigned long BitIndex;
        _BitScanReverse64(&BitIndex, uint64_t(Value) * 2 + 1);
        return 32 - BitIndex;
    }

    __FORCEINLINE uint64_t CeilLogTwo64(uint64_t Arg)
    {
        // if Arg is 0, change it to 1 so that we return 0
        Arg = Arg ? Arg : 1;
        return 64 - CountLeadingZeros64(Arg - 1);
    }

    __FORCEINLINE uint32_t FloorLog2(uint32_t Value)
    {
        // Use BSR to return the log2 of the integer
        // return 0 if value is 0
        unsigned long BitIndex;
        return _BitScanReverse(&BitIndex, Value) ? BitIndex : 0;
    }
    __FORCEINLINE uint8_t CountLeadingZeros8(uint8_t Value)
    {
        unsigned long BitIndex;
        _BitScanReverse(&BitIndex, uint32_t(Value) * 2 + 1);
        return uint8_t(8 - BitIndex);
    }

    __FORCEINLINE uint32_t CountTrailingZeros(uint32_t Value)
    {
        // return 32 if value was 0
        unsigned long BitIndex;	// 0-based, where the LSB is 0 and MSB is 31
        return _BitScanForward(&BitIndex, Value) ? BitIndex : 32;
    }

    __FORCEINLINE uint32_t CeilLogTwo(uint32_t Arg)
    {
        // if Arg is 0, change it to 1 so that we return 0
        Arg = Arg ? Arg : 1;
        return 32 - CountLeadingZeros(Arg - 1);
    }

    __FORCEINLINE uint32_t RoundUpToPowerOfTwo(uint32_t Arg)
    {
        return 1 << CeilLogTwo(Arg);
    }

    __FORCEINLINE uint64_t RoundUpToPowerOfTwo64(uint64_t Arg)
    {
        return uint64_t(1) << CeilLogTwo64(Arg);
    }

    __FORCEINLINE uint64_t FloorLog2_64(uint64_t Value)
    {
        unsigned long BitIndex;
        return _BitScanReverse64(&BitIndex, Value) ? BitIndex : 0;
    }

    Vec3 ClampEulerAngle(Vec3 Angle);

    INTERNALLIB_API Vec3 ToEulerAngle(const Quaternion& Quad);

    INTERNALLIB_API Quaternion ToQuaternion(const Vec3& EulerAngle);

    INTERNALLIB_API Mat4 LookAt(const Vec3& Eye, const Vec3& Center, const Vec3& Up);

    INTERNALLIB_API Mat4 Perspective(float Fov, float Aspect, float Near, float Far);

    Mat4 Ortho(float left, float right, float bottom, float top, float near, float far);

    void Mat3x4(Mat4 mat, float* newMat);
}