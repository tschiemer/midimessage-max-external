/// @file
///	@copyright	Copyright 2019 Philip Tschiemer
///	@license	MIT License, see project root

#include "c74_min.h"
#include <midimessage/stringifier.h>
#include <midimessage/parser.h>
#include <midimessage/commonccs.h>
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

    struct {
      uint8_t msgCount = 0;
      uint8_t channel = 0;
      uint8_t values[4] = {0,0,0,0};
    } m_nrpn;

    // thanks to https://stackoverflow.com/questions/45554639/is-there-a-way-to-check-if-a-string-can-be-a-float-in-c/45554836
    bool isInteger( uint8_t * str ){
      int ignore;
      int len = 0;
      int ret = sscanf((char*)str, "%i%n", &ignore, &len);
      return (ret == 1 && !str[len]);
    }

    bool isFloat( uint8_t * str ){
      float ignore;
      int len = 0;
      int ret = sscanf((char*)str, "%f%n", &ignore, &len);
      return (ret == 1 && !str[len]);
    }

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


    attribute<bool> nrpnfilter { this, "nrpnfilter", false,
                                           range { false, true },
                                           title {"Filter/detect NRPN sequences"},
                                           description {"Enable or disable filtering/detection of CC sequences ment as NRPN (default = off). NOTE: incomplete CC sequences simliar to 99-98-96 (increment), 99-98-97 (decrement), 99-98-6-38 (data entry) may be silently filtered out."}
    };

    static void messageHandler(Message_t * msg, void * context){
        midimessage_parse * self = (midimessage_parse*)context;


        if (self->nrpnfilter && msg->StatusClass == StatusClassControlChange) {
            if (self->m_nrpn.msgCount == 0 && (msg->Data.ControlChange.Controller == CcNonRegisteredParameterMSB || msg->Data.ControlChange.Controller == CcNonRegisteredParameterLSB)) {
                self->m_nrpn.channel = msg->Channel;
                self->m_nrpn.msgCount = 1;
                if (msg->Data.ControlChange.Controller == CcNonRegisteredParameterMSB){
                  self->m_nrpn.values[0] = msg->Data.ControlChange.Value;
                } else {
                  self->m_nrpn.values[1] = msg->Data.ControlChange.Value;
                }
                return;
            }

            uint8_t nrpnAction = 0;

            if (self->m_nrpn.msgCount > 0 && self->m_nrpn.channel != msg->Channel){
                self->m_nrpn.msgCount = 0;
            } else {
                if (self->m_nrpn.msgCount == 1) {
                    if (msg->Data.ControlChange.Controller == CcNonRegisteredParameterLSB || msg->Data.ControlChange.Controller == CcNonRegisteredParameterMSB) {
                      if (msg->Data.ControlChange.Controller == CcNonRegisteredParameterMSB){
                        self->m_nrpn.values[0] = msg->Data.ControlChange.Value;
                      } else {
                        self->m_nrpn.values[1] = msg->Data.ControlChange.Value;
                      }
                      self->m_nrpn.msgCount = 2;
                      return;
                    } else {
                      self->m_nrpn.msgCount = 0;
                    }
                }
                else if (self->m_nrpn.msgCount == 2) {
                    if (msg->Data.ControlChange.Controller == CcDataEntryMSB) {
                      self->m_nrpn.values[2] = msg->Data.ControlChange.Value;
                      self->m_nrpn.msgCount = 3;
                      return;
                    } else if (msg->Data.ControlChange.Controller == CcDataIncrement) {
                      self->m_nrpn.values[2] = msg->Data.ControlChange.Value;
                      nrpnAction = CcDataIncrement;
                    } else if (msg->Data.ControlChange.Controller == CcDataDecrement) {
                      self->m_nrpn.values[2] = msg->Data.ControlChange.Value;
                      nrpnAction = CcDataDecrement;
                    } else {
                      self->m_nrpn.msgCount = 0;
                    }
                }
                else if (self->m_nrpn.msgCount == 3){
                    if (msg->Data.ControlChange.Controller == CcDataEntryLSB) {
                      self->m_nrpn.values[3] = msg->Data.ControlChange.Value;
                      self->m_nrpn.msgCount = 4;
                      nrpnAction = CcDataEntryMSB;
                    } else {
                      self->m_nrpn.msgCount = 0;
                    }
                }
            }

            if (nrpnAction != 0){

                uint16_t controller = (self->m_nrpn.values[0] << 7) | self->m_nrpn.values[1];

                uint8_t str[64];
                uint8_t length;

                if (nrpnAction == CcDataIncrement) {
                    length = sprintf((char*)str, "nrpn %d %d inc %d", self->m_nrpn.channel, controller, self->m_nrpn.values[2]);
                } else if (nrpnAction == CcDataDecrement) {
                    length = sprintf((char*)str, "nrpn %d %d dec %d", self->m_nrpn.channel, controller, self->m_nrpn.values[2]);
                } else {
                    uint16_t value = (self->m_nrpn.values[2] << 7) | self->m_nrpn.values[3];
                    length = sprintf((char*)str, "nrpn %d %d %d", self->m_nrpn.channel, controller, value);
                }


                uint8_t * cur = str;
                atoms result;

                for(auto i = 0; i < length; i++){
                    if (str[i] == ' '){
                        str[i] = '\0';

                        if (self->isInteger(cur)){
                          result.push_back(atoi((char*)cur));
                        } else if (self->isFloat(cur)){
                          result.push_back(atoll((char*)cur));
                        } else {
                          result.push_back( (char*)cur);
                        }

                        cur = &str[i+1];
                    }
                }

                if (self->isInteger(cur)){
                  result.push_back(atoi((char*)cur));
                } else if (self->isFloat(cur)){
                  result.push_back(atoll((char*)cur));
                } else {
                  result.push_back( (char*)cur);
                }

                self->output(result);

                self->m_nrpn.msgCount = 0;

                return;
            }
        }

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

                if (self->isInteger(cur)){
                  result.push_back(atoi((char*)cur));
                } else if (self->isFloat(cur)){
                  result.push_back(atoll((char*)cur));
                } else {
                  result.push_back( (char*)cur);
                }

                cur = &str[i+1];
            }
        }

        if (self->isInteger(cur)){
          result.push_back(atoi((char*)cur));
        } else if (self->isFloat(cur)){
          result.push_back(atoll((char*)cur));
        } else {
          result.push_back( (char*)cur);
        }

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
