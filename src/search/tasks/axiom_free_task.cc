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

        parent_action_count = parent->get_num_operators();
        get_axiom_layer_count();
        add_new_variables();
        add_new_actions();
        modify_existing_actions();
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
        ExplicitVariable var_fixed = { 2, "fixed" + std::to_string(i), var_fixed_fact_names, -1, 1 };
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

vector<FactPair> AxiomFreeTask::collect_axiom_conditions(int axiom, int value) {

    vector<FactPair> conds;
    for (int i = 0; i < parent->get_num_axioms(); i++) {
        FactPair axiom_effect = parent->get_operator_effect(i, 0, true);

        if (axiom == axiom_effect.var && value == axiom_effect.value) {
            conds.push_back(parent->get_operator_precondition(i, 0, true));

            int effcond_count = parent->get_num_operator_effect_conditions(i, 0, true);
            for (int j = 0; j < effcond_count; j++) {
                conds.push_back(parent->get_operator_effect_condition(i, 0, j, true));
            }
        }
    }

    return conds;
}

void AxiomFreeTask::add_new_actions() {

    for (int i = 1; i <= axiom_layer_count; i++) {
        // add stratum action
        vector<ExplicitEffect> stratum_effects;
        ExplicitEffect effect_done = { parent_action_count + axiom_layer_count + i + 1, 0, vector<FactPair>{ FactPair::no_fact } };
        stratum_effects.push_back(effect_done);

        vector<int> axioms_in_layer = get_axioms_in_layer(i);
        for (unsigned int j = 0; j < axioms_in_layer.size(); j++) {
            vector<FactPair> value_0_effcond = collect_axiom_conditions(axioms_in_layer[j], 0);
            vector<FactPair> value_1_effcond = collect_axiom_conditions(axioms_in_layer[j], 1);

            if (!value_0_effcond.empty()) {
                ExplicitEffect derived_to_0_effect = { *new FactPair(axioms_in_layer[j], 0), value_0_effcond };
                ExplicitEffect new_after_derived_to_0_effect = { *new FactPair(parent_action_count, 0), value_0_effcond };
                stratum_effects.push_back(derived_to_0_effect);
                stratum_effects.push_back(new_after_derived_to_0_effect);
            }
            if (!value_1_effcond.empty()) {
                ExplicitEffect derived_to_1_effect = { *new FactPair(axioms_in_layer[j], 1), value_1_effcond };
                ExplicitEffect new_after_derived_to_1_effect = { *new FactPair(parent_action_count, 0), value_1_effcond };
                stratum_effects.push_back(derived_to_1_effect);
                stratum_effects.push_back(new_after_derived_to_1_effect);
            }
        }

        vector<FactPair> stratum_precond;
        stratum_precond.push_back(*new FactPair(parent_action_count + i, 0));
        stratum_precond.push_back(*new FactPair(parent_action_count + i + 1, 1));

        ExplicitOperator stratum_action = {stratum_precond, stratum_effects, 0, "stratum" + std::to_string(i), false};
        new_actions.push_back(stratum_action);

        // add fixpoint action
        vector<ExplicitEffect> fixpoint_effects;
        vector<FactPair> fixed_effcond;
        fixed_effcond.push_back(*new FactPair(parent_action_count, 1));

        ExplicitEffect fixed_to_0_effect = { *new FactPair(parent_action_count + i + 1, 0), fixed_effcond };
        ExplicitEffect new_to_1_effect = { parent_action_count, 1,  vector<FactPair>{ FactPair::no_fact } };
        ExplicitEffect done_to_1_effect = { parent_action_count + axiom_layer_count + i + 1, 1,  vector<FactPair>{ FactPair::no_fact } };

        fixpoint_effects.push_back(fixed_to_0_effect);
        fixpoint_effects.push_back(new_to_1_effect);
        fixpoint_effects.push_back(done_to_1_effect);

        vector<FactPair> fixpoint_precond;
        fixpoint_precond.push_back(*new FactPair(parent_action_count + axiom_layer_count + i + 1, 0));

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

    // modify copied actions
    for (unsigned int i = 0; i < actions.size(); i++) {
        int k = get_max_layer_in_precondition(actions.at(i));
        actions.at(i).preconditions.push_back(*new FactPair(parent_action_count + k + 1, 0));

        int m = get_min_layer_in_effect(actions.at(i));
        for (; m <= axiom_layer_count; m++) {
            ExplicitEffect fixed_to_1_effect = { *new FactPair(parent_action_count + m + 1, 1), vector<FactPair>{ FactPair::no_fact } };
            actions.at(i).effects.push_back(fixed_to_1_effect);
            ExplicitEffect done_to_1_effect = { *new FactPair(parent_action_count + axiom_layer_count + m + 1, 1), vector<FactPair>{ FactPair::no_fact } };
            actions.at(i).effects.push_back(done_to_1_effect);

            vector<int> axioms = get_axioms_in_layer(m);
            for (unsigned int j = 0; j < axioms.size(); j++) {
                ExplicitEffect axiom_to_default_effect = { *new FactPair(axioms.at(j), parent->get_variable_default_axiom_value(axioms.at(j))), vector<FactPair>{ FactPair::no_fact } };
                actions.at(i).effects.push_back(axiom_to_default_effect);
            }
        }
    }

    // append new actions to (copied and modified) existing actions
    actions.insert(actions.end(), new_actions.begin(), new_actions.end());
}


/*int AxiomFreeTask::get_operator_cost(int index, bool is_axiom) const {
    return 1;
}*/

/*int AxiomFreeTask::convert_operator_index_to_parent(int index) const {
    return -1;
}*/

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
