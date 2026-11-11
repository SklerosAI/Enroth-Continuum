cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

set(ENGINE_EVENTS_SOURCES
        EvtEnums.cpp
        EvtInstruction.cpp
        EvtProgram.cpp
        EvtVariables.cpp
        MM6ConditionVariables.cpp
        MM6SkillEvent.cpp
        MM6MapByteState.cpp
        MM6AutonoteState.cpp
        MM6QuestBitState.cpp
        MM6DayOfYear.cpp
        MM6MonthOfYear.cpp
        MM6PartyFlying.cpp
        MM6CharacterEventBit.cpp
        MM6GoldEvent.cpp
        MM6NpcEventState.cpp
        MM6ItemEventCompare.cpp
        MM6ItemEventWrite.cpp
        MM6ResistanceEvent.cpp
        MM6HirelingEvent.cpp
        MM6DayCounterEvent.cpp
        MM6CircusPrizeEvent.cpp
        MM6ReputationEvent.cpp
        MM6EventVariableDispatch.cpp
        MM6GiveItemEvent.cpp
        EvtInterpreter.cpp
        Processor.cpp)

set(ENGINE_EVENTS_HEADERS
        EvtInstruction.h
        EvtProgram.h
        EvtVariables.h
        EvtInterpreter.h
        EvtContext.h
        EvtContinuation.h
        EvtEnums.h
        MM6EventRules.h
        MM6EventVariables.h
        MM6ConditionVariables.h
        MM6SkillEvent.h
        MM6MapByteState.h
        MM6AutonoteState.h
        MM6QuestBitState.h
        MM6DayOfYear.h
        MM6MonthOfYear.h
        MM6PartyFlying.h
        MM6CharacterEventBit.h
        MM6GoldEvent.h
        MM6NpcEventState.h
        MM6ItemEventCompare.h
        MM6ItemEventWrite.h
        MM6ResistanceEvent.h
        MM6HirelingEvent.h
        MM6DayCounterEvent.h
        MM6CircusPrizeEvent.h
        MM6ReputationEvent.h
        MM6EventVariableDispatch.h
        MM6GiveItemEvent.h
        Processor.h
        EvtEnumFunctions.h)

add_library(engine_events STATIC ${ENGINE_EVENTS_SOURCES} ${ENGINE_EVENTS_HEADERS})
target_link_libraries(engine_events PUBLIC engine engine_mm6_item_generator tl::generator)
target_check_style(engine_events)

if(OE_BUILD_TESTS)
    add_library(test_engine_events OBJECT Tests/MM6EventFraming_ut.cpp Tests/MM6EventVariables_ut.cpp
        Tests/MM6MapByteState_ut.cpp Tests/MM6AutonoteState_ut.cpp Tests/MM6QuestBitState_ut.cpp
        Tests/MM6DayOfYear_ut.cpp
        Tests/MM6MonthOfYear_ut.cpp
        Tests/MM6PartyFlying_ut.cpp
        Tests/MM6CharacterEventBit_ut.cpp
        Tests/MM6GoldEvent_ut.cpp
        Tests/MM6NpcEventState_ut.cpp
        Tests/MM6ItemEventCompare_ut.cpp
        Tests/MM6ItemEventWrite_ut.cpp
        Tests/MM6ConditionVariables_ut.cpp
        Tests/MM6SkillEvent_ut.cpp
        Tests/MM6ResistanceEvent_ut.cpp
        Tests/MM6HirelingEvent_ut.cpp
        Tests/MM6DayCounterEvent_ut.cpp
        Tests/MM6CircusPrizeEvent_ut.cpp
        Tests/MM6ReputationEvent_ut.cpp)
    target_link_libraries(test_engine_events PUBLIC testing_unit engine_events)
    target_check_style(test_engine_events)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_events)
endif()
