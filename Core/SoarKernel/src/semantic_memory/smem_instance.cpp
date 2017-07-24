/*
 * smem_install.cpp
 *
 *  Created on: Aug 21, 2016
 *      Author: mazzin
 */
#include "semantic_memory.h"
#include "smem_settings.h"
#include "smem_timers.h"
#include "smem_db.h"

#include "VariadicBind.h"

#include "agent.h"
#include "dprint.h"
#include "ebc.h"
#include "episodic_memory.h"
#include "instantiation.h"
#include "mem.h"
#include "preference.h"
#include "production.h"
#include "symbol_manager.h"
#include "test.h"
#include "working_memory.h"
#include "working_memory_activation.h"

void SMem_Manager::install_buffered_triple_list(Symbol* state, wme_set& cue_wmes, symbol_triple_list& my_list, bool meta, bool stripLTILinks)
{
    if (my_list.empty())
    {
        return;
    }
    dprint(DT_SMEM_INSTANCE, "install_buffered_triple_list called in state %y.  Creating architectural instantiation.\n", state);

    instantiation* inst = make_architectural_instantiation_for_memory_system(thisAgent, state, &cue_wmes, &my_list, true);
    for (preference* pref = inst->preferences_generated; pref;)
    {
        if (add_preference_to_tm(thisAgent, pref))
        {
            // and add it to the list of preferences to be removed
            // when the goal is removed
            dprint(DT_SMEM_INSTANCE, "...adding preference %p to WM\n", pref);
            insert_at_head_of_dll(state->id->preferences_from_goal, pref, all_of_goal_next, all_of_goal_prev);
            pref->on_goal_list = true;
            if (meta)
            {
                // if this is a meta wme, then it is completely local
                // to the state and thus we will manually remove it
                // (via preference removal) when the time comes
                state->id->smem_info->smem_wmes->push_back(pref);
            }
        }
        else
        {
            dprint(DT_SMEM_INSTANCE, "...could not add preference %p to WM.  Deallocating.\n", pref);
            if (pref->reference_count == 0)
            {
                preference* previous = pref;
                pref = pref->inst_next;
                possibly_deallocate_preference_and_clones(thisAgent, previous, true);
                continue;
            }
        }
        if (stripLTILinks)
        {
            pref->id->id->LTI_ID = 0;
            pref->id->update_cached_lti_print_str();
            if (pref->value->is_sti())
            {
                pref->value->id->LTI_ID = 0;
                pref->value->update_cached_lti_print_str();
            }

        }
        pref = pref->inst_next;
    }

    if (!meta)
    {
        // Create an architectural instantiation to allow backtracing
        // for chunks, justifications and the GDS.

        instantiation* my_justification_list = NIL;
        dprint(DT_MILESTONES, "Asserting preferences of new architectural instantiation in _smem_process_buffered_wme_list...\n");
        // if any justifications are created, assert their preferences manually
        // (copied mainly from assert_new_preferences with respect to our circumstances)
        if (my_justification_list != NIL)
        {
            preference* just_pref = NIL;
            instantiation* next_justification = NIL;

            for (instantiation* my_justification = my_justification_list;
                my_justification != NIL;
                my_justification = next_justification)
            {
                next_justification = my_justification->next;

                if (my_justification->in_ms)
                {
                    insert_at_head_of_dll(my_justification->prod->instantiations, my_justification, next, prev);
                }

                for (just_pref = my_justification->preferences_generated; just_pref != NIL;)
                {
                    if (add_preference_to_tm(thisAgent, just_pref))
                    {
                        if (wma_enabled(thisAgent))
                        {
                            wma_activate_wmes_in_pref(thisAgent, just_pref);
                        }
                    }
                    else
                    {
                        if (just_pref->reference_count == 0)
                        {
                            preference* previous = just_pref;
                            just_pref = just_pref->inst_next;
                            possibly_deallocate_preference_and_clones(thisAgent, previous, true);
                            continue;
                        }
                    }

                    just_pref = just_pref->inst_next;
                }
            }
        }
    }
}

void SMem_Manager::install_recall_buffer(Symbol* state, wme_set& cue_wmes, symbol_triple_list& meta_wmes, symbol_triple_list& retrieval_wmes, bool stripLTILinks)
{
    dprint(DT_SMEM_INSTANCE, "Installing meta wme buffer.\n");
    install_buffered_triple_list(state, cue_wmes, meta_wmes, true, false);
    dprint(DT_SMEM_INSTANCE, "Installing retrieved wme buffer.\n");
    install_buffered_triple_list(state, cue_wmes, retrieval_wmes, false, stripLTILinks);
}

void SMem_Manager::add_triple_to_recall_buffer(symbol_triple_list& my_list, Symbol* id, Symbol* attr, Symbol* value)
{
    symbol_triple* new_triple;
    thisAgent->memoryManager->allocate_with_pool(MP_sym_triple, &new_triple);
    new_triple->id = id;
    new_triple->attr = attr;
    new_triple->value = value;
    thisAgent->symbolManager->symbol_add_ref(id);
    thisAgent->symbolManager->symbol_add_ref(attr);
    thisAgent->symbolManager->symbol_add_ref(value);
    my_list.push_back(new_triple);
    dprint(DT_SMEM_INSTANCE, "Adding (%y ^%y %y) to recall buffer.\n", id, attr, value);
}

