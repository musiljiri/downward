#include "easy_axiom_free_task.h"

#include "../plugins/plugin.h"
#include "../tasks/root_task.h"

#include <memory>

using namespace std;
using utils::ExitCode;

namespace tasks {
    EasyAxiomFreeTask::EasyAxiomFreeTask(
    const shared_ptr<AbstractTask> &parent)
    : DelegatingTask(parent) {

        parent_var_count = parent->get_num_variables();

        get_vars_to_replace();
        replace_goal();
        replace_axioms();
        replace_actions();
        process_variables();
        process_initial_state();
    }

vector<FactPair> EasyAxiomFreeTask::get_var_conditions(FactPair var) {

    for (int i = 0; i < parent->get_num_axioms(); i++) {
        ExplicitOperator o = parent->get_operator_or_axiom(i, true);
        if (o.effects.at(0).fact.var == var.var && o.effects.at(0).fact.value == var.value) {
            return o.effects.at(0).conditions;
        }
    }

}

void EasyAxiomFreeTask::get_vars_to_replace() {

    for (int i = 0; i < parent_var_count; i++) {
        if (parent->get_variable_axiom_layer(i) == -1) {
            continue;
        }

        for (int value = 0; value < 2; value++) {
            int num_rules = 0;
            bool basic_vars_in_rule = true;
            for (int j = 0; j < parent->get_num_axioms(); j++) {
                ExplicitOperator o = parent->get_operator_or_axiom(j, true);
                if (o.effects.at(0).fact.var == i && o.effects.at(0).fact.value == value) {
                    num_rules++;
                    for (unsigned int k = 0; k < o.effects.at(0).conditions.size(); k++) {
                        if (parent->get_variable_axiom_layer(o.effects.at(0).conditions.at(k).var) != -1) {
                            basic_vars_in_rule = false;
                        }
                    }
                }
            }

            if (num_rules != 1 || !basic_vars_in_rule) {
                continue;
            }

            bool only_value_used = true;
            //check goal
            for (int j = 0; j < parent->get_num_goals(); j++) {
                if (parent->get_goal_fact(j).var == i && parent->get_goal_fact(j).value != value) {
                    only_value_used = false;
                }
            }
            //check axioms
            for (int j = 0; j < parent->get_num_axioms(); j++) {
                ExplicitOperator o = parent->get_operator_or_axiom(j, true);
                for (unsigned int k = 0; k < o.effects.at(0).conditions.size(); k++) {
                    if (o.effects.at(0).conditions.at(k).var == i && o.effects.at(0).conditions.at(k).value != value) {
                        only_value_used = false;
                    }
                }
            }
            //check operators
            for (int j = 0; j < parent->get_num_operators(); j++) {
                ExplicitOperator o = parent->get_operator_or_axiom(j, false);
                for (unsigned int k = 0; k < o.preconditions.size(); k++) {
                    if (o.preconditions.at(k).var == i && o.preconditions.at(k).value != value) {
                        only_value_used = false;
                    }
                }
                for (unsigned int k = 0; k < o.effects.size(); k++) {
                    for (unsigned int l = 0; l < o.effects.at(k).conditions.size(); l++) {
                        if (o.effects.at(k).conditions.at(l).var == i && o.effects.at(k).conditions.at(l).value != value) {
                            only_value_used = false;
                        }
                    }
                }
            }

            if (!only_value_used) {
                continue;
            }
            vars_to_replace.push_back(*new FactPair(i, value));
            break;
        }
    }
}

void EasyAxiomFreeTask::replace_goal() {

    for (int i = 0; i < parent->get_num_goals(); i++) {
        if (std::find(vars_to_replace.begin(), vars_to_replace.end(), parent->get_goal_fact(i)) != vars_to_replace.end()) {
            vector<FactPair> conds = get_var_conditions(parent->get_goal_fact(i));
            goals.insert(goals.end(), conds.begin(), conds.end());
        } else {
            goals.push_back(parent->get_goal_fact(i));
        }
    }
}

void EasyAxiomFreeTask::replace_axioms() {

    for (int i = 0; i < parent->get_num_axioms(); i++) {
        ExplicitOperator axiom = parent->get_operator_or_axiom(i, true);

        // remove axiom if it changes a replaced variable
        if (std::find(vars_to_replace.begin(), vars_to_replace.end(), *new FactPair(axiom.effects.at(0).fact.var, 0)) != vars_to_replace.end() ||
            std::find(vars_to_replace.begin(), vars_to_replace.end(), *new FactPair(axiom.effects.at(0).fact.var, 1)) != vars_to_replace.end()) {
            continue;
        }

        vector<FactPair> precond;
        precond.reserve(axiom.preconditions.size());
        for (unsigned int j = 0; j < axiom.preconditions.size(); j++) {
            precond.push_back(*new FactPair(axiom.preconditions.at(j).var, axiom.preconditions.at(j).value));
        }

        vector<ExplicitEffect> effects;
        effects.reserve(axiom.effects.size());
        for (unsigned int j = 0; j < axiom.effects.size(); j++) {
            vector<FactPair> effcond;
            effcond.reserve(axiom.effects.at(j).conditions.size());
            for (unsigned int k = 0; k < axiom.effects.at(j).conditions.size(); k++) {
                if (std::find(vars_to_replace.begin(), vars_to_replace.end(), axiom.effects.at(j).conditions.at(k)) != vars_to_replace.end()) {
                    vector<FactPair> conds = get_var_conditions(axiom.effects.at(j).conditions.at(k));
                    effcond.insert(effcond.end(), conds.begin(), conds.end());
                } else {
                    effcond.push_back(*new FactPair(axiom.effects.at(j).conditions.at(k).var, axiom.effects.at(j).conditions.at(k).value));
                }
            }
            effects.push_back({ *new FactPair(axiom.effects.at(j).fact.var, axiom.effects.at(j).fact.value), effcond });
        }

        ExplicitOperator axiom_copy = {precond, effects, axiom.cost, axiom.name, true};
        axioms.push_back(axiom_copy);
    }
}

void EasyAxiomFreeTask::replace_actions() {

    for (int i = 0; i < parent->get_num_operators(); i++) {
        ExplicitOperator action = parent->get_operator_or_axiom(i, false);

        vector<FactPair> precond;
        precond.reserve(action.preconditions.size());
        for (unsigned int j = 0; j < action.preconditions.size(); j++) {
            if (std::find(vars_to_replace.begin(), vars_to_replace.end(), action.preconditions.at(j)) != vars_to_replace.end()) {
                vector<FactPair> conds = get_var_conditions(action.preconditions.at(j));
                precond.insert(precond.end(), conds.begin(), conds.end());
            } else {
                precond.push_back(*new FactPair(action.preconditions.at(j).var, action.preconditions.at(j).value));
            }
        }

        vector<ExplicitEffect> effects;
        effects.reserve(action.effects.size());
        for (unsigned int j = 0; j < action.effects.size(); j++) {
            vector<FactPair> effcond;
            effcond.reserve(action.effects.at(j).conditions.size());
            for (unsigned int k = 0; k < action.effects.at(j).conditions.size(); k++) {
                if (std::find(vars_to_replace.begin(), vars_to_replace.end(), action.effects.at(j).conditions.at(k)) != vars_to_replace.end()) {
                    vector<FactPair> conds = get_var_conditions(action.effects.at(j).conditions.at(k));
                    effcond.insert(effcond.end(), conds.begin(), conds.end());
                } else {
                    effcond.push_back(*new FactPair(action.effects.at(j).conditions.at(k).var, action.effects.at(j).conditions.at(k).value));
                }
            }
            effects.push_back({ *new FactPair(action.effects.at(j).fact.var, action.effects.at(j).fact.value), effcond });
        }

        ExplicitOperator action_copy = {precond, effects, action.cost, action.name, false};
        actions.push_back(action_copy);
    }
}

void EasyAxiomFreeTask::process_variables() {

    for (int i = 0; i < parent->get_num_variables(); i++) {
        /*if (std::find(vars_to_replace.begin(), vars_to_replace.end(), *new FactPair(i, 0)) != vars_to_replace.end() ||
            std::find(vars_to_replace.begin(), vars_to_replace.end(), *new FactPair(i, 1)) != vars_to_replace.end()) {
            continue;
        }*/

        vector<string> fact_names;
        fact_names.reserve(parent->get_variable_domain_size(i));
        for (int j = 0; j < parent->get_variable_domain_size(i); j++) {
            fact_names.push_back(parent->get_fact_name(*new FactPair(i, j)));
        }

        ExplicitVariable variable_copy = {parent->get_variable_domain_size(i), parent->get_variable_name(i),
                                          fact_names, parent->get_variable_axiom_layer(i), parent->get_variable_default_axiom_value(i)};
        variables.push_back(variable_copy);
    }
}

void EasyAxiomFreeTask::process_initial_state() {
    initial_state_values = parent->get_initial_state_values();
}

int EasyAxiomFreeTask::get_num_variables() const {
    return variables.size();
}

string EasyAxiomFreeTask::get_variable_name(int var) const {
    return get_variable(var).name;
}

int EasyAxiomFreeTask::get_variable_domain_size(int var) const {
    return get_variable(var).domain_size;
}

int EasyAxiomFreeTask::get_variable_axiom_layer(int var) const {
    return get_variable(var).axiom_layer;
}

int EasyAxiomFreeTask::get_variable_default_axiom_value(int var) const {
    return get_variable(var).axiom_default_value;
}

string EasyAxiomFreeTask::get_fact_name(const FactPair &fact) const {
    assert(utils::in_bounds(fact.value, get_variable(fact.var).fact_names));
    return get_variable(fact.var).fact_names[fact.value];
}

bool EasyAxiomFreeTask::are_facts_mutex(const FactPair &fact1, const FactPair &fact2) const {
    return parent->are_facts_mutex(fact1, fact2);
}

int EasyAxiomFreeTask::get_operator_cost(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).cost;
}

