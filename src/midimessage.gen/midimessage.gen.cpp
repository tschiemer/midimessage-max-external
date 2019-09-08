/// @file
///	@copyright	Copyright 2019 Philip Tschiemer
///	@license	MIT License, see project root

#include "c74_min.h"
#include <midimessage/stringifier.h>

using namespace c74::min;
using namespace MidiMessage;


class midimessage_gen : public object<midimessage_gen> {
public:
    MIN_DESCRIPTION	{"Generate MIDI Message"};
    MIN_TAGS		{"midi"};
    MIN_AUTHOR		{"Philip Tschiemer"};
    MIN_RELATED		{"midimessage.parse, midiout"};

    inlet<>  input	{ this, "(anything) list of arguments used for MIDI message genereation" };
    outlet<> output	{ this, "(anything) output raw MIDI message as list of byte integers" };
    outlet<> error { this, "(anything) error output"};

    attribute<bool> runningstatus { this, "runningstatus", false, range { false, true }, title {"Running Status"}, description {"Enable or disable running status explicitly (default = off)"} };

    // post to max window == but only when the class is loaded the first time
//    message<> maxclass_setup { this, "maxclass_setup",
//                               MIN_FUNCTION {
//                                       return {};
//                               }
//    };

    message<threadsafe::yes> anything { this, "anything", "Operate on the list. Either add it to the collection or calculate the mean.",
        MIN_FUNCTION {

                // convert argument list into usable form
                // https://stackoverflow.com/questions/26032039/convert-vectorstring-into-char-c
                std::vector < string > strings = from_atoms < std::vector < string >> (args);
                std::vector<uint8_t*> cstrings;
                cstrings.reserve(strings.size());

                for(auto& s: strings){
                    cstrings.push_back((uint8_t*)&s[0]);
                }


                // try parse arguments to populate message
                Stringifier stringifier;

                uint8_t sysexBuffer[128];
                Message_t msg;
                msg.Data.SysEx.ByteData = sysexBuffer;

                uint8_t ** argv = cstrings.data();
                uint8_t argc = cstrings.size();

                int resultCode = stringifier.fromArgs( &msg,  argc, argv );


                if (StringifierResultOk != resultCode){

                    atoms err;
                    err.reserve(args.size() + 1);

                    switch (resultCode) {
                        case StringifierResultGenericError:
                            err.push_back("Generic error");
                            break;
                        case StringifierResultInvalidValue:
                            err.push_back("Invalid value");
                            break;
                        case StringifierResultWrongArgCount:
                            err.push_back("Wrong arg count");
                            break;
                        case StringifierResultNoInput:
                            err.push_back("No input");
                            break;
                    }

                    err.insert( err.end(), args.begin(), args.end() );

                    error.send(err);

                    return {};

                }

                // try to generate raw bytes from message template
                uint8_t bytes[128];
                uint8_t length = pack( bytes, &msg );

                if (length == 0){
                    return {};
                }


                // optionally remove first (control) byte if runing status enabled and conditions met
                static uint8_t runningStatusState = MidiMessage_RunningStatusNotSet;

                uint8_t * start = bytes;

                if (runningstatus && updateRunningStatus( &runningStatusState, bytes[0] )){
                    start = &start[1];
                    length--;
                }


                // convert final byte message into integer list and send to main output
                atoms result;

                for(auto i = 0; i < length; i++){
                    result.push_back((int)start[i]);
                }

                output.send(result);



                return {};
        }
    };

};


MIN_EXTERNAL(midimessage_gen);