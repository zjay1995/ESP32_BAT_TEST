#include "LC709203F.h"

LC709203F gg;  // create a gas gauge object.

void setup() {
  Serial.begin(115200);
  if (!gg.begin()) {
    while (1) delay(10);
  }

  Serial.print("Version: 0x"); Serial.println(gg.getICversion(), HEX);

  gg.setCellCapacity(LC709203F_APA_1000MAH);
  gg.setAlarmVoltage(3.4);
  gg.setCellProfile( LC709203_NOM3p7_Charge4p2 ) ;

}

void loop() {
  Serial.print("V_mV: "); Serial.println( String(gg.cellVoltage_mV() / 1000.0).c_str());
  Serial.print("Cell_Remaining: ");Serial.println(String(gg.cellRemainingPercent10() / 10).c_str());

}
