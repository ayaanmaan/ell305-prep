  const int rows[] = {2, 3, 4};
  const int cols[] = {12, 11, 10};

  void setup() {
    for (int i = 0; i < 3; i++) {
      pinMode(rows[i], OUTPUT);
      pinMode(cols[i], OUTPUT);
      digitalWrite(rows[i], LOW);  // default rows off
      digitalWrite(cols[i], HIGH); // default columns off
    }
  }

  void loop() {
    for (int i = 0; i < 3; i++) {
      digitalWrite(rows[i], HIGH); // row on
      digitalWrite(cols[i], LOW);  // column on
      
      delay(500);                  
      
      digitalWrite(rows[i], LOW);  
      digitalWrite(cols[i], HIGH);
    }
  }