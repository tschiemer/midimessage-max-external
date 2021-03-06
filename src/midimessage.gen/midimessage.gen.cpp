/// @file
///	@copyright	Copyright 2019 Philip Tschiemer
///	@license	MIT License, see project root

#include "c74_min.h"
#include <midimessage/stringifier.h>
#include <midimessage/commonccs.h>

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

    attribute<bool> runningstatus { this, "runningstatus", false,
                                    range { false, true },
                                    title {"Running Status"},
                                    description {"Enable or disable running status explicitly (default = off)"},
                                    getter{
                                        MIN_GETTER_FUNCTION {
                                            return {m_runningStatusEnabled};
                                        }
                                    },
                                    setter {
                                        MIN_FUNCTION {
                                            m_runningStatusEnabled = args[0];
                                            m_runningStatusState = MidiMessage_RunningStatusNotSet;
                                            return {m_runningStatusEnabled};
                                        }
                                    }
        };

    // post to max window == but only when the class is loaded the first time
//    message<> maxclass_setup { this, "maxclass_setup",
//                               MIN_FUNCTION {
//                                       return {};
//                               }
//    };

    message<threadsafe::yes> anything { this, "anything", "Generation command (list)",
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

                uint8_t sysexBuffer[128];
                Message_t msg;
                msg.Data.SysEx.ByteData = sysexBuffer;

                uint8_t ** argv = cstrings.data();
                uint8_t argc = cstrings.size();

                int resultCode = StringifierResultGenericError;


                if (strcmp((char*)argv[0], "nrpn") == 0){
                    if (argc < 4 || 5 < argc) {
                        generatorError(StringifierResultWrongArgCount, args);
                        return {};
                    }
                    msg.StatusClass = StatusClassControlChange;
                    msg.Channel = atoi((char*)argv[1]);

                    if (msg.Channel > MaxU7) {
                        generatorError(StringifierResultInvalidU7, args);
                        return {};
                    }

                    uint16_t controller = atoi((char*)argv[2]);

                    if (controller > MaxU14) {
                        generatorError(StringifierResultInvalidU14, args);
                        return {};
                    }

                    uint8_t action = 0;
                    uint16_t value = 0;

                    if (strcmp((char*)argv[3], "inc") == 0){

                        action = CcDataIncrement;

                        if (argc == 5) {
                            value = atoi((char*)argv[4]);

                            if (value > MaxU7) {
                                generatorError(StringifierResultInvalidU14, args);
                                return {};
                            }
                        }
                    }
                    else if (strcmp((char*)argv[3], "dec") == 0){

                        action = CcDataDecrement;

                        if (argc == 5) {
                            value = atoi((char*)argv[4]);

                            if (value > MaxU7) {
                                generatorError(StringifierResultInvalidU14, args);
                                return {};
                            }
                        }

                    } else {

                      if (argc != 4) {
                          generatorError(StringifierResultWrongArgCount, args);
                          return {};
                      }

                      action = CcDataEntryMSB;
                      value = atoi((char*)argv[3]);

                      if (value > MaxU14) {
                          generatorError(StringifierResultInvalidU14, args);
                          return {};
                      }
                    }

                    msg.Data.ControlChange.Controller = CcNonRegisteredParameterMSB;
                    msg.Data.ControlChange.Value = (controller >> 7) & DataMask;
                    writeMidiPacket(&msg);

                    msg.Data.ControlChange.Controller = CcNonRegisteredParameterLSB;
                    msg.Data.ControlChange.Value = controller & DataMask;
                    writeMidiPacket(&msg);

                    if (action == CcDataEntryMSB){
                        msg.Data.ControlChange.Controller = CcDataEntryMSB;
                        msg.Data.ControlChange.Value = (value >> 7) & DataMask;
                        writeMidiPacket(&msg);

                        msg.Data.ControlChange.Controller = CcDataEntryLSB;
                        msg.Data.ControlChange.Value = value & DataMask;
                        writeMidiPacket(&msg);
                    } else {
                        msg.Data.ControlChange.Controller = action;
                        msg.Data.ControlChange.Value = value & DataMask;
                        writeMidiPacket(&msg);
                    }

                } else {
                    resultCode = MessagefromArgs( &msg,  argc, argv );

                    if (resultCode == StringifierResultOk) {
                        writeMidiPacket(&msg);
                    } else {
                        generatorError(resultCode, args);
                    }
                }

                return {};
        }
    };

private:
    bool m_runningStatusEnabled = false;
    uint8_t m_runningStatusState = MidiMessage_RunningStatusNotSet;

    void writeMidiPacket( Message_t * msg ){

        // try to generate raw bytes from message template
        uint8_t bytes[128];
        uint8_t length = pack( bytes, msg );

        if (length == 0){
            return;
        }


        uint8_t * start = bytes;

        if (m_runningStatusEnabled && updateRunningStatus( &m_runningStatusState, bytes[0] )){
            start = &start[1];
            length--;
        }


        // convert final byte message into integer list and send to main output
        atoms result;

        for(auto i = 0; i < length; i++){
            result.push_back((int)start[i]);
        }

        output.send(result);
    }

    void generatorError(int resultCode, const atoms& args = {}){


      //                    for(uint8_t i = 1; i < argc; i++){
      //                        argv[i][-1] = ' ';
      ////        post("%d = [%s] of [%s]", i-1, argv[i-1], argv[0]);
      //                    }
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

            case StringifierResultInvalidU4:
                err.push_back("Invalid U4/Nibble Value");
                break;
            case StringifierResultInvalidU7:
                err.push_back("Invalid U7 Value");
                break;
            case StringifierResultInvalidU14:
                err.push_back("Invalid U14 Value");
                break;
            case StringifierResultInvalidU21:
                err.push_back("Invalid U21 Value");
                break;
            case StringifierResultInvalidU28:
                err.push_back("Invalid U28 Value");
                break;
            case StringifierResultInvalidU35:
                err.push_back("Invalid U35 Value");
                break;
            case StringifierResultInvalidHex:
                err.push_back("Invalid Hex Value");
                break;
        }

        err.insert( err.end(), args.begin(), args.end() );

        error.send(err);
    }
};


MIN_EXTERNAL(midimessage_gen);
