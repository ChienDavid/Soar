/////////////////////////////////////////////////////////////////
// sp command file.
//
// Author: Jonathan Voigt, voigtjr@gmail.com
// Date  : 2004
//
/////////////////////////////////////////////////////////////////

#include <portability.h>

#include "cli_CommandLineInterface.h"

#include "cli_Commands.h"
#include "sml_AgentSML.h"

#include "agent.h"
#include "production.h"
#include "symtab.h"
#include "rete.h"
#include "parser.h"

using namespace cli;

// FIXME: copied from gSKI
void setLexerInput(agent *ai_agent, const char  *ai_string)
{
    ai_agent->lexer_input_string = const_cast<char*>(ai_string);
    //whitespace forces immediate read of first line
    ai_agent->current_char = ' ';
    return;
}

bool CommandLineInterface::DoSP(const std::string& productionString) {
    // Load the production
    agent* agnt = m_pAgentSML->GetSoarAgent();
    setLexerInput( agnt, productionString.c_str());
    set_lexer_allow_ids( agnt, false );
    get_lexeme( agnt );

    production* p;
    unsigned char rete_addition_result = 0;
    p = parse_production( agnt, &rete_addition_result );

    set_lexer_allow_ids( agnt, true );
    setLexerInput( agnt, NULL); 

    if (!p) { 
        // There was an error, but duplicate production is just a warning
        if (rete_addition_result != DUPLICATE_PRODUCTION) {
          return SetError("Production addition failed.");
        }
        // production ignored
        m_NumProductionsIgnored += 1;
    } else {
        if (!m_SourceFileStack.empty())
            p->filename = make_memory_block_for_string(agnt, m_SourceFileStack.top().c_str());

        // production was sourced
        m_NumProductionsSourced += 1;
        if (m_RawOutput) {
            m_Result << '*';
        }
    }
    return true;
}

