#include "axiom_free_task.h"

#include "../plugins/plugin.h"
#include "../tasks/root_task.h"

#include <memory>

using namespace std;
using utils::ExitCode;

namespace tasks {
    AxiomFreeTask::AxiomFreeTask(
    const shared_ptr<AbstractTask> &parent)
    : DelegatingTask(parent) {

        parent_var_count = parent->get_num_variables();
        get_axiom_layer_count();
        add_new_variables();
        if (axiom_layer_count != 0) { // do not modify tasks without axioms
            add_new_actions();
        }
        modify_existing_actions();
        modify_initial_state();
        modify_goal();
    }

void AxiomFreeTask::get_axiom_layer_count() {

    int layer = -1;
    for (int i = 0; i < parent->get_num_variables(); i++) {
        if (parent->get_variable_axiom_layer(i) > layer) {
            layer = parent->get_variable_axiom_layer(i);
        }
    }
    axiom_layer_count = layer + 1;
}

void AxiomFreeTask::add_new_variables() {

    vector<ExplicitVariable> new_variables;

    vector<string> var_new_fact_names;
    var_new_fact_names.push_back("Atom new");
    var_new_fact_names.push_back("NegatedAtom new");
    ExplicitVariable var_new = { 2, "new", var_new_fact_names, -1, 1 };
    new_variables.push_back(var_new);

    for (int i = 0; i <= axiom_layer_count; i++) {
        vector<string> var_fixed_fact_names;
        var_fixed_fact_names.push_back("Atom fixed" + std::to_string(i));
        var_fixed_fact_names.push_back("NegatedAtom fixed" + std::to_string(i));
        ExplicitVariable var_fixed = { 2, "fixed" + std::to_string(i), var_fixed_fact_names, -1, i == 0 ? 0 : 1 };
        new_variables.push_back(var_fixed);
    }

    for (int i = 1; i <= axiom_layer_count; i++) {
        vector<string> var_done_fact_names;
        var_done_fact_names.push_back("Atom done" + std::to_string(i));
        var_done_fact_names.push_back("NegatedAtom done" + std::to_string(i));
        ExplicitVariable var_done = { 2, "done" + std::to_string(i), var_done_fact_names, -1, 1 };
        new_variables.push_back(var_done);
    }

    // copy existing variables
    for (int i = 0; i < parent->get_num_variables(); i++) {
        vector<string> fact_names;
        fact_names.reserve(parent->get_variable_domain_size(i));
        for (int j = 0; j < parent->get_variable_domain_size(i); j++) {
            fact_names.push_back(parent->get_fact_name(*new FactPair(i, j)));
        }

        ExplicitVariable variable_copy = {parent->get_variable_domain_size(i), parent->get_variable_name(i),
                                          fact_names, -1, parent->get_variable_default_axiom_value(i)};
        variables.push_back(variable_copy);
    }

    if (axiom_layer_count == 0) { // do not modify tasks without axioms
        return;
    }

    // append new variables to (copied and modified) existing variables
    variables.insert(variables.end(), new_variables.begin(), new_variables.end());
}

vector<int> AxiomFreeTask::get_axioms_in_layer(int layer) {

    vector<int> axioms_in_layer;
    for (int i = 0; i < parent->get_num_variables(); i++) {
        if (parent->get_variable_axiom_layer(i) == layer - 1) {
            axioms_in_layer.push_back(i);
        }
    }
    return axioms_in_layer;
}

vector<vector<FactPair>> AxiomFreeTask::collect_axiom_conditions(int axiom, int value) {

    vector<vector<FactPair>> all_conds;

    for (int i = 0; i < parent->get_num_axioms(); i++) {
        assert (parent->get_num_operator_effects(i, true) == 1);
        FactPair axiom_effect = parent->get_operator_effect(i, 0, true);

        if (axiom == axiom_effect.var && value == axiom_effect.value) {
            assert (parent->get_num_operator_preconditions(i, true) == 1);
            vector<FactPair> conds;
            conds.push_back(parent->get_operator_precondition(i, 0, true));

            int effcond_count = parent->get_num_operator_effect_conditions(i, 0, true);
            for (int j = 0; j < effcond_count; j++) {
                conds.push_back(parent->get_operator_effect_condition(i, 0, j, true));
            }
            all_conds.push_back(conds);
        }
    }

    return all_conds;
}

void AxiomFreeTask::add_new_actions() {

    for (int i = 1; i <= axiom_layer_count; i++) {
        // add stratum action
        vector<ExplicitEffect> stratum_effects;
        ExplicitEffect effect_done = { parent_var_count + axiom_layer_count + i + 1, 0, vector<FactPair>{} };
        stratum_effects.push_back(effect_done);

        vector<int> axioms_in_layer = get_axioms_in_layer(i);
        for (unsigned int j = 0; j < axioms_in_layer.size(); j++) {
            vector<vector<FactPair>> value_0_effcond = collect_axiom_conditions(axioms_in_layer[j], 0);
            vector<vector<FactPair>> value_1_effcond = collect_axiom_conditions(axioms_in_layer[j], 1);

            if (parent->get_variable_default_axiom_value(axioms_in_layer[j]) == 1 && !value_0_effcond.empty()) {
                for (unsigned int k = 0; k < value_0_effcond.size(); k++) {
                    ExplicitEffect derived_to_0_effect = { *new FactPair(axioms_in_layer[j], 0), value_0_effcond.at(k) };
                    ExplicitEffect new_after_derived_to_0_effect = { *new FactPair(parent_var_count, 0), value_0_effcond.at(k) };
                    stratum_effects.push_back(derived_to_0_effect);
                    stratum_effects.push_back(new_after_derived_to_0_effect);
                }
            }
            if (parent->get_variable_default_axiom_value(axioms_in_layer[j]) == 0 && !value_1_effcond.empty()) {
                for (unsigned int k = 0; k < value_1_effcond.size(); k++) {
                    ExplicitEffect derived_to_1_effect = { *new FactPair(axioms_in_layer[j], 1), value_1_effcond.at(k) };
                    ExplicitEffect new_after_derived_to_1_effect = { *new FactPair(parent_var_count, 0), value_1_effcond.at(k) };
                    stratum_effects.push_back(derived_to_1_effect);
                    stratum_effects.push_back(new_after_derived_to_1_effect);
                }
            }
        }

        vector<FactPair> stratum_precond;
        stratum_precond.push_back(*new FactPair(parent_var_count + i, 0));
        stratum_precond.push_back(*new FactPair(parent_var_count + i + 1, 1));

        ExplicitOperator stratum_action = {stratum_precond, stratum_effects, 0, "stratum" + std::to_string(i), false};
        new_actions.push_back(stratum_action);

        // add fixpoint action
        vector<ExplicitEffect> fixpoint_effects;
        vector<FactPair> fixed_effcond;
        fixed_effcond.push_back(*new FactPair(parent_var_count, 1));

        ExplicitEffect fixed_to_0_effect = { *new FactPair(parent_var_count + i + 1, 0), fixed_effcond };
        ExplicitEffect new_to_1_effect = { parent_var_count, 1, vector<FactPair>{} };
        ExplicitEffect done_to_1_effect = { parent_var_count + axiom_layer_count + i + 1, 1, vector<FactPair>{} };

        fixpoint_effects.push_back(fixed_to_0_effect);
        fixpoint_effects.push_back(new_to_1_effect);
        fixpoint_effects.push_back(done_to_1_effect);

        vector<FactPair> fixpoint_precond;
        fixpoint_precond.push_back(*new FactPair(parent_var_count + axiom_layer_count + i + 1, 0));

        ExplicitOperator fixpoint_action = {fixpoint_precond, fixpoint_effects, 0, "fixpoint" + std::to_string(i), false};
        new_actions.push_back(fixpoint_action);
    }
}

int AxiomFreeTask::get_max_layer_in_precondition(ExplicitOperator action) {

    int k = 0;

    for (unsigned int i = 0; i < action.preconditions.size(); i++) {
        if (parent->get_variable_axiom_layer(action.preconditions.at(i).var) + 1 > k) {
            k = parent->get_variable_axiom_layer(action.preconditions.at(i).var) + 1;
        }
    }

    return k;
}

int AxiomFreeTask::get_min_layer_in_effect(ExplicitOperator action) {

    int m = axiom_layer_count + 1;

    for (int i = 0; i < parent->get_num_axioms(); i++) {
        ExplicitOperator axiomAction = parent->get_operator_or_axiom(i, true);

        // check precondition
        for (unsigned int j = 0; j < axiomAction.preconditions.size(); j++) {
            for (unsigned int k = 0; k < action.effects.size(); k++) {
                if (action.effects.at(k).fact.var == axiomAction.preconditions.at(j).var
                    && parent->get_variable_axiom_layer(axiomAction.effects.at(0).fact.var) + 1 < m) {
                    m = parent->get_variable_axiom_layer(axiomAction.effects.at(0).fact.var) + 1;
                }
            }
        }

        // check effect condition
        for (unsigned int j = 0; j < axiomAction.effects.size(); j++) {
            for (unsigned int k = 0; k < axiomAction.effects.at(j).conditions.size(); k++) {
                for (unsigned int l = 0; l < action.effects.size(); l++) {
                    if (action.effects.at(l).fact.var == axiomAction.effects.at(j).conditions.at(k).var
                        && parent->get_variable_axiom_layer(axiomAction.effects.at(j).fact.var) + 1 < m) {
                        m = parent->get_variable_axiom_layer(axiomAction.effects.at(j).fact.var) + 1;
                    }
                }
            }
        }
    }

    return m;
}

void AxiomFreeTask::modify_existing_actions() {

    // copy existing actions
    for (int i = 0; i < parent->get_num_operators(); i++) {
        ExplicitOperator action = parent->get_operator_or_axiom(i, false);

        vector<FactPair> precond;
        precond.reserve(action.preconditions.size());
        for (unsigned int j = 0; j < action.preconditions.size(); j++) {
            precond.push_back(*new FactPair(action.preconditions.at(j).var, action.preconditions.at(j).value));
        }

        vector<ExplicitEffect> effects;
        effects.reserve(action.effects.size());
        for (unsigned int j = 0; j < action.effects.size(); j++) {
            vector<FactPair> effcond;
            effcond.reserve(action.effects.at(j).conditions.size());
            for (unsigned int k = 0; k < action.effects.at(j).conditions.size(); k++) {
                effcond.push_back(*new FactPair(action.effects.at(j).conditions.at(k).var, action.effects.at(j).conditions.at(k).value));
            }
            effects.push_back({ *new FactPair(action.effects.at(j).fact.var, action.effects.at(j).fact.value), effcond });
        }

        ExplicitOperator action_copy = {precond, effects, action.cost, action.name, false};
        actions.push_back(action_copy);
    }

    if (axiom_layer_count == 0) { // do not modify tasks without axioms
        return;
    }

    // modify copied actions
    for (unsigned int i = 0; i < actions.size(); i++) {
        int k = get_max_layer_in_precondition(actions.at(i));
        actions.at(i).preconditions.push_back(*new FactPair(parent_var_count + k + 1, 0));

        int m = get_min_layer_in_effect(actions.at(i));
        for (; m <= axiom_layer_count; m++) {
            ExplicitEffect fixed_to_1_effect = { *new FactPair(parent_var_count + m + 1, 1), vector<FactPair>{} };
            actions.at(i).effects.push_back(fixed_to_1_effect);
            ExplicitEffect done_to_1_effect = { *new FactPair(parent_var_count + axiom_layer_count + m + 1, 1), vector<FactPair>{} };
            actions.at(i).effects.push_back(done_to_1_effect);

            vector<int> axioms = get_axioms_in_layer(m);
            for (unsigned int j = 0; j < axioms.size(); j++) {
                ExplicitEffect axiom_to_default_effect = { *new FactPair(axioms.at(j), parent->get_variable_default_axiom_value(axioms.at(j))), vector<FactPair>{} };
                actions.at(i).effects.push_back(axiom_to_default_effect);
            }
        }
    }

    // append new actions to (copied and modified) existing actions
    actions.insert(actions.end(), new_actions.begin(), new_actions.end());
}

void AxiomFreeTask::modify_initial_state() {

    initial_state_values.resize(parent->get_num_variables(),0);
    for (int i = 0; i < parent->get_num_variables(); i++) {
        initial_state_values.at(i) = parent->get_variable_default_axiom_value(i);
    }

    if (axiom_layer_count == 0) { // do not modify tasks without axioms
        return;
    }

    for (int i = 0; i < axiom_layer_count * 2 + 2; i++) {
        if (i == 1) {
            initial_state_values.push_back(0);
        } else {
            initial_state_values.push_back(1);
        }
    }
}

void AxiomFreeTask::modify_goal() {

    int k = 0;

    for (int i = 0; i < parent->get_num_goals(); i++) {
        goals.push_back(parent->get_goal_fact(i));
        if (parent->get_variable_axiom_layer(parent->get_goal_fact(i).var) + 1 > k) {
            k = parent->get_variable_axiom_layer(parent->get_goal_fact(i).var) + 1;
        }
    }

    if (axiom_layer_count == 0) { // do not modify tasks without axioms
        return;
    }

    goals.push_back(*new FactPair(parent_var_count + k + 1, 0));
}

int AxiomFreeTask::get_num_variables() const {
    return variables.size();
}

string AxiomFreeTask::get_variable_name(int var) const {
    return get_variable(var).name;
}

int AxiomFreeTask::get_variable_domain_size(int var) const {
    return get_variable(var).domain_size;
}

int AxiomFreeTask::get_variable_axiom_layer(int var) const {
    return -1;
}

int AxiomFreeTask::get_variable_default_axiom_value(int var) const {
    assert(false);
    return get_variable(var).axiom_default_value;
}

string AxiomFreeTask::get_fact_name(const FactPair &fact) const {
    assert(utils::in_bounds(fact.value, get_variable(fact.var).fact_names));
    return get_variable(fact.var).fact_names[fact.value];
}

bool AxiomFreeTask::are_facts_mutex(const FactPair &fact1, const FactPair &fact2) const {
    if (fact1.var >= parent_var_count || fact2.var >= parent_var_count) {
        if (fact1.var == fact2.var) {
            // Same variable: mutex iff different value.
            return fact1.value != fact2.value;
        }
        return false;
    }
    return parent->are_facts_mutex(fact1, fact2);
}

int AxiomFreeTask::get_operator_cost(int index, bool is_axiom) const {
    assert(!is_axiom);
    return get_operator(index).cost;
}

string AxiomFreeTask::get_operator_name(int index, bool is_axiom) const {
    assert(!is_axiom);
    return get_operator(index).name;
}

int AxiomFreeTask::get_num_operators() const {
    return actions.size();
}

int AxiomFreeTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    assert(!is_axiom);
    return get_operator(index).preconditions.size();
}

