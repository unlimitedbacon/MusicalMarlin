/*
  music.cpp - Plays a song
  Part of Marlin

  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "avr/pgmspace.h"
#include "stepper.h"
#include "song.h"

const float frequency[] = {
  16.35, //C0
  17.32, //Db0
  18.35, //D0
  19.45, //Eb0
  20.60, //E0
  21.83, //F0
  23.83, //Gb0
  24.50, //G0
  25.96, //Ab0
  27.50, //A0
  29.14, //Bb0
  30.87, //B0
  32.70, //C1
  34.65, //Db1
  36.71, //D1
  38.89, //Eb1
  41.20, //E1
  43.65, //F1
  46.25, //Gb1
  49.00, //G1
  51.91, //Ab1
  55.00, //A1
  58.27, //Bb1
  61.74, //B1
  65.41, //C2
  69.30, //Db2
  73.42, //D2
  77.78, //Eb2
  82.41, //E2
  87.31, //F2
  92.50, //Gb2
  98.00, //G2
  103.83, //Ab2
  110.00, //A2
  116.54, //Bb2
  123.47, //B2
  130.81, //C3
  138.59, //Db3
  146.83, //D3
  155.56, //Eb3
  164.81, //E3
  174.61, //F3
  185.00, //Gb3
  196.00, //G3    30
  207.65, //Ab3   31
  220.00, //A3    32
  233.08, //Bb3   33
  246.94, //B3    34
  261.63, //C4    35
  277.18, //Db4   36
  293.66, //D4    37
  311.13, //Eb4   38
  329.63, //E4    39
  349.23, //F4    40
  369.99, //Gb4   41
  392.00, //G4    42
  415.30, //Ab4   43
  440.00, //A4    44
  466.16, //Bb4   45
  493.88, //B4    46
  523.25, //C5    47
  554.37, //Db5   48
  587.33, //D5    49
  622.25, //Eb5   50
  659.26, //E5    51
  698.46, //F5    52
  739.99, //Gb5   53
  783.99, //G5    54
  830.61, //Ab5   55
  880.00, //A5    56
  932.33,
  987.77,
  1046.50,
  1108.73,
  1174.66,
  1244.51,
  1318.51,
  1396.91,
  1479.98,
  1567.98,
  1661.22,
  1760.00,
  1864.66,
  1975.53,
  2093.00,
  2217.46,
  2349.32,
  2489.02,
  2637.02,
  2793.83,
  2959.96,
  3135.96,
  3322.44,
  3520.00,
  3729.31,
  3951.07,
  4186.01,
  4434.92,
  4698.63,
  4978.03,
  5274.04,
  5587.65,
  5919.91,
  6271.93,
  6644.88,
  7040.00,
  7458.62,
  7902.13
};

const int scale = 4;

const uint8_t microsteps[] = MICROSTEP_MODES;

uint8_t timbre[] = {4,microsteps[1],microsteps[2]};

uint8_t last_notes[3] = {128,128,128};

bool dirs[3] = {true,true,true};

void play_notes(uint8_t notes[3])
{
  // Process notes for each channel
  float delta[3];
  float target[3];
  for (byte i=0; i<3; i++) {
    SERIAL_ECHO_START;
    // Continue last note
    if (notes[i] == 129) {
      notes[i] = last_notes[i];
    }
    // Stop note
    if (notes[i] == 128) {
      delta[i] = 0;
      SERIAL_ECHOLN("STOP");
    } else {
      // speed (mm/sec) = frequency (steps/sec) * (microsteps/step) / (microsteps/mm)
      // distance (mm) = speed (mm/sec) * delay (sec)
      delta[i] = frequency[notes[i]] * microsteps[i] / (axis_steps_per_unit[i] * scale) * 0.3125;
      if (!dirs[i]) {
        delta[i] = -1.0 * delta[i];
      }
      SERIAL_ECHO("Freq: ");
      SERIAL_ECHO(frequency[notes[i]]);
      SERIAL_ECHO(" Steps/mm: ");
      SERIAL_ECHO(axis_steps_per_unit[i]);
      SERIAL_ECHO(" Delta: ");
      SERIAL_ECHOLN(delta[i]);
    }
    target[i] = current_position[i] + delta[i];
    // Bounce off endstops
    if (target[i]>(max_pos[i]/(microsteps[i]/timbre[i])) or target[i]<0) {
      target[i] = current_position[i] - delta[i];
      dirs[i] = !dirs[i];
    }
    current_position[i] = target[i];
    last_notes[i] = notes[i];
  }
  // Handle pauses in music
  if (delta[0]==0 and delta[1]==0 and delta[2]==0) {
    st_synchronize();
    delay(312);
    SERIAL_ECHO_START;
    SERIAL_ECHOLN("PAUSE");
  } else {
    // Calculate hypotenuse of move and divide by the note duration to ge the speed
    float feedrate = sqrt(sq(delta[0]) + sq(delta[1]) + sq(delta[2])) / 0.3125;
    // Queue move
    plan_buffer_line( target[0], target[1], target[2], current_position[E_AXIS], feedrate, 0);
    SERIAL_ECHO_START;
    SERIAL_ECHO("Hyp: ");
    SERIAL_ECHO(sqrt(sq(delta[0]) + sq(delta[1]) + sq(delta[2])));
    SERIAL_ECHO(" Feed: ");
    SERIAL_ECHOLN(feedrate);
  }
}

void play_music()
{
  int i = 0;
  uint8_t notes[3];
  //unsigned long notetime;
  SERIAL_ECHO_START;
  SERIAL_ECHOLNPGM("Playing song:");
  // Disable microstepping and speed limits
  for (uint8_t i=0; i<3; i++) {
    microstep_mode(i, timbre[i]);
    max_feedrate[i] = 3.4028235E+38;
    max_acceleration_units_per_sq_second[i] = 4294967295; // Max value for ulong
  }
  reset_acceleration_rates();
  acceleration = 3.4028235E+38; // Max value for float
  retract_acceleration = 3.4028235E+38;
  // Disable jerk
  max_xy_jerk = 3.4028235E+38;
  max_z_jerk = 3.4028235E+38;
  while(notes[0] != 255) {
    //notetime = millis();
    //st_synchronize();
    //SERIAL_ECHO_START;
    //SERIAL_ECHO("Time: ");
    //SERIAL_ECHOLN(millis()-notetime);
    SERIAL_ECHO_START;
    SERIAL_ECHO("Note");
    SERIAL_ECHOLN(i/3);
    notes[0] = pgm_read_byte_near(song + i);
    notes[1] = pgm_read_byte_near(song + i+1);
    notes[2] = pgm_read_byte_near(song + i+2);
    play_notes(notes);
    i = i+3;
  }
}
