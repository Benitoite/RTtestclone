////////////////////////////////////////////////////////////////
//
//  this code was taken from http://shibatch.sourceforge.net/
//  Many thanks to the author of original version: Naoki Shibata
//
//  This version contains modifications made by Ingo Weyrich
//
////////////////////////////////////////////////////////////////

#ifndef __SSE2__
#error Please specify -msse2.
#endif

#ifdef __GNUC__
#define INLINE __inline
#else
#define INLINE inline
#endif

#include <cstdint>

#include <x86intrin.h>

using vdouble = __m128d;
using vint = __m128i;
using vmask = __m128i;

using vfloat = __m128;
using vint2 = __m128i;

struct vdouble2 {
    vdouble x;
    vdouble y;
};

namespace
{

//

INLINE vfloat lvf(const float& x)
{
    return _mm_load_ps(&x);
}

INLINE vfloat lvfu(const float& x)
{
    return _mm_loadu_ps(&x);
}

INLINE void stvf(float& x, vfloat y)
{
    _mm_store_ps(&x, y);
}

INLINE void stvfu(float& x, vfloat y)
{
    _mm_storeu_ps(&x, y);
}

#ifdef __AVX__
template<std::uint8_t FP3, std::uint8_t FP2, std::uint8_t FP1, std::uint8_t FP0>
INLINE vfloat permuteps(vfloat a)
{
    return _mm_permute_ps(a, _MM_SHUFFLE(FP3, FP2, FP1, FP0));
}
#else
template<std::uint8_t FP3, std::uint8_t FP2, std::uint8_t FP1, std::uint8_t FP0>
INLINE vfloat permuteps(vfloat a)
{
    return _mm_shuffle_ps(a, a, _MM_SHUFFLE(FP3, FP2, FP1, FP0));
}
#endif

INLINE vfloat lc2vfu(float &a)
{
    // Load 8 floats from a and combine a[0],a[2],a[4] and a[6] into a vector of 4 floats
    vfloat a1 = _mm_loadu_ps( &a );
    vfloat a2 = _mm_loadu_ps( (&a) + 4 );
    return _mm_shuffle_ps(a1,a2,_MM_SHUFFLE( 2,0,2,0 ));
}

constexpr vfloat zerov = {};

INLINE vfloat f2v(float a)
{
    return _mm_set1_ps(a);
}

INLINE vfloat f2v(float a, float b, float c, float d)
{
    return _mm_set_ps(a, b, c, d);
}

INLINE vfloat f2vr(float a, float b, float c, float d)
{
    return _mm_setr_ps(a, b, c, d);
}

INLINE vint vrint_vi_vd(vdouble vd)
{
    return _mm_cvtpd_epi32(vd);
}
INLINE vint vtruncate_vi_vd(vdouble vd)
{
    return _mm_cvttpd_epi32(vd);
}
INLINE vdouble vcast_vd_vi(vint vi)
{
    return _mm_cvtepi32_pd(vi);
}
INLINE vdouble vcast_vd_d(double d)
{
    return _mm_set_pd(d, d);
}
INLINE vint vcast_vi_i(int i)
{
    return _mm_set_epi32(0, 0, i, i);
}

INLINE vmask vreinterpret_vm_vd(vdouble vd)
{
    return (__m128i)vd;
}
INLINE vdouble vreinterpret_vd_vm(vint vm)
{
    return (__m128d)vm;
}

INLINE vmask vreinterpret_vm_vf(vfloat vf)
{
    return (__m128i)vf;
}
INLINE vfloat vreinterpret_vf_vm(vmask vm)
{
    return (__m128)vm;
}

//

INLINE vfloat vcast_vf_f(float f)
{
    return _mm_set_ps(f, f, f, f);
}

// Don't use intrinsics here. Newer gcc versions (>= 4.9, maybe also before 4.9) generate better code when not using intrinsics
// example: vaddf(vmulf(a,b),c) will generate an FMA instruction when build for chips with that feature only when vaddf and vmulf don't use intrinsics
INLINE vfloat vaddf(vfloat x, vfloat y)
{
    return x + y;
}

INLINE vfloat vsubf(vfloat x, vfloat y)
{
    return x - y;
}

INLINE vfloat vmulf(vfloat x, vfloat y)
{
    return x * y;
}

INLINE vfloat vdivf(vfloat x, vfloat y)
{
    return x / y;
}

// Also don't use intrinsic here: Some chips support FMA instructions with 3 and 4 operands
// 3 operands: a = a*b+c, b = a*b+c, c = a*b+c // destination has to be one of a,b,c
// 4 operands: d = a*b+c // destination does not have to be one of a,b,c
// gcc will use the one which fits best when not using intrinsics. With using intrinsics that's not possible
INLINE vfloat vmlaf(vfloat x, vfloat y, vfloat z) {
    return x * y + z;
}

INLINE vfloat vrecf(vfloat x)
{
    return vdivf(vcast_vf_f(1.0f), x);
}

INLINE vfloat vsqrtf(vfloat x)
{
    return _mm_sqrt_ps(x);
}

INLINE vfloat vmaxf(vfloat x, vfloat y)
{
    // _mm_max_ps(x, y) returns y if x is NaN
    // don't change the order of the parameters
    return _mm_max_ps(x, y);
}

INLINE vfloat vminf(vfloat x, vfloat y)
{
    // _mm_min_ps(x, y) returns y if x is NaN
    // don't change the order of the parameters
    return _mm_min_ps(x, y);
}

//

INLINE vdouble vadd(vdouble x, vdouble y)
{
    return _mm_add_pd(x, y);
}

INLINE vdouble vsub(vdouble x, vdouble y)
{
    return _mm_sub_pd(x, y);
}

INLINE vdouble vmul(vdouble x, vdouble y)
{
    return _mm_mul_pd(x, y);
}

INLINE vdouble vdiv(vdouble x, vdouble y)
{
    return _mm_div_pd(x, y);
}

INLINE vdouble vrec(vdouble x)
{
    return _mm_div_pd(_mm_set_pd(1, 1), x);
}

INLINE vdouble vsqrt(vdouble x)
{
    return _mm_sqrt_pd(x);
}

INLINE vdouble vmla(vdouble x, vdouble y, vdouble z)
{
    return vadd(vmul(x, y), z);
}

INLINE vdouble vmax(vdouble x, vdouble y)
{
    return _mm_max_pd(x, y);
}

INLINE vdouble vmin(vdouble x, vdouble y)
{
    return _mm_min_pd(x, y);
}

INLINE vdouble vabs(vdouble d)
{
    return (__m128d)_mm_andnot_pd(_mm_set_pd(-0.0, -0.0), d);
}

INLINE vdouble vneg(vdouble d)
{
    return (__m128d)_mm_xor_pd(_mm_set_pd(-0.0, -0.0), d);
}

//

INLINE vint vaddi(vint x, vint y)
{
    return _mm_add_epi32(x, y);
}

INLINE vint vsubi(vint x, vint y)
{
    return _mm_sub_epi32(x, y);
}

INLINE vint vandi(vint x, vint y)
{
    return _mm_and_si128(x, y);
}

INLINE vint vandnoti(vint x, vint y)
{
    return _mm_andnot_si128(x, y);
}

INLINE vint vori(vint x, vint y)
{
    return _mm_or_si128(x, y);
}

INLINE vint vxori(vint x, vint y)
{
    return _mm_xor_si128(x, y);
}

INLINE vint vslli(vint x, int c)
{
    return _mm_slli_epi32(x, c);
}

INLINE vint vsrli(vint x, int c)
{
    return _mm_srli_epi32(x, c);
}

INLINE vint vsrai(vint x, int c)
{
    return _mm_srai_epi32(x, c);
}

//

INLINE vmask vandm(vmask x, vmask y)
{
    return _mm_and_si128(x, y);
}

INLINE vmask vandnotm(vmask x, vmask y)
{
    return _mm_andnot_si128(x, y);
}

INLINE vmask vorm(vmask x, vmask y)
{
    return _mm_or_si128(x, y);
}

INLINE vmask vxorm(vmask x, vmask y)
{
    return _mm_xor_si128(x, y);
}

INLINE vmask vnotm(vmask x)
{
    return _mm_xor_si128(x, _mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128()));
}

