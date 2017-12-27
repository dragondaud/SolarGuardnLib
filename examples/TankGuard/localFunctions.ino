int getDist() { // get distance to water from HC-SR04 sensor
  int r = 0, c = 0;
  for (int i = 0; i < 10; i++) {
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);
    unsigned long p = pulseIn(ECHO, HIGH, 12000) * 0.34 / 2;
    if (!p == 0 || p > 400) {
      r += p;
      c++;
    }
    delay(20);
  }
  if (!c == 0) return round(r / c);
  else return -1;
} // getDist