FactPair AxiomFreeTask::get_operator_precondition(int op_index, int fact_index, bool is_axiom) const {
    assert(!is_axiom);
    const ExplicitOperator &op = get_operator(op_index);
    assert(utils::in_bounds(fact_index, op.preconditions));
    return op.preconditions[fact_index];
}

int AxiomFreeTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    assert(!is_axiom);
    return get_operator(op_index).effects.size();
}

int AxiomFreeTask::get_num_operator_effect_conditions(int op_index, int eff_index, bool is_axiom) const {
    assert(!is_axiom);
    return get_effect(op_index, eff_index).conditions.size();
}

FactPair AxiomFreeTask::get_operator_effect_condition(int op_index, int eff_index, int cond_index, bool is_axiom) const {
    assert(!is_axiom);
    const ExplicitEffect &effect = get_effect(op_index, eff_index);
    assert(utils::in_bounds(cond_index, effect.conditions));
    return effect.conditions[cond_index];
}

FactPair AxiomFreeTask::get_operator_effect(int op_index, int eff_index, bool is_axiom) const {
    assert(!is_axiom);
    return get_effect(op_index, eff_index).fact;
}

int AxiomFreeTask::convert_operator_index_to_parent(int index) const {
    if (index >= parent->get_num_operators()) {
        return -1;
    }
    return index;
}