void SMem_Manager::clear_instance_mappings()
{
    lti_to_sti_map.clear();
    iSti_to_lti_map.clear();
    dprint(DT_SMEM_INSTANCE, "Clearing iSti/LTI mappings %d %d.\n", lti_to_sti_map.size(), iSti_to_lti_map.size());
}

Symbol* SMem_Manager::get_current_iSTI_for_LTI(uint64_t pLTI_ID, goal_stack_level pLevel, char pChar)
{
    id_to_sym_map::iterator lIter;

    dprint(DT_SMEM_INSTANCE, "Getting current iSTI for lti ID %u at level %d.\n", pLTI_ID, static_cast<int64_t>(pLevel));
    lIter = lti_to_sti_map.find(pLTI_ID);

    if (lIter != lti_to_sti_map.end())
    {
        thisAgent->symbolManager->symbol_add_ref(lIter->second);
        dprint(DT_SMEM_INSTANCE, "-> returning existing symbol %y.\n",lIter->second);
        return (lIter->second);
    } else {
        Symbol* return_val;
        return_val = thisAgent->symbolManager->make_new_identifier(pChar, pLevel, NIL);
        return_val->id->level = pLevel;
        return_val->id->promotion_level = pLevel;
        return_val->id->LTI_ID = pLTI_ID;
        return_val->id->smem_valid = smem_validation;
        lti_to_sti_map[pLTI_ID] = return_val;
        dprint(DT_SMEM_INSTANCE, "-> returning newly created symbol %y.\n",return_val);
        return return_val;
    }
}


uint64_t SMem_Manager::get_current_LTI_for_iSTI(Symbol* pISTI, bool useLookupTable, bool pOverwriteOldLinkToLTM)
{
    uint64_t returnVal = 0;

    if (useLookupTable)
    {
        sym_to_id_map::iterator lIter;

        dprint(DT_SMEM_INSTANCE, "Getting current LTI ID for STI %y.\n", pISTI);
        lIter = iSti_to_lti_map.find(pISTI);

        if (lIter != iSti_to_lti_map.end())
        {
            dprint(DT_SMEM_INSTANCE, "-> returning existing lti id %u.\n", lIter->second);
            returnVal = lIter->second;
        } else {
            uint64_t lNewID = add_new_LTI();
            iSti_to_lti_map[pISTI]  = lNewID;
            dprint(DT_SMEM_INSTANCE, "-> returning newly generated lti id %u.\n",lNewID);
            returnVal = lNewID;
        }
    } else {
        if (!pISTI->id->LTI_ID)
        {
            uint64_t lNewID = add_new_LTI();
            dprint(DT_SMEM_INSTANCE, "-> returning newly generated lti id %u.\n",lNewID);
            returnVal = lNewID;
        } else {
            dprint(DT_SMEM_INSTANCE, "-> returning existing lti id %u.\n", pISTI->id->LTI_ID);
            returnVal = pISTI->id->LTI_ID;
        }
    }
    if (pOverwriteOldLinkToLTM || !pISTI->id->LTI_ID)
    {
        pISTI->id->LTI_ID = returnVal;
        pISTI->update_cached_lti_print_str();
        pISTI->id->smem_valid = smem_validation;
    }
    return returnVal;

}

