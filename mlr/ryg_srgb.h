/*
 * ryg's sse srgb conversion kit
 */

#ifndef __RYG_SRGB_H
#define __RYG_SRGB_H

typedef unsigned int uint;
typedef unsigned char uint8;

union FP32
{
	uint u;
	float f;
	struct {
		uint Mantissa : 23;
		uint Exponent : 8;
		uint Sign : 1;
	};
};


static float srgb8_to_float( uint8 val )
{
	// There's just 256 different input valus - just use a table.
	static float table[256];
	static bool init;
	if ( !init ) {
		init = true;
		for (int i=0; i < 256; i++) {
			
			// Matches the conversion mandated by D3D10 spec
			float c = (float) i * (1.0f / 255.0f);
			if (c <= 0.04045f) {
				table[i] = c / 12.92f;
			} else {
				table[i] = pow((c + 0.055f) / 1.055f, 2.4f);
			}
		
		}
		
	}
 
	return table[val];

}


static const uint fp32_to_srgb8_tab3[64] = {
    0x0b0f01cb, 0x0bf401ae, 0x0ccb0195, 0x0d950180, 0x0e56016e, 0x0f0d015e, 0x0fbc0150, 0x10630143,
    0x11070264, 0x1238023e, 0x1357021d, 0x14660201, 0x156601e9, 0x165a01d3, 0x174401c0, 0x182401af,
    0x18fe0331, 0x1a9602fe, 0x1c1502d2, 0x1d7e02ad, 0x1ed4028d, 0x201a0270, 0x21520256, 0x227d0240,
    0x239f0443, 0x25c003fe, 0x27bf03c4, 0x29a10392, 0x2b6a0367, 0x2d1d0341, 0x2ebe031f, 0x304d0300,
    0x31d105b0, 0x34a80555, 0x37520507, 0x39d504c5, 0x3c37048b, 0x3e7c0458, 0x40a8042a, 0x42bd0401,
    0x44c20798, 0x488e071e, 0x4c1c06b6, 0x4f76065d, 0x52a50610, 0x55ac05cc, 0x5892058f, 0x5b590559,
    0x5e0c0a23, 0x631c0980, 0x67db08f6, 0x6c55087f, 0x70940818, 0x74a007bd, 0x787d076c, 0x7c330723,
    0x06970158, 0x07420142, 0x07e30130, 0x087b0120, 0x090b0112, 0x09940106, 0x0a1700fc, 0x0a9500f2,
};

static const uint fp32_to_srgb8_tab4[104] = {
    0x0073000d, 0x007a000d, 0x0080000d, 0x0087000d, 0x008d000d, 0x0094000d, 0x009a000d, 0x00a1000d,
    0x00a7001a, 0x00b4001a, 0x00c1001a, 0x00ce001a, 0x00da001a, 0x00e7001a, 0x00f4001a, 0x0101001a,
    0x010e0033, 0x01280033, 0x01410033, 0x015b0033, 0x01750033, 0x018f0033, 0x01a80033, 0x01c20033,
    0x01dc0067, 0x020f0067, 0x02430067, 0x02760067, 0x02aa0067, 0x02dd0067, 0x03110067, 0x03440067,
    0x037800ce, 0x03df00ce, 0x044600ce, 0x04ad00ce, 0x051400ce, 0x057b00c5, 0x05dd00bc, 0x063b00b5,
    0x06970158, 0x07420142, 0x07e30130, 0x087b0120, 0x090b0112, 0x09940106, 0x0a1700fc, 0x0a9500f2,
    0x0b0f01cb, 0x0bf401ae, 0x0ccb0195, 0x0d950180, 0x0e56016e, 0x0f0d015e, 0x0fbc0150, 0x10630143,
    0x11070264, 0x1238023e, 0x1357021d, 0x14660201, 0x156601e9, 0x165a01d3, 0x174401c0, 0x182401af,
    0x18fe0331, 0x1a9602fe, 0x1c1502d2, 0x1d7e02ad, 0x1ed4028d, 0x201a0270, 0x21520256, 0x227d0240,
    0x239f0443, 0x25c003fe, 0x27bf03c4, 0x29a10392, 0x2b6a0367, 0x2d1d0341, 0x2ebe031f, 0x304d0300,
    0x31d105b0, 0x34a80555, 0x37520507, 0x39d504c5, 0x3c37048b, 0x3e7c0458, 0x40a8042a, 0x42bd0401,
    0x44c20798, 0x488e071e, 0x4c1c06b6, 0x4f76065d, 0x52a50610, 0x55ac05cc, 0x5892058f, 0x5b590559,
    0x5e0c0a23, 0x631c0980, 0x67db08f6, 0x6c55087f, 0x70940818, 0x74a007bd, 0x787d076c, 0x7c330723,
};


