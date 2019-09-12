
#include <cstdlib>
#include <iomanip>
#include <sstream>

#include <c10/util/Exception.h>
#include <c10/util/StringUtil.h>
#include <torch/csrc/jit/function.h>
#include <torch/csrc/jit/ir.h>
#include <torch/csrc/jit/jit_log.h>
#include <torch/csrc/jit/passes/python_print.h>
#include <torch/csrc/jit/script/error_report.h>

namespace torch {
namespace jit {

JitLoggingLevels jit_log_level() {
  static const char* c_log_level = std::getenv("PYTORCH_JIT_LOG_LEVEL");
  static const JitLoggingLevels log_level = c_log_level
      ? static_cast<JitLoggingLevels>(std::atoi(c_log_level))
      : JitLoggingLevels::OFF;
  return log_level;
}

// Unfortunately, in `GraphExecutor` where `log_function` is invoked
// we won't have access to an original function, so we have to construct
// a dummy function to give to PythonPrint
std::string log_function(const std::shared_ptr<torch::jit::Graph> &graph) {
  torch::jit::Function func("source_dump", graph, nullptr);
  std::stringstream ss;
  std::vector<at::Tensor> tensors;
  std::vector<c10::NamedTypePtr> deps;
  SourceRangeRecords source_ranges;
  PythonPrint(ss, source_ranges, func, false, tensors, deps, false);
  return ss.str();
}

std::string debugValueOrDefault(const Node* n) {
  return n->outputs().size() > 0 ? n->outputs().at(0)->debugName() : "n/a";
}

std::string jit_log_prefix(
    const std::string& prefix,
    const std::string& in_str) {
  std::stringstream in_ss(in_str);
  std::stringstream out_ss;
  std::string line;
  while (std::getline(in_ss, line)) {
    out_ss << prefix << line << std::endl;
  }

  return out_ss.str();
}

std::string jit_log_prefix(
    JitLoggingLevels level,
    const char* fn,
    int l,
    const std::string& in_str) {
  std::stringstream prefix_ss;
  prefix_ss << "[";
  prefix_ss << level << " ";
  prefix_ss << c10::detail::StripBasename(std::string(fn)) << ":";
  prefix_ss << std::setfill('0') << std::setw(3) << l;
  prefix_ss << "] ";

  return jit_log_prefix(prefix_ss.str(), in_str);
}

std::ostream& operator<<(std::ostream& out, JitLoggingLevels level) {
  switch (level) {
    case JitLoggingLevels::OFF:
      TORCH_INTERNAL_ASSERT("UNREACHABLE");
      break;
    case JitLoggingLevels::GRAPH_DUMP:
      out << "DUMP";
      break;
    case JitLoggingLevels::GRAPH_UPDATE:
      out << "UPDATE";
      break;
    case JitLoggingLevels::GRAPH_DEBUG:
      out << "DEBUG";
      break;
    default:
      TORCH_INTERNAL_ASSERT("Invalid level");
  }

  return out;
}

} // namespace jit
} // namespace torch