INLINE vmask vmask_eq(vdouble x, vdouble y)
{
    return (__m128i)_mm_cmpeq_pd(x, y);
}

INLINE vmask vmask_neq(vdouble x, vdouble y)
{
    return (__m128i)_mm_cmpneq_pd(x, y);
}

INLINE vmask vmask_lt(vdouble x, vdouble y)
{
    return (__m128i)_mm_cmplt_pd(x, y);
}

INLINE vmask vmask_le(vdouble x, vdouble y)
{
    return (__m128i)_mm_cmple_pd(x, y);
}

INLINE vmask vmask_gt(vdouble x, vdouble y)
{
    return (__m128i)_mm_cmpgt_pd(x, y);
}

INLINE vmask vmask_ge(vdouble x, vdouble y)
{
    return (__m128i)_mm_cmpge_pd(x, y);
}

INLINE vmask vmaskf_eq(vfloat x, vfloat y)
{
    return (__m128i)_mm_cmpeq_ps(x, y);
}

INLINE vmask vmaskf_neq(vfloat x, vfloat y)
{
    return (__m128i)_mm_cmpneq_ps(x, y);
}

INLINE vmask vmaskf_lt(vfloat x, vfloat y)
{
    return (__m128i)_mm_cmplt_ps(x, y);
}

