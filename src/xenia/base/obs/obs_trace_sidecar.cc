/**
 ******************************************************************************
 * Writes `{trace}.obs.json` with obs event seq range for phoenixctl trace
 *explain.
 ******************************************************************************
 */

#include "xenia/base/obs/obs_trace_sidecar.h"

#include <fstream>

#include "third_party/fmt/include/fmt/format.h"
#include "xenia/base/obs/obs.h"

namespace xe {
namespace obs {

void WriteTraceObsSidecar(const std::filesystem::path& xtr_path,
                          uint64_t seq_first, uint64_t seq_last,
                          uint32_t frame_id) {
  if (xtr_path.empty() || seq_last < seq_first) {
    return;
  }
  auto sidecar = xtr_path;
  sidecar += ".obs.json";
  std::ofstream out(sidecar);
  if (!out) {
    return;
  }
  out << fmt::format(
      "{{\"v\":1,\"xtr\":\"{}\",\"session\":\"{}\",\"obs_event_seq_first\":{},"
      "\"obs_event_seq_last\":{},\"frame\":{}}}\n",
      xtr_path.filename().string(), SessionId(), seq_first, seq_last, frame_id);
}

}  // namespace obs
}  // namespace xe
