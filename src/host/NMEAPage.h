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

#include <MFD.h>

class NMEAPage : public Page {
  private:
    bool needsPainting;
    int status = -1;

  public:
    NMEAPage();

    void willAppear();
    bool processEvent(const TickEvent &e);
    bool processEvent(const ButtonEvent &e);
    void paint(GC &gc);
};

