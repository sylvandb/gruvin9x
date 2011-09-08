/*
 * Authors (alphabetical order)
 * - Bertrand Songis <bsongis@gmail.com>
 * - Bryan J. Rentoul (Gruvin) <gruvin@gmail.com>
 *
 * gruvin9x is based on code named er9x by
 * Author - Erez Raviv <erezraviv@gmail.com>, which is in turn
 * was based on the original (and ongoing) project by Thomas Husterer,
 * th9x -- http://code.google.com/p/th9x/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <gtest/gtest.h>
#include "gruvin9x.h"

extern uint8_t displayBuf[DISPLAY_W*DISPLAY_H/8];

TEST(outdezNAtt, test_unsigned) {
  uint16_t altitude = 65530;

  uint8_t refBuf[sizeof(displayBuf)];
  memset(displayBuf, 0, sizeof(displayBuf));
  lcd_putc(0*FWNUM, 0, '6');
  lcd_putc(1*FWNUM, 0, '5');
  lcd_putc(2*FWNUM, 0, '5');
  lcd_putc(3*FWNUM, 0, '3');
  lcd_putc(4*FWNUM, 0, '0');
  memcpy(refBuf, displayBuf, sizeof(displayBuf));

  memset(displayBuf, 0, sizeof(displayBuf));
  lcd_outdezNAtt(1, 0, altitude, LEFT|UNSIGN);

  EXPECT_EQ(memcmp(refBuf, displayBuf, sizeof(displayBuf)), 0) << "Unsigned numbers will be bad displayed";
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
