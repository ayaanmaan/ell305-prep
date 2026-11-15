const int a = 2, b = 3, c = 4, d = 5, e = 6, f = 7, g = 8;

void d0() { 
  digitalWrite(a,HIGH);
  digitalWrite(b,HIGH);
  digitalWrite(c,HIGH);
  digitalWrite(d,HIGH);  
  digitalWrite(e,HIGH);
  digitalWrite(f,HIGH);  
} 

void d1() {
  digitalWrite(b,HIGH);
  digitalWrite(c,HIGH);
} 

void d2() {
  digitalWrite(a,HIGH);
  digitalWrite(b,HIGH);
  digitalWrite(g,HIGH);
  digitalWrite(e,HIGH);
  digitalWrite(d,HIGH);
}

void d3() { 
  digitalWrite(a,HIGH);
  digitalWrite(b,HIGH);  
  digitalWrite(c,HIGH);
  digitalWrite(d,HIGH);
  digitalWrite(g,HIGH);
} 

void d4() {
  digitalWrite(f,HIGH);
  digitalWrite(b,HIGH);
  digitalWrite(g,HIGH);
  digitalWrite(c,HIGH);
}

void d5() { 
  digitalWrite(a,HIGH);
  digitalWrite(f,HIGH);
  digitalWrite(g,HIGH);
  digitalWrite(c,HIGH);
  digitalWrite(d,HIGH);
} 

void d6() {
  d5();
  digitalWrite(e,HIGH); 
  /*
  digitalWrite(a,HIGH);
  digitalWrite(f,HIGH);
  digitalWrite(g,HIGH);
  digitalWrite(c,HIGH);
  digitalWrite(d,HIGH);
  */ 
}

void d7() {
  d1();   
  digitalWrite(a,HIGH);
  /*
  digitalWrite(b,HIGH);
  digitalWrite(c,HIGH);
  */
}

void d8() {
  d0();
  digitalWrite(g,HIGH);
  /*
  digitalWrite(a,HIGH);
  digitalWrite(b,HIGH);
  digitalWrite(c,HIGH);
  digitalWrite(d,HIGH);  
  digitalWrite(e,HIGH);  
  digitalWrite(f,HIGH);
  */
} 

void d9() {
  d5(); 
  digitalWrite(b,HIGH);
  /*
  digitalWrite(a,HIGH);
  digitalWrite(g,HIGH);
  digitalWrite(c,HIGH);
  digitalWrite(d,HIGH);  
  digitalWrite(f,HIGH);
  */
} 

void clr() { 
  digitalWrite(a,LOW);
  digitalWrite(b,LOW);
  digitalWrite(g,LOW);
  digitalWrite(c,LOW);
  digitalWrite(d,LOW);  
  digitalWrite(e,LOW);  
  digitalWrite(f,LOW);  
} 

const int pSwitch = 9;
int rVal;
int bState;
int op = -1;

void setup() {
  pinMode(pSwitch, INPUT);
  bState = digitalRead(pSwitch); 
  int i;
  for(i=2; i<=8; ++i) pinMode(i, OUTPUT);
  clr();
} 

void loop() {
  rVal = digitalRead(pSwitch);
  if (rVal != bState) {
    if (rVal == HIGH) {
	    clr();
      ++op;
    }
    else {
      if (op > 9) op = -1;
      if (op == -1) clr();
      else if (op == 0) d0();
      else if (op == 1) d1();
      else if (op == 2) d2();
      else if (op == 3) d3();
      else if (op == 4) d4();
      else if (op == 5) d5();
      else if (op == 6) d6();
      else if (op == 7) d7();
      else if (op == 8) d8();
      else d9();
    }
  }
  bState = rVal;
}