string EasyAxiomFreeTask::get_operator_name(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).name;
}

int EasyAxiomFreeTask::get_num_operators() const {
    return actions.size();
}

int EasyAxiomFreeTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).preconditions.size();
}

FactPair EasyAxiomFreeTask::get_operator_precondition(int op_index, int fact_index, bool is_axiom) const {
    const ExplicitOperator &op = get_operator_or_axiom(op_index, is_axiom);
    assert(utils::in_bounds(fact_index, op.preconditions));
    return op.preconditions[fact_index];
}

int EasyAxiomFreeTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    return get_operator_or_axiom(op_index, is_axiom).effects.size();
}

int EasyAxiomFreeTask::get_num_operator_effect_conditions(int op_index, int eff_index, bool is_axiom) const {
    return get_effect(op_index, eff_index, is_axiom).conditions.size();
}

FactPair EasyAxiomFreeTask::get_operator_effect_condition(int op_index, int eff_index, int cond_index, bool is_axiom) const {
    const ExplicitEffect &effect = get_effect(op_index, eff_index, is_axiom);
    assert(utils::in_bounds(cond_index, effect.conditions));
    return effect.conditions[cond_index];
}

FactPair EasyAxiomFreeTask::get_operator_effect(int op_index, int eff_index, bool is_axiom) const {
    return get_effect(op_index, eff_index, is_axiom).fact;
}

