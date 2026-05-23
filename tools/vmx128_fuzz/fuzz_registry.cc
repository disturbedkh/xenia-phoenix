/**
 ******************************************************************************
 * vmx128-fuzz registry + JSON helpers.
 ******************************************************************************
 */

#include "fuzz_internal.h"

#include <algorithm>
#include <iostream>

namespace xe::vmx128_fuzz {

std::vector<FuzzCase>& FuzzCases() {
  static std::vector<FuzzCase> g;
  return g;
}

void AppendVecJson(std::ostringstream& oss, std::string_view key,
                   const vec128_t& v) {
  oss << '"' << key << "_lo\":\"" << std::hex << std::uppercase << v.low
      << "\",\"" << key << "_hi\":\"" << v.high << '"' << std::dec;
}

void LogDivergence(std::string_view op, const vec128_t& a, const vec128_t& b,
                   const vec128_t& expect, const vec128_t& got) {
  std::ofstream f("vmx128_divergences.jsonl", std::ios::app);
  if (!f) {
    return;
  }
  std::ostringstream oss;
  oss << "{\"op\":\"" << op << "\",";
  AppendVecJson(oss, "a", a);
  oss << ',';
  AppendVecJson(oss, "b", b);
  oss << ',';
  AppendVecJson(oss, "expect", expect);
  oss << ',';
  AppendVecJson(oss, "got", got);
  oss << "}\n";
  f << oss.str();
}

bool IsHighCompileCostOpcode(std::string_view name) {
  return name.find("vrlimi128") != std::string_view::npos ||
         name.find("vspltisw") != std::string_view::npos ||
         name.find("vperm128") != std::string_view::npos ||
         name.find("vperm") != std::string_view::npos;
}

void RegisterAllFuzzCases() {
  auto& v = FuzzCases();
  v.clear();
  RegisterIntegerAddSub();
  RegisterLogicalMinmaxAvg();
  RegisterCompareSelect();
  RegisterShiftsRotatesWhole();
  RegisterPermSplatMerge();
  RegisterPackUnpack();
  RegisterMulSum();
  RegisterSums();
  RegisterFloatConverts();
  RegisterFloatBinaryVm128();
  std::sort(v.begin(), v.end(),
            [](const FuzzCase& a, const FuzzCase& b) {
              const bool ah = IsHighCompileCostOpcode(a.name);
              const bool bh = IsHighCompileCostOpcode(b.name);
              if (ah != bh) {
                return !ah;
              }
              return std::string_view(a.name) < std::string_view(b.name);
            });
}

}  // namespace xe::vmx128_fuzz
