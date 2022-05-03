//
//  Math.hpp
//  CppMetalTesting
//
//  Created by Johan Solbakken on 03/05/2022.
//

#ifndef Math_hpp
#define Math_hpp

#include <simd/simd.h>

namespace math
{
    simd::float3 add( const simd::float3& a, const simd::float3& b );
    simd_float4x4 makeIdentity();
    simd::float4x4 makePerspective(float fovRadians, float aspect, float znear, float zfar);
    simd::float4x4 makeXRotate( float angleRadians );
    simd::float4x4 makeYRotate( float angleRadians );
    simd::float4x4 makeZRotate( float angleRadians );
    simd::float4x4 makeTranslate( const simd::float3& v );
    simd::float4x4 makeScale( const simd::float3& v );
    simd::float3x3 discardTranslation( const simd::float4x4& m );
}

#endif /* Math_hpp */