static uint8 float_to_srgb8(float in)
{
    static const FP32 almostone = { 0x3f7fffff }; // 1-eps
    static const FP32 lutthresh = { 0x3b800000 }; // 2^(-8)
    static const FP32 linearsc = { 0x454c5d00 };
    static const FP32 float2int = { (127 + 23) << 23 };
    FP32 f;

    // Clamp to [0, 1-eps]; these two values map to 0 and 1, respectively.
    // The tests are carefully written so that NaNs map to 0, same as in the reference
    // implementation.
    if (!(in > 0.0f)) // written this way to catch NaNs
        in = 0.0f;
    if (in > almostone.f)
        in = almostone.f;

    // Check which region this value falls into
    f.f = in;
    if (f.f < lutthresh.f) // linear region
    {
        f.f *= linearsc.f;
        f.f += float2int.f; // use "magic value" to get float->int with rounding.
        return (uint8) (f.u & 255);
    }
    else // non-linear region
    {
        // Unpack bias, scale from table
        uint tab = fp32_to_srgb8_tab3[(f.u >> 20) & 63];
        uint bias = (tab >> 16) << 9;
        uint scale = tab & 0xffff;

        // Grab next-highest mantissa bits and perform linear interpolation
        uint t = (f.u >> 12) & 0xff;
        return (uint8) ((bias + scale*t) >> 16);
    }
}


static __forceinline __m128i float_to_rgb8_SSE2(__m128 f)
{
#define SSE_CONST4(name, val) static const __declspec(align(16)) uint name[4] = { (val), (val), (val), (val) }
#define CONST(name) *(const __m128i *)&name
#define CONSTF(name) *(const __m128 *)&name

    SSE_CONST4(c_almostone, 0x3f7fffff);
	static const __m128 myscale = _mm_set1_ps(255.0f );

	// clamp
//	__m128 zero = _mm_setzero_ps();
//	__m128 myscale = _mm_set1_ps(255.0f);

//	__m128 clamp1 = _mm_max_ps(f,zero);
	__m128 clamp2 = _mm_min_ps(f,CONSTF(c_almostone));

	//clamp2 = _mm_sqrt_ps(clamp2);

	__m128 scaled = _mm_mul_ps(clamp2,myscale);

	__m128i result = _mm_cvtps_epi32( scaled );
	return result;
}



