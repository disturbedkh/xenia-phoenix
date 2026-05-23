#ifndef XENIA_TOOLS_VMX128_FUZZ_VMX128_PIN_TABLE_H_
#define XENIA_TOOLS_VMX128_FUZZ_VMX128_PIN_TABLE_H_

#include "xenia/base/vec128.h"

namespace xe::vmx128_fuzz {

bool LookupVaddfpPin(const vec128_t& a, const vec128_t& b, vec128_t* out);
bool LookupVaddfp128Pin(const vec128_t& a, const vec128_t& b, vec128_t* out);
bool LookupVmaxfpPin(const vec128_t& a, const vec128_t& b, vec128_t* out);
bool LookupVmaxfp128Pin(const vec128_t& a, const vec128_t& b, vec128_t* out);
bool LookupVminfpPin(const vec128_t& a, const vec128_t& b, vec128_t* out);
bool LookupVminfp128Pin(const vec128_t& a, const vec128_t& b, vec128_t* out);
bool LookupVmulfp128Pin(const vec128_t& a, const vec128_t& b, vec128_t* out);
bool LookupVrfipPin(const vec128_t& vb, vec128_t* out);
bool LookupVrfip128Pin(const vec128_t& vb, vec128_t* out);
bool LookupVrefpPin(const vec128_t& vb, vec128_t* out);
bool LookupVrefp128Pin(const vec128_t& vb, vec128_t* out);
bool LookupVmaddfp128Pin(const vec128_t& a, const vec128_t& b, vec128_t* out);
bool LookupVnmsubfpPin(const vec128_t& a, const vec128_t& b, vec128_t* out);
bool LookupVnmsubfp128Pin(const vec128_t& a, const vec128_t& b, vec128_t* out);
bool LookupVsubfpPin(const vec128_t& a, const vec128_t& b, vec128_t* out);
bool LookupVsubfp128Pin(const vec128_t& a, const vec128_t& b, vec128_t* out);

}  // namespace xe::vmx128_fuzz

#endif
