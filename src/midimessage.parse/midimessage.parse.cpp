/// @file
///	@copyright	Copyright 2019 Philip Tschiemer
///	@license	MIT License, see project root

#include "c74_min.h"
#include <midimessage/stringifier.h>
#include <midimessage/parser.h>

using namespace c74::min;
using namespace MidiMessage;


class midimessage_parse : public object<midimessage_parse> {
public:
    MIN_DESCRIPTION	{"Parse binary MIDI Message"};
    MIN_TAGS		{"midi"};
    MIN_AUTHOR		{"Philip Tschiemer"};
    MIN_RELATED		{"midimessage.gen, midiin"};

    inlet<>  input	{ this, "(anything) list of raw bytes as integers" };
    outlet<> output	{ this, "(anything) output human readable form of parsed message" };
    outlet<> discarded { this, "(int) discarded bytes output"};

    attribute<bool> runningstatus { this, "runningstatus", false, range { false, true }, title {"Running Status"}, description {"Enable or disable running status explicitly (default = off)"} };
    attribute<bool> outputdiscardedbytes { this, "outputdiscardedbytes", false, range { false, true }, title {"Output discarded bytes"}, description {"Enable or disable output of (otherwise silently) discarded bytes explicitly (default = off)"} };


    midimessage_parse (const atoms& args = {}){

        m_msg.Data.SysEx.ByteData = m_sysexBuffer;

//
//        auto discardedBytesCallback =

//        void (*asdf)(Message_t*) = Lambda::ptr(pm);

        m_parser = new Parser(
            runningstatus,
            m_dataBuffer,
            sizeof(m_dataBuffer),
            &m_msg,
            this,
            [](Message_t * msg, void * context){
                midimessage_parse * self = (midimessage_parse*)context;

                uint8_t str[256];

                Stringifier stringifier;

                int length = stringifier.toString(str, msg);

                if (length == 0){
                    return;
                }

                uint8_t * cur = str;
                atoms result;

                for(auto i = 0; i < length; i++){
                    if (str[i] == ' '){
                        str[i] = '\0';

                        result.push_back( (char*)cur);

                        cur = &str[i+1];
                    }
                }

                result.push_back( (char*)cur);

                self->output(result);
            },
            [](uint8_t * discardedBytes, uint8_t length, void * context) {

                midimessage_parse * self = (midimessage_parse*)context;

                if (self->outputdiscardedbytes == false){
                    return;
                }

                atoms bytes;

                bytes.reserve(length);

                for(auto i = 0; i < length; i++){
                    bytes.push_back((int)discardedBytes[i]);
                }

                self->discarded.send(bytes);
            }
        );

//        m_parser->MessageHandler = [&localOutput](Message_t * msg){
//
//        };

    };

    ~midimessage_parse () {
            delete m_parser;
    };

//    void parsedMessage(Message_t * msg ) {
//
//    };

//    message<> maxclass_setup { this, "maxclass_setup",
//        MIN_FUNCTION {
//
//
//
//            return {};
//        }
//    };


    message<threadsafe::yes> anything { this, "int", "Operate on the list. Either add it to the collection or calculate the mean.",
        MIN_FUNCTION {

//                cout << "recv " << args.size() << endl;
//
//                for(auto v : args){
//                    cout << v;
//                }
//                cout << endl;

            // convert argument list into usable form
            // https://stackoverflow.com/questions/6399090/c-convert-vectorint-to-vectordouble
            std::vector < int > ints = from_atoms < std::vector < int >> (args);
            std::vector<uint8_t> uint8s(ints.begin(), ints.end());

            m_parser->receivedData( uint8s.data(), uint8s.size() );

//                for(auto i = 0; i < uint8s.size(); i++){
//                    cout << (int)uint8s[i];
//                }
//                cout << endl;

            return {};
        }
    };


private:

    uint8_t m_sysexBuffer[128];
    Message_t m_msg;
    uint8_t m_dataBuffer[128];
    Parser * m_parser;

};


MIN_EXTERNAL(midimessage_parse);