cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

set(ENGINE_EVENTS_SOURCES
        EvtEnums.cpp
        EvtInstruction.cpp
        EvtProgram.cpp
        EvtVariables.cpp
        MM6MapByteState.cpp
        MM6AutonoteState.cpp
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
        MM6MapByteState.h
        MM6AutonoteState.h
        Processor.h
        EvtEnumFunctions.h)

add_library(engine_events STATIC ${ENGINE_EVENTS_SOURCES} ${ENGINE_EVENTS_HEADERS})
target_link_libraries(engine_events PUBLIC engine tl::generator)
target_check_style(engine_events)

if(OE_BUILD_TESTS)
    add_library(test_engine_events OBJECT Tests/MM6EventFraming_ut.cpp Tests/MM6EventVariables_ut.cpp
        Tests/MM6MapByteState_ut.cpp Tests/MM6AutonoteState_ut.cpp)
    target_link_libraries(test_engine_events PUBLIC testing_unit engine_events)
    target_check_style(test_engine_events)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_events)
endif()
