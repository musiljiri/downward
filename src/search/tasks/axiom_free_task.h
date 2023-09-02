#ifndef TASKS_AXIOM_FREE_TASK_H
#define TASKS_AXIOM_FREE_TASK_H

#include "delegating_task.h"
#include "../utils/collections.h"

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
    int parent_var_count;
    int axiom_layer_count;
    vector<ExplicitVariable> variables;
    vector<ExplicitOperator> new_actions;
    vector<ExplicitOperator> actions;
    vector<int> initial_state_values;
    vector<FactPair> goals;

    const ExplicitVariable &get_variable(int var) const;
    const ExplicitOperator &get_operator(int index) const;
    const ExplicitEffect &get_effect(int op_id, int effect_id) const;

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
    vector<vector<FactPair>> collect_axiom_conditions(int axiom, int value);
    // adds stratum and fixpoint action
    void add_new_actions();
    // computes k = maximum layer of a derived variable that appears in the precondition (and effect conditions) of the action
    int get_max_layer_in_precond_and_effcond(ExplicitOperator action);
    // computes m = minimum layer of a derived variable whose axiom condition (body) is modified by the action
    int get_min_layer_in_effect(ExplicitOperator action);
    // modifies all original existing actions
    void modify_existing_actions();
    // adds to the initial state the values for done/fixed/new variables - only fixed0 is true
    void modify_initial_state();
    // adds to the goal the fixed variable of the highest stratum appearing in the goal
    void modify_goal();

    int get_num_variables() const override;
    string get_variable_name(int var) const override;
    int get_variable_domain_size(int var) const override;
    int get_variable_axiom_layer(int var) const override;
    int get_variable_default_axiom_value(int var) const override;
    string get_fact_name(const FactPair &fact) const override;
    bool are_facts_mutex(const FactPair &fact1, const FactPair &fact2) const override;

    int get_operator_cost(int index, bool is_axiom) const override;
    string get_operator_name(int index, bool is_axiom) const override;
    int get_num_operators() const override;
    int get_num_operator_preconditions(int index, bool is_axiom) const override;
    FactPair get_operator_precondition(int op_index, int fact_index, bool is_axiom) const override;
    int get_num_operator_effects(int op_index, bool is_axiom) const override;
    int get_num_operator_effect_conditions(int op_index, int eff_index, bool is_axiom) const override;
    FactPair get_operator_effect_condition(int op_index, int eff_index, int cond_index, bool is_axiom) const override;
    FactPair get_operator_effect(int op_index, int eff_index, bool is_axiom) const override;
    const ExplicitOperator &get_operator_or_axiom(int index, bool is_axiom) const override;
    int convert_operator_index_to_parent(int index) const override;

    int get_num_axioms() const override;
    int get_num_goals() const override;
    FactPair get_goal_fact(int index) const override;
    vector<int> get_initial_state_values() const override;
    void convert_state_values_from_parent(vector<int> &values) const override;
};
}

#endif
