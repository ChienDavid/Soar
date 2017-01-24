//
//  SMemEpMemCombinedFunctionalTests.cpp
//  Prototype-UnitTesting
//
//  Created by Alex Turner on 6/23/15.
//  Copyright © 2015 University of Michigan – Soar Group. All rights reserved.
//

#include "SMemEpMemCombinedFunctionalTests.hpp"

#include <string>

void SMemEpMemCombinedFunctionalTests::testSmemEpMemFactorizationCombinationTest()
{
	runTestSetup("testSMemEpMemFactorization");
	agent->RunSelf(100);
	std::string actualResultSMem = agent->ExecuteCommandLine("p @");
	std::string expectedResultSMem = "(@1 ^complete true ^factor @2 ^number 3 [+5.000])\n(@2 ^multiplicity 1 ^value 3 [+6.000])\n(@3 ^complete true ^factor @4 ^number 5 [+3.000])\n(@4 ^multiplicity 1 ^value 5 [+4.000])\n(@5 ^complete true ^factor @6 ^number 7 [+7.000])\n(@6 ^multiplicity 1 ^value 7 [+8.000])\n";
	
	assertTrue_msg(std::string("Unexpected output from SMem:\n") + actualResultSMem, actualResultSMem == expectedResultSMem);
	
	/* Todo: The old test definitely expected a bunch of structure that isn't here. Need to see what should happen with new model of smem and fix. */
#ifndef NO_SVS
	std::string actualResultEpMem = agent->ExecuteCommandLine("epmem --print 97");
	std::string expectedResultEpMem = "========================================\n               Episode 97               \n========================================\n(<id0> ^counter 7 ^io <id1> ^name Factorization ^needs-factorization true ^number-to-factor 7 ^number-to-factor-int 7 ^operator <id3> ^operator* <id3> ^reward-link <id4> ^superstate nil ^svs <id2> ^type state)\n(<id1> ^input-link <id6> ^output-link <id5>)\n(<id2> ^command <id8> ^spatial-scene <id7>)\n(<id3> ^name factor-number ^number-to-factor 7)\n(<id7> ^id world)\n\n";
	assertTrue_msg(std::string("Unexpected output from EpMem:\n") + actualResultEpMem, actualResultEpMem == expectedResultEpMem);
#endif
}