static __forceinline __m128i float_to_srgb8_SSE2(__m128 f)
{
#define SSE_CONST4(name, val) static const __declspec(align(16)) uint name[4] = { (val), (val), (val), (val) }
#define CONST(name) *(const __m128i *)&name
#define CONSTF(name) *(const __m128 *)&name

    SSE_CONST4(c_almostone, 0x3f7fffff);
    SSE_CONST4(c_lutthresh, 0x3b800000);
    SSE_CONST4(c_tabmask, 63);
    SSE_CONST4(c_linearsc, 0x454c5d00);
    SSE_CONST4(c_mantmask, 0xff);
    SSE_CONST4(c_topscale, 0x02000000);

    __m128i temp; // temp value (on stack)

    // Initial clamp
    __m128 zero = _mm_setzero_ps();
    __m128 clamp1 = _mm_max_ps(f, zero); // limit to [0,1-eps] - also nukes NaNs
    __m128 clamp2 = _mm_min_ps(clamp1, CONSTF(c_almostone));

    // Table index
    __m128i tabidx1 = _mm_srli_epi32(_mm_castps_si128(clamp2), 20);
    __m128i tabidx2 = _mm_and_si128(tabidx1, CONST(c_tabmask));
    _mm_store_si128(&temp, tabidx2);

    // Table lookup
    temp.m128i_u32[0] = fp32_to_srgb8_tab3[temp.m128i_u32[0]];
    temp.m128i_u32[1] = fp32_to_srgb8_tab3[temp.m128i_u32[1]];
    temp.m128i_u32[2] = fp32_to_srgb8_tab3[temp.m128i_u32[2]];
    temp.m128i_u32[3] = fp32_to_srgb8_tab3[temp.m128i_u32[3]];

    // Linear part of ramp
    __m128 linear1 = _mm_mul_ps(clamp2, CONSTF(c_linearsc));
    __m128i linear2 = _mm_cvtps_epi32(linear1);

    // Table finisher
    __m128i tabval = _mm_load_si128(&temp);
    __m128i tabmult1 = _mm_srli_epi32(_mm_castps_si128(clamp2), 12);
    __m128i tabmult2 = _mm_and_si128(tabmult1, CONST(c_mantmask));
    __m128i tabmult3 = _mm_or_si128(tabmult2, CONST(c_topscale));
    __m128i tabprod = _mm_madd_epi16(tabval, tabmult3);
    __m128i tabshifted = _mm_srli_epi32(tabprod, 16);

    // Combine linear+table
    __m128 b_uselin = _mm_cmplt_ps(clamp2, CONSTF(c_lutthresh)); // use linear results
    __m128i merge1 = _mm_and_si128(linear2, _mm_castps_si128(b_uselin));
    __m128i merge2 = _mm_andnot_si128(_mm_castps_si128(b_uselin), tabshifted);
    __m128i result = _mm_or_si128(merge1, merge2);

    return result;

#undef SSE_CONST4
#undef CONST
#undef CONSTF
}

static __forceinline __m128i float_to_srgb8_var2_SSE2(__m128 f)
{
#define SSE_CONST4(name, val) static const __declspec(align(16)) uint name[4] = { (val), (val), (val), (val) }
#define CONST(name) *(const __m128i *)&name
#define CONSTF(name) *(const __m128 *)&name

    SSE_CONST4(c_clampmin, (127 - 13) << 23);
    SSE_CONST4(c_almostone, 0x3f7fffff);
    SSE_CONST4(c_lutthresh, 0x3b800000);
    SSE_CONST4(c_mantmask, 0xff);
    SSE_CONST4(c_topscale, 0x02000000);

    __m128i temp; // temp value (on stack)

    // Initial clamp
    __m128 clamp1 = _mm_max_ps(f, CONSTF(c_clampmin)); // limit to [clampmin,1-eps] - also nuke NaNs
    __m128 clamp2 = _mm_min_ps(clamp1, CONSTF(c_almostone));

    // Table index
    __m128i tabidx = _mm_srli_epi32(_mm_castps_si128(clamp2), 20);
    _mm_store_si128(&temp, tabidx);

    // Table lookup
    temp.m128i_u32[0] = fp32_to_srgb8_tab4[temp.m128i_i32[0] - (127-13)*8];
    temp.m128i_u32[1] = fp32_to_srgb8_tab4[temp.m128i_i32[1] - (127-13)*8];
    temp.m128i_u32[2] = fp32_to_srgb8_tab4[temp.m128i_i32[2] - (127-13)*8];
    temp.m128i_u32[3] = fp32_to_srgb8_tab4[temp.m128i_i32[3] - (127-13)*8];

    // Finisher
    __m128i tabval = _mm_load_si128(&temp);
    __m128i tabmult1 = _mm_srli_epi32(_mm_castps_si128(clamp2), 12);
    __m128i tabmult2 = _mm_and_si128(tabmult1, CONST(c_mantmask));
    __m128i tabmult3 = _mm_or_si128(tabmult2, CONST(c_topscale));
    __m128i tabprod = _mm_madd_epi16(tabval, tabmult3);
    __m128i result = _mm_srli_epi32(tabprod, 16);

    return result;

#undef SSE_CONST4
#undef CONST
#undef CONSTF
}


#endif //__RYG_SRGB_H