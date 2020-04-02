/// @file
///	@copyright	Copyright 2019 Philip Tschiemer
///	@license	MIT License, see project root

#include "c74_min.h"
#include <midimessage/stringifier.h>
#include <midimessage/parser.h>
#include <assert.h>

using namespace c74::min;
using namespace MidiMessage;


class midimessage_parse : public object<midimessage_parse> {

private:

    uint8_t m_sysexBuffer[128];
    Message_t m_msg = {
            .Data.SysEx.ByteData = m_sysexBuffer
    };
    uint8_t m_streamBuffer[128];
    Parser_t m_parser = {
            .RunningStatusEnabled = false,
            .Buffer = m_streamBuffer,
            .MaxLength = 128,
            .Message = &m_msg,
            .MessageHandler = messageHandler,
            .DiscardingDataHandler = discardedHandler,
            .Context = this,
            .Length = 0
      };

public:
    MIN_DESCRIPTION	{"Parse binary MIDI Message"};
    MIN_TAGS		{"midi"};
    MIN_AUTHOR		{"Philip Tschiemer"};
    MIN_RELATED		{"midimessage.gen, midiin"};

    inlet<>  input	{ this, "(anything) list of raw bytes as integers" };
    outlet<> output	{ this, "(anything) output human readable form of parsed message" };
    outlet<> discarded { this, "(int) discarded bytes output"};

    attribute<bool> runningstatus { this, "runningstatus", false,
                                    range { false, true },
                                    title {"Running Status"},
                                    description {"Enable or disable running status explicitly (default = off)"},
                                    getter { MIN_GETTER_FUNCTION {
                                        return {m_parser.RunningStatusEnabled};
                                    }},
                                    setter { MIN_FUNCTION {
                                        m_parser.RunningStatusEnabled = args[0];
                                        parser_reset( &m_parser );
                                        return {m_parser.RunningStatusEnabled};
                                    }}
    };

    attribute<bool> outputdiscardedbytes { this, "outputdiscardedbytes", false,
                                           range { false, true },
                                           title {"Output discarded bytes"},
                                           description {"Enable or disable output of (otherwise silently) discarded bytes explicitly (default = off)"}
    };


    static void messageHandler(Message_t * msg, void * context){
        midimessage_parse * self = (midimessage_parse*)context;

        uint8_t str[256];

        int length = MessagetoString(str, msg);

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
    };

    static void discardedHandler(uint8_t * discardedBytes, uint8_t length, void * context) {

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
    };


    message<threadsafe::yes> anything { this, "list", "Incoming MIDI data bytes",
        MIN_FUNCTION {

            // cout << "anything " << args.size() << endl;

            for (auto i = 0; i < args.size(); i++){
                handleInt(args[i]);
            }

            return {};
        }
    };

    message<threadsafe::yes> handleInt { this, "int", "Incoming MIDI data byte (single)",
  MIN_FUNCTION {

                // cout << "int " << (int)args[0] << endl;

                uint8_t byte = (int)args[0];

                parser_receivedData( &m_parser, &byte, 1 );

                return {};
  }};

    message<threadsafe::yes> reset {this, "reset", "Reset parser state.",
        MIN_FUNCTION {
            parser_reset( &m_parser );
            return {};
        }
    };


};


MIN_EXTERNAL(midimessage_parse);