const ExplicitOperator &EasyAxiomFreeTask::get_operator_or_axiom(int index, bool is_axiom) const {
    if (is_axiom) {
        assert(utils::in_bounds(index, axioms));
        return axioms[index];
    } else {
        assert(utils::in_bounds(index, actions));
        return actions[index];
    }
}

int EasyAxiomFreeTask::convert_operator_index_to_parent(int index) const {
    return index;
}

int EasyAxiomFreeTask::get_num_axioms() const {
    return axioms.size();
}

int EasyAxiomFreeTask::get_num_goals() const {
    return goals.size();
}

FactPair EasyAxiomFreeTask::get_goal_fact(int index) const {
    assert(utils::in_bounds(index, goals));
    return goals[index];
}

vector<int> EasyAxiomFreeTask::get_initial_state_values() const {
    return initial_state_values;
}

void EasyAxiomFreeTask::convert_state_values_from_parent(vector<int> &values) const {

    vector<int> new_values = values;
    values = new_values;
}

const ExplicitVariable &EasyAxiomFreeTask::get_variable(int var) const {
    assert(utils::in_bounds(var, variables));
    return variables[var];
}

const ExplicitEffect &EasyAxiomFreeTask::get_effect(int op_id, int effect_id, bool is_axiom) const {
    const ExplicitOperator &op = get_operator_or_axiom(op_id, is_axiom);
    assert(utils::in_bounds(effect_id, op.effects));
    return op.effects[effect_id];
}

class EasyAxiomFreeTaskFeature : public plugins::TypedFeature<AbstractTask, EasyAxiomFreeTask> {
public:
    EasyAxiomFreeTaskFeature() : TypedFeature("remove_easy_axioms") {
        document_title("Easy axiom-free task");
        document_synopsis("A root task transformation that removes easy axioms.");
    }

    virtual shared_ptr<EasyAxiomFreeTask> create_component(const plugins::Options &options, const utils::Context &) const override {
        return make_shared<EasyAxiomFreeTask>(g_root_task);
    }
};

static plugins::FeaturePlugin<EasyAxiomFreeTaskFeature> _plugin;
}
