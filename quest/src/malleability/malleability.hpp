/** @file
 * Internal malleability helpers, C++ only.
 * Do NOT include from .c files.
 *
 * @author Íñigo Aréjula Aísa
 */

#ifndef MALLEABILITY_HPP
#define MALLEABILITY_HPP

#include "quest/include/malleability.h"

#include <vector>
#include <cstdio>
#include <cstdlib>


void mal_resize_to(int target_nodes);
int mal_target_nodes_for_qubits(int numQubits);

// CircuitExecutor is a singleton class that manages the execution of circuits
// with malleability support
class CircuitExecutor {

public:

    static CircuitExecutor& instance();

    /* Disable copy and move. */
    CircuitExecutor(const CircuitExecutor&)            = delete;
    CircuitExecutor& operator=(const CircuitExecutor&) = delete;
    CircuitExecutor(CircuitExecutor&&)                 = delete;
    CircuitExecutor& operator=(CircuitExecutor&&)      = delete;

    void init(int argc, char** argv);
    void add(Circuit circuit);
    void run();
    void destroy();

private:

    CircuitExecutor();

    std::vector<Circuit> circuits_;
    bool                 initialized_;
};

#endif /* MALLEABILITY_HPP */
