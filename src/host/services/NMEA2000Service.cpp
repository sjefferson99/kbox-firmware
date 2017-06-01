/*
  The MIT License

  Copyright (c) 2016 Thomas Sarlandie thomas@sarlandie.net

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include <NMEA2000.h>
#include <N2kMessages.h>
#include <KBoxLogging.h>
#include <KBoxHardware.h>
#include "common/stats/KBoxMetrics.h"
#include "common/nmea/nmea2000.h"
#include "common/signalk/SKUpdate.h"
#include "NMEA2000Service.h"

static NMEA2000Service *handlerContext;

static void handler(const tN2kMsg &msg) {
  DEBUG("Received N2K Message with pgn: %i", msg.PGN);
  handlerContext->publishN2kMessage(msg);
}

void NMEA2000Service::publishN2kMessage(const tN2kMsg& msg) {
  KBoxMetrics.event(KBoxEventNMEA2000MessageReceived);
  NMEA2000Message m(msg);
  sendMessage(m);
}

void NMEA2000Service::setup() {
  // Make sure the CAN transceiver is enabled.
  pinMode(can_standby, OUTPUT);
  digitalWrite(can_standby, 0);

  // FIXME The hardware information should automatically stay up to date.
  NMEA2000.SetProductInformation("2", // Manufacturer's Model serial code
                                 // Manufacturer's product code
                                 1,
                                 // Manufacturer's Model ID
                                 "KBox",
                                 // Manufacturer's Software version code
                                 "0.0.1 (2016-04-01)",
                                 // Manufacturer's Model version
                                 "v1 revB (2016-02-01)"
                                 );
  //// Set device information
  NMEA2000.SetDeviceInformation(2, // Unique number. Use e.g. Serial number.
    // Class 25, Function 130 => Register on network as a PC Gateway.
    // See: http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
    130, 25,
    // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
    42
  );

  handlerContext = this;
  NMEA2000.SetMsgHandler(handler);
  NMEA2000.EnableForward(false);
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, 22);

  _hub.subscribe(this);

  if (NMEA2000.Open()) {
    DEBUG("Initialized NMEA2000");
  }
  else {
    DEBUG("Something went wrong initializing NMEA2000 ... ");
  }
}

void NMEA2000Service::sendN2kMessage(const tN2kMsg& msg) {
  bool result = NMEA2000.SendMsg(msg);

  DEBUG("Sending message on n2k bus - pgn=%i prio=%i src=%i dst=%i len=%i result=%s", msg.PGN, msg.Priority,
      msg.Source,
      msg.Destination, msg.DataLen, result ? "success":"fail");

  char *pcdin = nmea_pcdin_sentence_for_n2kmsg(msg, 0);
  DEBUG("TX: %s", pcdin);
  free(pcdin);

  if (result) {
    KBoxMetrics.event(KBoxEventNMEA2000MessageSent);
  }
  else {
    KBoxMetrics.event(KBoxEventNMEA2000MessageSendError);
  }
}

void NMEA2000Service::loop() {
  NMEA2000.ParseMessages();

  if (_skVisitor.getMessages().size() > 0) {
    // FIXME: We should move the messages into a circular buffer.
    // This code will block too long if there are more than 4 messages.
    for (LinkedListConstIterator<tN2kMsg*> it = _skVisitor.getMessages().begin();
        it != _skVisitor.getMessages().end(); it++) {
      sendN2kMessage(**it);
    }
    _skVisitor.flushMessages();
  }
}

void NMEA2000Service::updateReceived(const SKUpdate& update) {
  if (update.getSource().getInput() != SKSourceInputNMEA2000) {
    _skVisitor.processUpdate(update);
  }
}

void NMEA2000Service::visit(const NMEA2000Message &m) {
  sendN2kMessage(m.getN2kMsg());
}

void NMEA2000Service::visit(const IMUMessage &m) {
  if (m.getCalibration() == IMUMessage::IMU_CALIBRATED) {
    tN2kMsg n2kmessage;
    SetN2kAttitude(n2kmessage, _imuSequence, m.getYaw(), m.getPitch(), m.getRoll());
    sendN2kMessage(n2kmessage);

    double magneticVariation = N2kDoubleNA;
    double magneticDeviation = N2kDoubleNA;
    SetN2kMagneticHeading(n2kmessage, _imuSequence, m.getCourse(), magneticDeviation, magneticVariation);

    sendN2kMessage(n2kmessage);

    _imuSequence++;
  }
}

void NMEA2000Service::processMessage(const KMessage &m) {
  m.accept(*this);
}
