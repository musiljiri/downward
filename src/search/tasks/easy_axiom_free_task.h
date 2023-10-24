#ifndef TASKS_EASY_AXIOM_FREE_TASK_H
#define TASKS_EASY_AXIOM_FREE_TASK_H

#include "delegating_task.h"
#include "../utils/collections.h"

using namespace std;

namespace plugins {
class Options;
}

namespace tasks {
/*
  Task transformation that removes easy axioms.
*/
class EasyAxiomFreeTask : public DelegatingTask {

    int parent_var_count;
    vector<FactPair> vars_to_replace;
    vector<ExplicitVariable> variables;
    vector<ExplicitOperator> actions;
    vector<ExplicitOperator> axioms;
    vector<int> initial_state_values;
    vector<FactPair> goals;

    const ExplicitVariable &get_variable(int var) const;
    const ExplicitEffect &get_effect(int op_id, int effect_id, bool is_axiom) const;

public:
    EasyAxiomFreeTask(
        const std::shared_ptr<AbstractTask> &parent);
    virtual ~EasyAxiomFreeTask() override = default;

    // returns derived variables that are only used by one of their two values with only one rule for this value only containing basic variables
    void get_vars_to_replace();
    // returns effect conditions from the variable's axiom
    vector<FactPair> get_var_conditions(FactPair var);
    // copies the goal and replaces derived variables
    void replace_goal();
    // copies axioms and replaces derived variables in effect conditions
    void replace_axioms();
    // copies actions and replaces derived variables in preconditions and effect conditions
    void replace_actions();
    // copies variables (and removes replaced derived variables)
    void process_variables();
    // copies initial state values
    void process_initial_state();

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