int AxiomFreeTask::get_num_axioms() const {
    return 0;
}

int AxiomFreeTask::get_num_goals() const {
    return goals.size();
}

FactPair AxiomFreeTask::get_goal_fact(int index) const {
    assert(utils::in_bounds(index, goals));
    return goals[index];
}

vector<int> AxiomFreeTask::get_initial_state_values() const {
    return initial_state_values;
}

void AxiomFreeTask::convert_state_values_from_parent(vector<int> &values) const {

    if (axiom_layer_count == 0) { // do not modify tasks without axioms
        return;
    }

    vector<int> new_values = values;

    for (int i = 0; i < axiom_layer_count * 2 + 2; i++) {
        if (i == 0) {
            new_values.push_back(1);
        } else {
            new_values.push_back(0);
        }
    }

    values = new_values;
}

const ExplicitVariable &AxiomFreeTask::get_variable(int var) const {
    assert(utils::in_bounds(var, variables));
    return variables[var];
}

const ExplicitOperator &AxiomFreeTask::get_operator(int index) const {
    assert(utils::in_bounds(index, actions));
    return actions[index];
}

const ExplicitEffect &AxiomFreeTask::get_effect(int op_id, int effect_id) const {
    const ExplicitOperator &op = get_operator(op_id);
    assert(utils::in_bounds(effect_id, op.effects));
    return op.effects[effect_id];
}

class AxiomFreeTaskFeature : public plugins::TypedFeature<AbstractTask, AxiomFreeTask> {
public:
    AxiomFreeTaskFeature() : TypedFeature("remove_axioms") {
        document_title("Axiom-free task");
        document_synopsis("A root task transformation that removes axioms.");
    }

    virtual shared_ptr<AxiomFreeTask> create_component(const plugins::Options &options, const utils::Context &) const override {
        return make_shared<AxiomFreeTask>(g_root_task);
    }
};

static plugins::FeaturePlugin<AxiomFreeTaskFeature> _plugin;
}