void SMem_Manager::install_memory(Symbol* state, uint64_t pLTI_ID, Symbol* sti, bool activate_lti, symbol_triple_list& meta_wmes, symbol_triple_list& retrieval_wmes, smem_install_type install_type, uint64_t depth, std::set<uint64_t>* visited)
{
    ////////////////////////////////////////////////////////////////////////////
    timers->ncb_retrieval->start();
    ////////////////////////////////////////////////////////////////////////////

    // get the ^result header for this state
    Symbol* result_header = NULL;
    if (install_type == wm_install)
    {
        result_header = state->id->smem_info->result_wme->value;
    }
    dprint(DT_SMEM_INSTANCE, "Install memory called for %y %u %y.\n", state, pLTI_ID, sti);
    // get identifier if not known
    bool sti_created_here = false;
    if (install_type == wm_install)
    {
        if (sti == NIL)
        {
            sti = get_current_iSTI_for_LTI(pLTI_ID, result_header->id->level);
            sti_created_here = true;
        }
    }
    else
    {
        if (sti == NIL)
        {
            sti = thisAgent->symbolManager->make_new_identifier('L', NO_WME_LEVEL, NIL);
            sti->id->level = NO_WME_LEVEL;
            sti->id->promotion_level = NO_WME_LEVEL;
            sti->id->LTI_ID = pLTI_ID;
            sti->id->smem_valid = smem_validation;
            sti_created_here = true;
        }
    }
    // activate lti
    if (activate_lti)
    {
        lti_activate(pLTI_ID, true);
    }

    dprint(DT_SMEM_INSTANCE, "...installing meta wmes for %y\n", sti);
    // point retrieved to lti
    if (install_type == wm_install)
    {
        if (visited == NULL)
        {
            add_triple_to_recall_buffer(meta_wmes, result_header, thisAgent->symbolManager->soarSymbols.smem_sym_retrieved, sti);
        }
        else
        {
            add_triple_to_recall_buffer(meta_wmes, result_header, thisAgent->symbolManager->soarSymbols.smem_sym_depth_retrieved, sti);
        }
    }

    /* Not sure if this is still needed with this new implementation of smem*/
    if (sti_created_here && install_type == wm_install)
    {
        // if the identifier was created above we need to
        // remove a single ref count AFTER the wme
        // is added (such as to not deallocate the symbol
        // prematurely)
        thisAgent->symbolManager->symbol_remove_ref(&sti);
    }

    dprint(DT_SMEM_INSTANCE, "...installing children of %y\n", sti);
    bool triggered = false;

    /* This previously would only return the children if there were no impasse wmes, input wmes and slots for sti */
        if (visited == NULL)
        {
            triggered = true;
            visited = new std::set<uint64_t>;
        }

        Symbol* attr_sym;
        Symbol* value_sym;

    std::set<Symbol*> children;

        // get direct children: attr_type, attr_hash, value_type, value_hash, value_letter, value_num, value_lti

    //attr_type, attr_hash, value_type, value_hash, value_lti"
    struct result_t {
        byte attr_type;
        smem_hash_id attr_hash;
        byte value_type;
        smem_hash_id value_hash;
        uint64_t value_lti;
    };

    std::packaged_task<std::vector<result_t>()> expand([this, pLTI_ID, depth] {
        std::vector<result_t> results;
        auto sql = sqlite_thread_guard(SQL->web_expand);

        sql->bind(1, pLTI_ID);

        while (sql->executeStep())
            results.push_back   ({
                static_cast<byte>(sql->getColumn(0).getInt()),
                sql->getColumn(1).getUInt64(),
                static_cast<byte>(sql->getColumn(2).getInt()),
                sql->getColumn(3).getUInt64(),
                sql->getColumn(4).getUInt64()
                                });

        return results;
    });

    auto results = JobQueue->post(expand).get();
    for (const auto& r : results)
        {
            // make the identifier symbol irrespective of value type
        Symbol* attr_sym = rhash_(r.attr_type, r.attr_hash);
        Symbol* value_sym = nullptr;

        if (r.value_lti != SMEM_AUGMENTATIONS_NULL)
            {
            dprint(DT_SMEM_INSTANCE, "Child LTI augmentation found.  Getting STI for lti_id %u...", r.value_lti);
            value_sym = get_current_iSTI_for_LTI(r.value_lti, sti->id->level, 'L');
                dprint_noprefix(DT_SMEM_INSTANCE, "%y\n", value_sym);
                if (depth > 1)
                {
                //visited->insert((*iterator)->id->LTI_ID);
                //install_memory(state, (*iterator)->id->LTI_ID, (*iterator), false, meta_wmes, retrieval_wmes, install_type, depth - 1, visited);//choosing not to bla children of retrived node
                    dprint(DT_SMEM_INSTANCE, "Depth parameter > 1, so adding children of %y to add list.\n", value_sym);
                    children.insert(value_sym);
                }
            }
            else
            {
            dprint(DT_SMEM_INSTANCE, "Child constant augmentation found.  Getting constant for value hash %d %u...", r.value_type, r.value_hash);
            value_sym = rhash_(r.value_type, r.value_hash);
                dprint_noprefix(DT_SMEM_INSTANCE, "%y\n", value_sym);
            }

            // add wme
            add_triple_to_recall_buffer(retrieval_wmes, sti, attr_sym, value_sym);

            // deal with ref counts - attribute/values are always created in this function
            // (thus an extra ref count is set before adding a wme)
            if (install_type == fake_install && sti_created_here)
            {
                thisAgent->symbolManager->symbol_remove_ref(&sti);
            }
            thisAgent->symbolManager->symbol_remove_ref(&attr_sym);
            thisAgent->symbolManager->symbol_remove_ref(&value_sym);
        }

        //Attempt to find children for the case of depth.
        std::set<Symbol*>::iterator iterator;
        std::set<Symbol*>::iterator end = children.end();
        dprint(DT_SMEM_INSTANCE, "...processing add list of children of %y\n", sti);
        for (iterator = children.begin(); iterator != end; ++iterator)
        {
            if (visited->find((*iterator)->id->LTI_ID) == visited->end())
            {
                visited->insert((*iterator)->id->LTI_ID);
                // In original async branch, install_memory used (settings->activate_on_query->get_value() == on) instead of false for 4th arg
                install_memory(state, (*iterator)->id->LTI_ID, (*iterator), false, meta_wmes, retrieval_wmes, install_type, depth - 1, visited);
            }
        }
        dprint(DT_SMEM_INSTANCE, "Done installing memory called for %y %u %y.\n", state, pLTI_ID, sti);

    if (triggered)
    {
        delete visited;
    }

    ////////////////////////////////////////////////////////////////////////////
    timers->ncb_retrieval->stop();
    ////////////////////////////////////////////////////////////////////////////
}
