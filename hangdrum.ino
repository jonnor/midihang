
#include <CapacitiveSensor.h>
#include <MIDIUSB.h>

#include "./hangdrum.hpp"
using namespace hangdrum;

Config config;
struct Sensors {
  CapacitiveSensor capacitive[N_PADS] = {
    CapacitiveSensor(config.sendPin, config.pads[0].pin),
    CapacitiveSensor(config.sendPin, config.pads[1].pin),
    CapacitiveSensor(config.sendPin, config.pads[2].pin),
    CapacitiveSensor(config.sendPin, config.pads[3].pin),
    CapacitiveSensor(config.sendPin, config.pads[4].pin),
    CapacitiveSensor(config.sendPin, config.pads[5].pin),
    CapacitiveSensor(config.sendPin, config.pads[6].pin),
    CapacitiveSensor(config.sendPin, config.pads[7].pin),
    CapacitiveSensor(config.sendPin, config.pads[8].pin),
    CapacitiveSensor(config.sendPin, config.pads[9].pin),
  };
};
Sensors sensors;
State state;
Parser parser;

#ifndef EMULATE_INPUTS
void sendMessagesMidiUSB(MidiEventMessage *messages) {
    for (int i=0; i<N_PADS; i++) {
        const auto &m = messages[i];
        if (m.type == MidiMessageType::Nothing) {
            continue;
        }
        const midiEventPacket_t packet = {
            (uint8_t)m.type,
            0x80 | m.channel,
            m.pitch,
            m.velocity,
        };
        MidiUSB.sendMIDI(packet);
    }
    MidiUSB.flush();
}
#endif

Input readInputs() {
  Input current;

  const uint8_t param = 20;
  current.time = millis();
  for (int i=0; i<N_PADS; i++) {
    const int val = sensors.capacitive[i].capacitiveSensor(param);
    if (val > 0) {
      current.values[i].capacitance = val;
    } else {
      // error/timeout
      current.values[i].capacitance = 0;
    }
  }
}

void setup() {
  //Pin setup
  for (int i=0; i<N_PADS; i++) {
    auto &pad = config.pads[i];
    pinMode(pad.pin, INPUT);
  }

  Serial.begin(115200);
}

void loop(){
  const long beforeRead = millis();
  //Input input = readInputs();
  const long afterRead = millis();

  //state = hangdrum::calculateState(state, input, config);
  //const long afterCalculation = millis();

  while (Serial.available()) {
    Parser::DelayValue val = parser.push(Serial.read());
    if (val.valid()) {
      const long beforeCalculation = millis();
      Input input = {
          { val.value },
          beforeCalculation
      };
      State next = state;
      for (int i=0; i<100; i++) { // waste, to be able to profile
         next = hangdrum::calculateState(state, input, config);
      }
      state = next;
      const long afterCalculation = millis();

      Serial.print("calculating: ");
      Serial.println(afterCalculation-beforeCalculation);
      Serial.flush();
    } else {
      Serial.println("w");
    }

  }

#ifndef EMULATE_INPUTS
  sendMessagesMidiUSB(state.messages);
#endif
  const long afterSend = millis();

  const long readingTime = afterRead-beforeRead;
  //Serial.print("(");
  //Serial.print(readingTime);
  //Serial.print(",");
  //Serial.print(input.values[0].capacitance);
  //Serial.println(")");
}

