/*

    This file is part of KBox.

    Copyright (C) 2016 Thomas Sarlandie.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "ShuntMonitorPage.h"
#include "i2c_t3.h"
#include "board.h"

ShuntMonitorPage::ShuntMonitorPage() {
}

bool ShuntMonitorPage::readRegister(uint8_t registerAddress, uint16_t *registerValue) {
  Wire1.beginTransmission(ina219_address);
  Wire1.send(registerAddress);
  int ret = Wire1.endTransmission();
  if (ret != 0) {
    DEBUG("INA219 write req for register %x: %i", registerAddress, ret);
    status = ret;
    return false;
  }

  ret = Wire1.requestFrom(ina219_address, (size_t)2);
  if (ret == 2) {
    *registerValue = (Wire1.read() << 8) & Wire1.read();
    status = 0;
    return true;
  }
  else {
    DEBUG("INA219 read failed: %i bytes read.", ret);
    status = ret;
    return false;
  }
}

void ShuntMonitorPage::willAppear() {
  fetchValues();
  needsPainting = true;
}

void ShuntMonitorPage::fetchValues() {
  uint16_t busVoltageRegister, shuntVoltageRegister;

  DEBUG("Reading data from ina219");

  if (readRegister(shuntVoltageRegisterAddress, &shuntVoltageRegister) &&
      readRegister(busVoltageRegisterAddress, &busVoltageRegister) ) {
    shuntVoltage = (int16_t)shuntVoltageRegister;
    busVoltage = ((int16_t)busVoltageRegisterAddress) >> 3;
    DEBUG("Got new shuntVoltageRegister=%x busVoltageRegister=%x", shuntVoltageRegister, busVoltageRegister);
  }
  else {
    DEBUG("failed :(");
  }
}

bool ShuntMonitorPage::processEvent(const ButtonEvent &e) {
  if (e.clickType == ButtonEventTypePressed) {
    fetchValues();
    needsPainting = true;
  }
  return true;
}

void ShuntMonitorPage::paint(GC &gc) {
  if (!needsPainting)
    return;

  gc.fillRectangle(Point(0, 0), Size(320, 240), ColorBlue);
  gc.drawText(Point(2, 5), FontDefault, ColorWhite, "  INA219 Test Page");

  char text[30];
  if (status == 0) {
    snprintf(text, sizeof(text), "Bus Voltage: %i", busVoltage);
    gc.drawText(Point(0, 30), FontDefault, ColorWhite, text);
    snprintf(text, sizeof(text), "Shunt Voltage: %i", shuntVoltage);
    gc.drawText(Point(0, 60), FontDefault, ColorWhite, text);
  }
  else if (status == -1) {
    gc.drawText(Point(0, 30), FontDefault, ColorRed, "Press button to read bus and shunt voltages.");
  } else {
    snprintf(text, sizeof(text), "Error: %i", status);
    gc.drawText(Point(0, 30), FontDefault, ColorRed, text);
  }

  needsPainting = false;
}