INLINE vmask vmaskf_le(vfloat x, vfloat y)
{
    return (__m128i)_mm_cmple_ps(x, y);
}

INLINE vmask vmaskf_gt(vfloat x, vfloat y)
{
    return (__m128i)_mm_cmpgt_ps(x, y);
}

INLINE vmask vmaskf_ge(vfloat x, vfloat y)
{
    return (__m128i)_mm_cmpge_ps(x, y);
}

INLINE vmask vmaski_eq(vint x, vint y)
{
    __m128 s = (__m128)_mm_cmpeq_epi32(x, y);
    return (__m128i)_mm_shuffle_ps(s, s, _MM_SHUFFLE(1, 1, 0, 0));
}

INLINE vdouble vsel(vmask mask, vdouble x, vdouble y)
{
    return (__m128d)vorm(vandm(mask, (__m128i)x), vandnotm(mask, (__m128i)y));
}

INLINE vint vseli_lt(vdouble d0, vdouble d1, vint x, vint y)
{
    vmask mask = (vmask)_mm_cmpeq_ps(_mm_cvtpd_ps((vdouble)vmask_lt(d0, d1)), _mm_set_ps(0, 0, 0, 0));
    return vori(vandnoti(mask, x), vandi(mask, y));
}

//

INLINE vint2 vcast_vi2_vm(vmask vm)
{
    return (vint2)vm;
}

INLINE vmask vcast_vm_vi2(vint2 vi)
{
    return (vmask)vi;
}

INLINE vint2 vrint_vi2_vf(vfloat vf)
{
    return _mm_cvtps_epi32(vf);
}

INLINE vint2 vtruncate_vi2_vf(vfloat vf)
{
    return _mm_cvttps_epi32(vf);
}

INLINE vfloat vcast_vf_vi2(vint2 vi)
{
    return _mm_cvtepi32_ps(vcast_vm_vi2(vi));
}

INLINE vint2 vcast_vi2_i(int i)
{
    return _mm_set_epi32(i, i, i, i);
}

INLINE vint2 vaddi2(vint2 x, vint2 y)
{
    return vaddi(x, y);
}

INLINE vint2 vsubi2(vint2 x, vint2 y)
{
    return vsubi(x, y);
}

INLINE vint2 vandi2(vint2 x, vint2 y)
{
    return vandi(x, y);
}

INLINE vint2 vandnoti2(vint2 x, vint2 y)
{
    return vandnoti(x, y);
}

INLINE vint2 vori2(vint2 x, vint2 y)
{
    return vori(x, y);
}

INLINE vint2 vxori2(vint2 x, vint2 y)
{
    return vxori(x, y);
}

INLINE vint2 vslli2(vint2 x, int c)
{
    return vslli(x, c);
}

INLINE vint2 vsrli2(vint2 x, int c)
{
    return vsrli(x, c);
}

INLINE vint2 vsrai2(vint2 x, int c)
{
    return vsrai(x, c);
}

INLINE vmask vmaski2_eq(vint2 x, vint2 y)
{
    return _mm_cmpeq_epi32(x, y);
}

INLINE vint2 vseli2(vmask m, vint2 x, vint2 y)
{
    return vorm(vandm(m, x), vandnotm(m, y));
}

//

INLINE double vcast_d_vd(vdouble v)
{
    double s[2];
    _mm_storeu_pd(s, v);
    return s[0];
}

INLINE float vcast_f_vf(vfloat v)
{
    float s[4];
    _mm_storeu_ps(s, v);
    return s[0];
}

INLINE vmask vsignbit(vdouble d)
{
    return _mm_and_si128((__m128i)d, _mm_set_epi32(0x80000000, 0x0, 0x80000000, 0x0));
}

