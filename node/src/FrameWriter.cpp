#include "FrameWriter.hpp"

using std::uint8_t;
using std::uint16_t;

namespace {
  // ID
  constexpr uint8_t ID = 0xde;

  // Radio setup, Adafruit LoRa feather
  constexpr uint8_t RFM95_CS = 8;
  constexpr uint8_t RFM95_RST = 4;
  constexpr uint8_t RFM95_INT = 3;
  constexpr float   RFM95_FREQ = 915.0; // MHz

  // Message constants
  constexpr uint16_t LEN = 256;
  constexpr uint8_t END     = 0xC0;
  constexpr uint8_t ESC     = 0xDB;
  constexpr uint8_t ESC_END = 0xDC;
  constexpr uint8_t ESC_ESC = 0xDD;
}

namespace SensorNode {

FrameWriter::FrameWriter() :
    _radio(RH_RF95(RFM95_CS, RFM95_INT)) {}

uint8_t FrameWriter::_slip(uint8_t *vals, int length) {
  uint8_t msg_cursor = 0;

  for(uint8_t raw_cursor = 0; raw_cursor < MAX_RAW; raw_cursor++) {
    if(raw_cursor >= length) {
      msg_cursor++;
      break;
    }

    if(vals[raw_cursor] == END) {
      _msg_buffer[msg_cursor] = ESC;
      msg_cursor++;
      _msg_buffer[msg_cursor] = ESC_END;
      msg_cursor++;
    } else if(vals[raw_cursor] == ESC) {
      _msg_buffer[msg_cursor] = ESC;
      msg_cursor++;
      _msg_buffer[msg_cursor] = ESC_ESC;
      msg_cursor++;
    }
    else {
      _msg_buffer[msg_cursor] = vals[raw_cursor];
      msg_cursor++;
    }
  }

  _msg_buffer[msg_cursor-1] = END;

  return msg_cursor;
}

void FrameWriter::sendMsg(uint8_t *vals, uint16_t length) {
  if(!_isSetUp)
    _setup();

  if(length > MAX_RAW)
    length = MAX_RAW;

  int send_length = _slip(vals, length);

  Serial.println(send_length);
  _radio.send(_msg_buffer, send_length);
  _clearBuffers();
}

void FrameWriter::_clearBuffers() {
  for(uint16_t i = 0; i < MAX_BUF; i++)
    _msg_buffer[i] = 0;
}

bool FrameWriter::_setup() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  delay(100);
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if(!_radio.init())
    return false;

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if(!_radio.setFrequency(RFM95_FREQ))
    return false;
  if(!_radio.setModemConfig(RH_RF95::ModemConfigChoice::Bw125Cr48Sf4096))
    return false;
  _radio.setTxPower(23, false);

  _isSetUp = true;
  return true;
}

}
