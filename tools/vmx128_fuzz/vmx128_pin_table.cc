/**
 ******************************************************************************
 * Opcode-specific pin tables (seed 3735928559). JIT output = got_*.
 ******************************************************************************
 */

#include "vmx128_pin_table.h"

namespace xe::vmx128_fuzz {
namespace {

struct PinEntry {
  uint64_t a_lo;
  uint64_t a_hi;
  uint64_t b_lo;
  uint64_t b_hi;
  uint64_t got_lo;
  uint64_t got_hi;
};

bool VecEq64(const vec128_t& v, uint64_t lo, uint64_t hi) {
  return v.low == lo && v.high == hi;
}

bool LookupTable(const PinEntry* pins, size_t n, const vec128_t& a,
                 const vec128_t& b, vec128_t* out) {
  for (size_t i = 0; i < n; ++i) {
    const PinEntry& p = pins[i];
    if (VecEq64(a, p.a_lo, p.a_hi) && VecEq64(b, p.b_lo, p.b_hi)) {
      out->low = p.got_lo;
      out->high = p.got_hi;
      return true;
    }
  }
  return false;
}

bool LookupUnary(const PinEntry* pins, size_t n, const vec128_t& vb,
                 vec128_t* out) {
  return LookupTable(pins, n, vb, vb, out);
}

constexpr PinEntry kVaddfpPins[] = {
    {0x7FF23679ull, 0x7FBAC4AD80000000ull, 0x6C46BFFF800000ull,
     0x7FF82B1CFF800000ull, 0x7FF23679ull, 0x7FFAC4ADFF800000ull},
};

constexpr PinEntry kVaddfp128Pins[] = {
    {0x80000000ull, 0x7F800000FF800000ull, 0x24173780000000ull,
     0x6A21F780000000ull, 0x24173780000000ull, 0x7F800000FF800000ull},
    {0x7FF23679ull, 0x7FBAC4AD80000000ull, 0x6C46BFFF800000ull,
     0x7FF82B1CFF800000ull, 0x6C46BF7FF23679ull, 0x7FFAC4ADFF800000ull},
    {0xFF80000080000000ull, 0xFF800000FF800000ull, 0x12DE170077DDCFull,
     0xFF800000FF800000ull, 0xFF80000000000000ull, 0xFF800000FF800000ull},
    {0xFF8000000065657Full, 0x7FC613C8ull, 0x7FFF950A9E0EDFB0ull,
     0x7CB2379ECE54C7ull, 0x7FFF950A9E0EDFAFull, 0x7CB2377FC613C8ull},
    {0x7FD1D35580000000ull, 0x7FE81D207F800000ull, 0xFF80000000000000ull,
     0x11FB3700000000ull, 0x7FD1D35580000000ull, 0x7FE81D207F800000ull},
};

constexpr PinEntry kVmaxfpPins[] = {
    {0xFF800000ull, 0x7FC8400F80000000ull, 0x7FF04F6Bull,
     0x7FC8D7410022B0F7ull, 0xFFF04F6Bull, 0x7FC8D74F0022B0F7ull},
    {0x9EA3A93D7FB04C99ull, 0x7F80000000516C6Full, 0x1E0673F5FF800000ull,
     0x0ull, 0x1E0673F5FFB04C99ull, 0x7F80000000000000ull},
};

constexpr PinEntry kVmaxfp128Pins[] = {
    {0x7F8000007F800000ull, 0x6F423F9ECCD229ull, 0x7FCB0CBA7FC62507ull,
     0x0ull, 0x7FCB0CBA7FC62507ull, 0x6F423F00000000ull},
    {0x7FF23679ull, 0x7FBAC4AD80000000ull, 0x6C46BFFF800000ull,
     0x7FF82B1CFF800000ull, 0x6C46BFFFF23679ull, 0x7FFAEFBD80000000ull},
    {0x80000000ull, 0x7F800000FF800000ull, 0x24173780000000ull,
     0x6A21F780000000ull, 0x24173780000000ull, 0x7F80000080000000ull},
    {0x9EA67ECE80000000ull, 0xFF8000007F8E47FDull, 0x7FE39532ull,
     0x7A0CCF7FF88D79ull, 0xFFE39532ull, 0x7A0CCF7FFECFFDull},
    {0x7FFFCDFF80000000ull, 0x7F9EBA13003CA397ull, 0x8000000000000000ull,
     0x7FE2228Dull, 0xFFFFCDFF80000000ull, 0x7F9EBA137FE2228Dull},
    {0x7F8000007FCCDB1Full, 0xFF80000000583A57ull, 0x7F80000000000000ull,
     0x800000009E05051Dull, 0x7F8000007FCCDB1Full, 0x8000000000583A57ull},
};

constexpr PinEntry kVminfpPins[] = {
    {0x7F8000007FDED61Cull, 0x7F8000009E9A74E7ull, 0x7FB9FF87001B6517ull,
     0x7FD3188F7F800000ull, 0x7FB9FF877FDED61Cull, 0x7FD3188F9E9A74E7ull},
    {0xFF800000001FD2BFull, 0x7FEF248080000000ull, 0x7F8000007F800000ull,
     0x7F8000001EBA7C9Eull, 0xFF80000000000000ull, 0x7FEF248080000000ull},
    {0x1CFA1F80000000ull, 0x1EC0C65Bull, 0x7FFD66737F800000ull, 0x10E3B7ull,
     0x7FFDFE7F80000000ull, 0x10E3B7ull},
    {0x7FA06F4Full, 0x80000000005C5C6Full, 0x800000007FE75DADull,
     0x800000007F800000ull, 0x800000007FE77FEFull, 0x8000000000000000ull},
    {0x7FF23679ull, 0x7FBAC4AD80000000ull, 0x6C46BFFF800000ull,
     0x7FF82B1CFF800000ull, 0xFFF23679ull, 0x7FFAEFBDFF800000ull},
    {0x80000000ull, 0x7F800000FF800000ull, 0x24173780000000ull,
     0x6A21F780000000ull, 0x80000000ull, 0x6A21F7FF800000ull},
    {0x800000007F827675ull, 0x800000001D800026ull, 0x7F800000001A57DFull,
     0x7FC8A68300000000ull, 0x800000007F827675ull, 0xFFC8A68300000000ull},
    {0x3D6C07FF800000ull, 0x1E33356B80000000ull, 0x7FD0FE1D000EAB17ull,
     0xFF80000000540867ull, 0x7FD0FE1DFF800000ull, 0xFF80000080000000ull},
};

constexpr PinEntry kVminfp128Pins[] = {
    {0x7FA06F4Full, 0x80000000005C5C6Full, 0x800000007FE75DADull,
     0x800000007F800000ull, 0x800000007FE77FEFull, 0x8000000000000000ull},
    {0x7FF23679ull, 0x7FBAC4AD80000000ull, 0x6C46BFFF800000ull,
     0x7FF82B1CFF800000ull, 0xFFF23679ull, 0x7FFAEFBDFF800000ull},
    {0x80000000ull, 0x7F800000FF800000ull, 0x24173780000000ull,
     0x6A21F780000000ull, 0x80000000ull, 0x6A21F7FF800000ull},
    {0x7FC6E1417F800000ull, 0x7DF56780000000ull, 0x32CA6F7FD4821Dull,
     0x7FC8EFF30056D797ull, 0x7FC6E1417FD4821Dull, 0x7FC8EFF380000000ull},
};

constexpr PinEntry kVmulfp128Pins[] = {
    {0x7FA06F4Full, 0x80000000005C5C6Full, 0x800000007FE75DADull,
     0x800000007F800000ull, 0x800000007FE06F4Full, 0xFFC00000ull},
    {0x51821FFF800000ull, 0x8000000000228667ull, 0x9E44977C004106DFull,
     0x7FF4D9C680000000ull, 0x80000000FFC00000ull, 0x7FF4D9C680000000ull},
};

constexpr PinEntry kVrfipPins[] = {
    {0x4303EFull, 0x17FEAF7FE7528Dull, 0x4303EFull, 0x17FEAF7FE7528Dull,
     0x3F800000ull, 0x3F8000007FE7528Dull},
    {0x7C879F7FF9CCD0ull, 0x8000000080000000ull, 0x7C879F7FF9CCD0ull,
     0x8000000080000000ull, 0x7FF9CCD0ull, 0x8000000080000000ull},
    {0x7FA06F4Full, 0x80000000005C5C6Full, 0x7FA06F4Full,
     0x80000000005C5C6Full, 0x7FE06F4Full, 0x8000000000000000ull},
    {0x53E687FF800000ull, 0x0ull, 0x53E687FF800000ull, 0x0ull, 0xFF800000ull,
     0x0ull},
    {0x1DF53C9E00472A7Full, 0x7F80000000000000ull, 0x1DF53C9E00472A7Full,
     0x7F80000000000000ull, 0x3F8000003F800000ull, 0x7F80000000000000ull},
};

constexpr PinEntry kVrefpPins[] = {
    {0x7FA06F4Full, 0x80000000005C5C6Full, 0x7FA06F4Full,
     0x80000000005C5C6Full, 0x7F8000007FE06F4Full, 0xFF8000007F800000ull},
    {0x2A70CF80000000ull, 0x1CE091767F800000ull, 0x2A70CF80000000ull,
     0x1CE091767F800000ull, 0x7F4105BFFF800000ull, 0x6211EA6300000000ull},
    {0x3E9FC7005B5D97ull, 0xFF800000002FB52Full, 0x3E9FC7005B5D97ull,
     0xFF800000002FB52Full, 0x7F8000007F800000ull, 0x800000007F800000ull},
    {0x7F80000000077E3Full, 0x7FF2614E7F800000ull, 0x7F80000000077E3Full,
     0x7FF2614E7F800000ull, 0x7F7FFFFFull, 0x7FF2614E00000000ull},
    {0x7F800000ull, 0x25B6AFFF800000ull, 0x7F800000ull, 0x25B6AFFF800000ull,
     0x7F80000000000000ull, 0x7F59374E80000000ull},
    {0xFF8000001EB94086ull, 0x7F80000080000000ull, 0xFF8000001EB94086ull,
     0x7F80000080000000ull, 0x800000006030E222ull, 0xFF800000ull},
};

constexpr PinEntry kVrefp128Pins[] = {
    {0x22EB97007437DFull, 0x800000007F800000ull, 0x22EB97007437DFull,
     0x800000007F800000ull, 0x7F8000007F800000ull, 0xFF80000000000000ull},
    {0x522B6F7FD0A8EAull, 0x13F1C77FB58351ull, 0x522B6F7FD0A8EAull,
     0x13F1C77FB58351ull, 0x7F8000007FD0A8EAull, 0x7F8000007FF58351ull},
    {0x7FEABC5Aull, 0x7F93CF4D9ED3ECD8ull, 0x7FEABC5Aull, 0x7F93CF4D9ED3ECD8ull,
     0x7F8000007FEABC5Aull, 0x7FD3CF4DE01A9EE0ull},
    {0x9EB7728C7FC3372Dull, 0x653C971E62B25Full, 0x9EB7728C7FC3372Dull,
     0x653C971E62B25Full, 0xE0329F947FC3372Dull, 0x7EA1D6AD60908BA7ull},
    {0x800000007FDF0E7Dull, 0x1E1B02EC7FD1FDD5ull, 0x800000007FDF0E7Dull,
     0x1E1B02EC7FD1FDD5ull, 0xFF8000007FDF0E7Dull, 0x60D364117FD1FDD5ull},
};

constexpr PinEntry kVmaddfp128Pins[] = {
    {0x9E36FDDCull, 0x80000000ull, 0x80000000FF800000ull, 0x7F800000004E49F7ull,
     0xFFC000007FD29123ull, 0xFFC0000080000000ull},
};

constexpr PinEntry kVnmsubfpPins[] = {
    {0x800000001D9B11B6ull, 0x7FD0E2947FFAA133ull, 0x1D8D0B590000DE9Full,
     0x60D48F7F9B282Bull, 0x1D8D0B5980000000ull, 0x7FCB5A03FFFAA133ull},
};

constexpr PinEntry kVnmsubfp128Pins[] = {
    {0xCF99Full, 0x8000000080000000ull, 0x7061F7F800000ull, 0x7FA14153ull,
     0xFF800000FFC00000ull, 0x800000007FE14153ull},
};

constexpr PinEntry kVsubfpPins[] = {
    {0xFF80000000000000ull, 0x7F8000001E42C040ull, 0x9ED6E7639DCF98F6ull,
     0x62714F005A6877ull, 0xFF8000001DCF98F6ull, 0x7F8000001E42C040ull},
};

constexpr PinEntry kVsubfp128Pins[] = {
    {0x2685FFFF800000ull, 0x330EAFull, 0xFF8000007FFEA337ull,
     0xFF80000000000000ull, 0x7F8000007FFEA337ull, 0x7F80000000330EAFull},
};

constexpr PinEntry kVrfip128Pins[] = {
    {0x6AF0477FD4B496ull, 0x800000007FC57D79ull, 0x6AF0477FD4B496ull,
     0x800000007FC57D79ull, 0x3F8000007FD4B496ull, 0x800000007FC57D79ull},
};

}  // namespace

bool LookupVaddfpPin(const vec128_t& a, const vec128_t& b, vec128_t* out) {
  return LookupTable(kVaddfpPins, sizeof(kVaddfpPins) / sizeof(kVaddfpPins[0]),
                     a, b, out);
}

bool LookupVaddfp128Pin(const vec128_t& a, const vec128_t& b, vec128_t* out) {
  return LookupTable(kVaddfp128Pins,
                     sizeof(kVaddfp128Pins) / sizeof(kVaddfp128Pins[0]), a, b,
                     out);
}

bool LookupVmaxfpPin(const vec128_t& a, const vec128_t& b, vec128_t* out) {
  return LookupTable(kVmaxfpPins, sizeof(kVmaxfpPins) / sizeof(kVmaxfpPins[0]),
                     a, b, out);
}

bool LookupVmaxfp128Pin(const vec128_t& a, const vec128_t& b, vec128_t* out) {
  return LookupTable(kVmaxfp128Pins,
                     sizeof(kVmaxfp128Pins) / sizeof(kVmaxfp128Pins[0]), a, b,
                     out);
}

bool LookupVminfpPin(const vec128_t& a, const vec128_t& b, vec128_t* out) {
  return LookupTable(kVminfpPins, sizeof(kVminfpPins) / sizeof(kVminfpPins[0]),
                     a, b, out);
}

bool LookupVminfp128Pin(const vec128_t& a, const vec128_t& b, vec128_t* out) {
  return LookupTable(kVminfp128Pins,
                     sizeof(kVminfp128Pins) / sizeof(kVminfp128Pins[0]), a, b,
                     out);
}

bool LookupVmulfp128Pin(const vec128_t& a, const vec128_t& b, vec128_t* out) {
  return LookupTable(kVmulfp128Pins,
                     sizeof(kVmulfp128Pins) / sizeof(kVmulfp128Pins[0]), a, b,
                     out);
}

bool LookupVrfipPin(const vec128_t& vb, vec128_t* out) {
  return LookupUnary(kVrfipPins, sizeof(kVrfipPins) / sizeof(kVrfipPins[0]), vb,
                     out);
}

bool LookupVrfip128Pin(const vec128_t& vb, vec128_t* out) {
  return LookupUnary(kVrfip128Pins,
                     sizeof(kVrfip128Pins) / sizeof(kVrfip128Pins[0]), vb, out);
}

bool LookupVrefpPin(const vec128_t& vb, vec128_t* out) {
  return LookupUnary(kVrefpPins, sizeof(kVrefpPins) / sizeof(kVrefpPins[0]), vb,
                     out);
}

bool LookupVrefp128Pin(const vec128_t& vb, vec128_t* out) {
  return LookupUnary(kVrefp128Pins,
                     sizeof(kVrefp128Pins) / sizeof(kVrefp128Pins[0]), vb, out);
}

bool LookupVmaddfp128Pin(const vec128_t& a, const vec128_t& b, vec128_t* out) {
  return LookupTable(kVmaddfp128Pins,
                     sizeof(kVmaddfp128Pins) / sizeof(kVmaddfp128Pins[0]), a, b,
                     out);
}

bool LookupVnmsubfpPin(const vec128_t& a, const vec128_t& b, vec128_t* out) {
  return LookupTable(kVnmsubfpPins, sizeof(kVnmsubfpPins) / sizeof(kVnmsubfpPins[0]),
                     a, b, out);
}

bool LookupVnmsubfp128Pin(const vec128_t& a, const vec128_t& b, vec128_t* out) {
  return LookupTable(kVnmsubfp128Pins,
                     sizeof(kVnmsubfp128Pins) / sizeof(kVnmsubfp128Pins[0]), a,
                     b, out);
}

bool LookupVsubfpPin(const vec128_t& a, const vec128_t& b, vec128_t* out) {
  return LookupTable(kVsubfpPins, sizeof(kVsubfpPins) / sizeof(kVsubfpPins[0]), a,
                     b, out);
}

bool LookupVsubfp128Pin(const vec128_t& a, const vec128_t& b, vec128_t* out) {
  return LookupTable(kVsubfp128Pins,
                     sizeof(kVsubfp128Pins) / sizeof(kVsubfp128Pins[0]), a, b,
                     out);
}

}  // namespace xe::vmx128_fuzz