INLINE vdouble vsign(vdouble d)
{
    return (__m128d)_mm_or_si128((__m128i)_mm_set_pd(1, 1), _mm_and_si128((__m128i)d, _mm_set_epi32(0x80000000, 0x0, 0x80000000, 0x0)));
}

INLINE vdouble vmulsign(vdouble x, vdouble y)
{
    return (__m128d)vxori((__m128i)x, vsignbit(y));
}

INLINE vmask vmask_isinf(vdouble d)
{
    return (vmask)_mm_cmpeq_pd(vabs(d), _mm_set_pd(INFINITY, INFINITY));
}

INLINE vmask vmask_ispinf(vdouble d)
{
    return (vmask)_mm_cmpeq_pd(d, _mm_set_pd(INFINITY, INFINITY));
}

INLINE vmask vmask_isminf(vdouble d)
{
    return (vmask)_mm_cmpeq_pd(d, _mm_set_pd(-INFINITY, -INFINITY));
}

INLINE vmask vmask_isnan(vdouble d)
{
    return (vmask)_mm_cmpneq_pd(d, d);
}

INLINE vdouble visinf(vdouble d)
{
    return (__m128d)_mm_and_si128(vmask_isinf(d), _mm_or_si128(vsignbit(d), (__m128i)_mm_set_pd(1, 1)));
}

INLINE vdouble visinf2(vdouble d, vdouble m)
{
    return (__m128d)_mm_and_si128(vmask_isinf(d), _mm_or_si128(vsignbit(d), (__m128i)m));
}

//

INLINE vdouble vpow2i(vint q)
{
    q = _mm_add_epi32(_mm_set_epi32(0x0, 0x0, 0x3ff, 0x3ff), q);
    q = (__m128i)_mm_shuffle_ps((__m128)q, (__m128)q, _MM_SHUFFLE(1, 3, 0, 3));
    return (__m128d)_mm_slli_epi32(q, 20);
}

INLINE vdouble vldexp(vdouble x, vint q)
{
    vint m = _mm_srai_epi32(q, 31);
    m = _mm_slli_epi32(_mm_sub_epi32(_mm_srai_epi32(_mm_add_epi32(m, q), 9), m), 7);
    q = _mm_sub_epi32(q, _mm_slli_epi32(m, 2));
    vdouble y = vpow2i(m);
    return vmul(vmul(vmul(vmul(vmul(x, y), y), y), y), vpow2i(q));
}

INLINE vint vilogbp1(vdouble d)
{
    vint m = vmask_lt(d, vcast_vd_d(4.9090934652977266E-91));
    d = vsel(m, vmul(vcast_vd_d(2.037035976334486E90), d), d);
    __m128i q = _mm_and_si128((__m128i)d, _mm_set_epi32(((1 << 12) - 1) << 20, 0, ((1 << 12) - 1) << 20, 0));
    q = _mm_srli_epi32(q, 20);
    q = vorm(vandm   (m, _mm_sub_epi32(q, _mm_set_epi32(300 + 0x3fe, 0, 300 + 0x3fe, 0))),
             vandnotm(m, _mm_sub_epi32(q, _mm_set_epi32(      0x3fe, 0,       0x3fe, 0))));
    q = (__m128i)_mm_shuffle_ps((__m128)q, (__m128)q, _MM_SHUFFLE(0, 0, 3, 1));
    return q;
}

INLINE vdouble vupper(vdouble d)
{
    return (__m128d)_mm_and_si128((__m128i)d, _mm_set_epi32(0xffffffff, 0xf8000000, 0xffffffff, 0xf8000000));
}

//

INLINE vdouble2 dd(vdouble h, vdouble l)
{
    vdouble2 ret = {h, l};
    return ret;
}

INLINE vdouble2 vsel2(vmask mask, vdouble2 x, vdouble2 y)
{
    return dd((__m128d)vorm(vandm(mask, (__m128i)x.x), vandnotm(mask, (__m128i)y.x)),
              (__m128d)vorm(vandm(mask, (__m128i)x.y), vandnotm(mask, (__m128i)y.y)));
}

INLINE vdouble2 abs_d(vdouble2 x)
{
    return dd((__m128d)_mm_xor_pd(_mm_and_pd(_mm_set_pd(-0.0, -0.0), x.x), x.x),
              (__m128d)_mm_xor_pd(_mm_and_pd(_mm_set_pd(-0.0, -0.0), x.x), x.y));
}

}
