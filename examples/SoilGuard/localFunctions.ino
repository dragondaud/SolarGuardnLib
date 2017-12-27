bool readDHT(float *h, float *t) { // read temp and humidity from DHT22
  *h = dht.readHumidity();
  *t = dht.readTemperature(true);
  if (isnan(*h) || isnan(*t)) {
    return false;
  } else {
    return true;
  }
} // readDHT

int readMoisture(int pin, int pow, int num, int tim) { // read soil moisture using DFRobot SEN0193
  int s = 0;
  for (int i = 0; i < num; i++) {
    int r = 0, x = 0;
    do {
      digitalWrite(pow, HIGH);       // power to moisture sensor
      delay(tim);
      r = 1023 - analogRead(pin);   // read analog value from moisture sensor (invert for capacitance sensor)
      digitalWrite(pow, LOW);        // turn off moisture sensor
      delay(tim * 1.2);
      x++;
    } while (((r < 200) || (r > 800)) && x < num);  // skip invalid values
    s += r;
  }
  return round((float)s / (float)num);
} // readMoisture()


