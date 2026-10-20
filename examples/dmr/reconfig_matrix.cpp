/** @file
 * All-to-all reconfiguration matrix through the malleable circuit executor.
 *
 * Every step only creates and destroys a Qureg whose size the resize policy
 * maps to a given node count, so the run measures DMR reconfigurations and
 * nothing else. The node counts (default 1 2 4 8 16 32 64) are visited along
 * an Eulerian circuit of the complete directed graph over them: every ordered
 * pair (from, to) with from != to appears exactly once per repetition, and
 * each repetition starts and ends on the first node count, which is also the
 * size the job is launched with.
 *
 *   reconfig_matrix_cpp [--reps=R] [node counts...]
 *
 * @author Íñigo Aréjula Aísa
 */
#include "quest.h"
#include "dmr.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <vector>

namespace {

constexpr int MAX_STEPS = 512;

std::vector<int> stepNodes;
std::vector<int> stepQubits;

void emit_analytics_event(const char *event_name) {
  DMRAnalytics *analytics;
  dmr_create_custom_analytics_event(const_cast<char *>(event_name), &analytics);
  dmr_print_analytics_from(analytics);
  dmr_destroy_custom_analytics_event(analytics);
}

// Qubit count the resize policy maps to the given node count, chosen so that
// every node holds the same number of amplitudes: the smallest such count for
// distributed registers, and the largest one that stays on a single node.
int qubits_for_nodes(int nodes) {
  for (int q = 1; q < 64; q++) {
    int target = mal_target_nodes_for_qubits(q);
    if (nodes == 1 && target > 1)
      return q - 1;
    if (nodes > 1 && target == nodes)
      return q;
  }
  return -1;
}

// Hierholzer's algorithm on the complete directed graph without self-loops:
// returns the vertex sequence of a circuit that starts and ends at vertex 0
// and uses every edge once.
std::vector<int> eulerian_circuit(int numVertices) {
  std::vector<std::vector<int>> pending(numVertices);
  for (int v = 0; v < numVertices; v++)
    for (int w = numVertices - 1; w >= 0; w--)
      if (w != v)
        pending[v].push_back(w);

  std::vector<int> stack = {0}, circuit;
  while (!stack.empty()) {
    int v = stack.back();
    if (pending[v].empty()) {
      circuit.push_back(v);
      stack.pop_back();
    } else {
      stack.push_back(pending[v].back());
      pending[v].pop_back();
    }
  }
  return std::vector<int>(circuit.rbegin(), circuit.rend());
}

void run_step(int step) {
  int nodes = stepNodes[step];
  int qubits = stepQubits[step];
  printf("\n[matrix] step %d/%zu: %d qubits -> %d nodes\n", step,
         stepNodes.size() - 1, qubits, nodes);

  Qureg qureg = createQureg(qubits);
  emit_analytics_event("step_start");
  destroyQureg(qureg);
}

template <int I> void step_fn(void) { run_step(I); }

template <int... Is>
constexpr std::array<Circuit, sizeof...(Is)>
make_steps(std::integer_sequence<int, Is...>) {
  return {step_fn<Is>...};
}

} // namespace

int main(int argc, char *argv[]) {
  int reps = 1;
  std::vector<int> nodeCounts;
  for (int i = 1; i < argc; i++) {
    if (strncmp(argv[i], "--reps=", 7) == 0)
      reps = atoi(argv[i] + 7);
    else
      nodeCounts.push_back(atoi(argv[i]));
  }
  if (nodeCounts.empty())
    nodeCounts = {1, 2, 4, 8, 16, 32, 64};

  initQuESTEnv();
  circuit_executor_init(argc, argv);

  std::vector<int> order = eulerian_circuit((int)nodeCounts.size());
  for (int r = 0; r < reps; r++)
    for (size_t k = (r == 0) ? 0 : 1; k < order.size(); k++)
      stepNodes.push_back(nodeCounts[order[k]]);

  if ((int)stepNodes.size() > MAX_STEPS) {
    fprintf(stderr, "reconfig_matrix: %zu steps, at most %d\n",
            stepNodes.size(), MAX_STEPS);
    return 1;
  }
  for (int nodes : stepNodes) {
    int q = qubits_for_nodes(nodes);
    if (q < 0) {
      fprintf(stderr, "reconfig_matrix: no qubit count maps to %d nodes\n",
              nodes);
      return 1;
    }
    stepQubits.push_back(q);
  }

  printf("# matrix nodes:");
  for (int n : nodeCounts)
    printf(" %d", n);
  printf("\n# matrix reps: %d\n# matrix sequence:", reps);
  for (int n : stepNodes)
    printf(" %d", n);
  printf("\n");

  static const auto steps =
      make_steps(std::make_integer_sequence<int, MAX_STEPS>{});
  for (size_t i = 0; i < stepNodes.size(); i++)
    circuit_executor_add(steps[i]);
  circuit_executor_run();
  circuit_executor_destroy();
  finalizeQuESTEnv();
  return 0;
}
