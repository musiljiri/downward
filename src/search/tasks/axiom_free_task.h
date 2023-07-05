#ifndef TASKS_AXIOM_FREE_TASK_H
#define TASKS_AXIOM_FREE_TASK_H

#include "delegating_task.h"

using namespace std;

namespace plugins {
class Options;
}

namespace tasks {
/*
  Task transformation that removes axioms
  using the compilation from paper "In defense of PDDL axioms", page 58.
*/
class AxiomFreeTask : public DelegatingTask {
    int parent_action_count;
    int axiom_layer_count;
    vector<ExplicitVariable> variables;
    vector<ExplicitOperator> new_actions;
    vector<ExplicitOperator> actions;
public:
    AxiomFreeTask(
        const std::shared_ptr<AbstractTask> &parent);
    virtual ~AxiomFreeTask() override = default;

    // computes the count of axiom layers
    void get_axiom_layer_count();
    // adds done/fixed/new variables
    void add_new_variables();
    // returns axioms in given layer
    vector<int> get_axioms_in_layer(int layer);
    // returns the precondition and all effect conditions for changing axiom to value
    vector<FactPair> collect_axiom_conditions(int axiom, int value);
    // adds stratum and fixpoint action
    void add_new_actions();
    // computes k = maximum layer of a derived variable that appears in the precondition of the action
    int get_max_layer_in_precondition(ExplicitOperator action);
    // computes m = minimum layer of a derived variable whose axiom condition (body) is modified by the action
    int get_min_layer_in_effect(ExplicitOperator action);
    // modifies all original existing actions
    void modify_existing_actions();


    //int get_operator_cost(int index, bool is_axiom) const override;
    //int convert_operator_index_to_parent(int index) const override;
};
}

#endif
