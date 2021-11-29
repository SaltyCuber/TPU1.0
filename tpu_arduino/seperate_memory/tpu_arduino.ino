byte regs[8] = {0,0,0,0,0,0,255,0};
// pin 2 will be cpu clock pin
const byte CLK = 2;
const byte RWB = 38;
unsigned int temp;
unsigned int temp1;
byte oldCarry;
byte pmemBank = 0;
byte vmemBank = 1;
byte imm;
byte instr;
byte reg;
byte instrReg;


void setup() {
  pinMode(RWB, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(CLK), onClock, RISING);
  DDRA = 255;
  DDRC = 255;
  DDRL = 0;
  Serial.begin(9600);
}

byte readMem(byte lower, byte upper) {
  digitalWrite(RWB, HIGH);
  DDRL = 0;
  PORTC = lower;
  PORTA = upper;
  byte data = PINL;
  return data;
}

void writeMem(byte lower, byte upper, byte data) {
  digitalWrite(RWB, LOW);
  DDRL = 0xFF;
  PORTC = lower;
  PORTA = upper;
  PORTL = data;
  return;
}

byte getImm() {
  if (regs[6] == 0xFF) {
    pmemBank++;
  }
  regs[6]++;
  return readMem(regs[6],pmemBank);
}


byte calcFlags(int x) {
  bool flags[] = {false,false,false,false,false,false,false,false};
  if (x > 0xFF) {
    flags[0] = true;
  }
  if (x == 0) {
    flags[1] = true;
  }
  if ((x & 0b10000000) > 0) {
    flags[2] = true;
  }
  if ((x & 1) > 0) {
    flags[3] = true;
  }
  byte out = 0;
  for (int i = 0; i <=7; i++) {
    if (flags[i] == true) {
      out++;
    }
    out = out << 1;
  }
  return out;
}

void onClock() {
  if (regs[6] == 0xFF) {
    pmemBank++;
  }
  instrReg = readMem(++regs[6], pmemBank);
  instr = (instrReg & 0b11111000) >> 3;
  reg = instrReg & 0b00000111;
  switch (instr) {
    case 0:
      // NOP
      break;
    
    case 1:
      // ADC : acc = acc + imm + carry
      imm = getImm();
      temp = regs[1] + imm;
      if (regs[7] & 0b10000000 > 0) {
        temp++;
      }
      regs[7] = calcFlags(temp);
      regs[1] = byte(temp);
      break;
    
    case 2:
      // ADC : acc = acc + reg + carry
      temp = regs[1] + regs[reg];
      if (regs[7] & 0b10000000 > 0) {
        temp++;
      }
      regs[7] = calcFlags(temp);
      regs[1] = byte(temp);
      break;
    
    case 3:
      // SBB : acc = acc - imm - carry
      temp = regs[1] + (~getImm());
      if (regs[7] & 0b10000000 == 0) {
        temp++;
      }
      regs[7] = calcFlags(temp);
      regs[1] = byte(temp);
      break;

    
    case 4:
      // SBB : acc = acc - reg - carry
      temp = regs[1] + (~regs[reg]);
      if (regs[7] & 0b10000000 == 0) {
        temp++;
      }
      regs[7] = calcFlags(temp);
      regs[1] = byte(temp);
      break;

    case 5:
      // ROR : acc = carry > imm > carry
      imm = getImm();
      oldCarry = regs[7] & 0b10000000;
      if ((imm >> 1 << 1) < imm) {
        temp1 = 0b10000000;
      }
      temp = imm >> 1 + oldCarry;
      regs[7] = calcFlags(temp) | temp1;
      regs[1] = byte(temp);
      break;
    
    case 6:
      // ROR : reg = carry > reg > carry
      oldCarry = regs[7] & 0b10000000;
      if ((regs[reg] >> 1 << 1) < regs[reg]) {
        temp1 = 0b10000000;
      }
      temp = regs[reg] >> 1 + oldCarry;
      regs[7] = calcFlags(temp) | temp1;
      regs[reg] = byte(temp);
      break;
    
    case 7:
      // XOR : acc = acc ^ imm
      imm = getImm();
      regs[1] = byte(regs[1] ^ imm);
      regs[7] = calcFlags(regs[1]) | (regs[7] & 0b10000000); // don't affect carry flag
      break;

    case 8:
      // XOR : acc = acc ^ reg
      regs[1] = byte(regs[1] ^ regs[reg]);
      regs[7] = calcFlags(regs[1]) | (regs[7] & 0b10000000);
      break;

    case 9:
      // OR : acc = acc | imm
      imm = getImm();
      regs[1] = byte(regs[1] | imm);
      regs[7] = calcFlags(regs[1]) | (regs[7] & 0b10000000);
      break;
    
    case 10:
      // OR : acc = acc | reg
      regs[1] = byte(regs[1] | regs[reg]);
      regs[7] = calcFlags(regs[1]) | (regs[7] & 0b10000000);
      break;

    case 11:
      // AND : acc = acc & imm
      imm = getImm();
      regs[1] = byte(regs[1] & imm);
      regs[7] = calcFlags(regs[1]) | (regs[7] & 0b10000000);
      break;

    case 12:
      // AND : acc = acc & reg
      regs[1] = byte(regs[1] & regs[reg]);
      regs[7] = calcFlags(regs[1]) | (regs[7] & 0b10000000);
      break;

    case 13:
      // MTA : acc = imm
      regs[1] = getImm();
      regs[7] = calcFlags(regs[1]) | (regs[7] & 0b10000000);
      break;

    case 14:
      // MTA : acc = reg
      regs[1] = regs[reg];
      regs[7] = calcFlags(regs[1]) | (regs[7] & 0b10000000);
      break;

    case 15:
      // MTR : reg = imm
      regs[reg] = getImm();
      regs[7] = calcFlags(regs[reg]) | (regs[7] & 0b10000000);
      break;

    case 16:
      // MTR : reg = acc
      regs[reg] = regs[1];
      regs[7] = calcFlags(regs[reg]) | (regs[7] & 0b10000000);
      break;

    case 17:
      // JRI : jump to 16 bit address (reg = upper word, imm = lower word)
      regs[6] = getImm();
      pmemBank = regs[reg];
      break;

    case 18:
      // JRI : jump to 16 bit address (imm = upper word, reg = lower word)
      pmemBank = getImm();
      regs[6] = regs[reg];
      break;

    case 19:
      // BRS : branch to imm if bit set
      if (regs[7] & (1<<(reg)) == 1) {
        temp = pmemBank;
        regs[6] = getImm();
        pmemBank = temp;
      }
      break;

    case 20:
      // BRS : branch to acc if bit set
      if (regs[7] & (1<<(reg)) == 1) {
        temp = pmemBank;
        regs[6] = regs[1];
        pmemBank = temp;
      }
      break;

    case 21:
      // BNS : branch to imm if bit not set
      if (regs[7] & (1<<(reg)) == 0) {
        temp = pmemBank;
        regs[6] = getImm();
        pmemBank = temp;
      }
      break;

    case 22:
      // BNS : branch to acc if bit not set
      if (regs[7] & (1<<(reg)) == 1) {
        temp = pmemBank;
        regs[6] = regs[1];
        pmemBank = temp;
      }
      break;

    case 23:
      // LDA : acc = vmem[imm]
      imm = getImm();
      regs[1] = readMem(imm, vmemBank);
      break;

    case 24:
      // LDA : acc = vmem[reg]
      regs[1] = readMem(regs[reg], vmemBank);
      break;

    case 25:
      // STA : vmem[imm] = acc
      imm = getImm();
      writeMem(imm, vmemBank, regs[1]);
      break;

    case 26:
      // STA : vmem[reg] = acc
      writeMem(regs[reg], vmemBank, regs[1]);
      break;

    case 27:
      // INP : acc = data from port number imm
      imm = getImm();
      switch (imm) {
        // TPU 1.0 only utilizes 8 of the possible 256 i/o ports
        case 0:
          regs[1] = pmemBank;
          break;

        case 1:
          regs[1] = vmemBank;
          break;

        case 2:

          break;

        case 3:
          break;

        case 4:
          break;

        case 5:
          break;

        case 6:
          break;

        case 7:
          break;
      }
      break;

    case 28:
      // OUT : send acc to port number imm
      imm = getImm();
      switch (imm) {
        case 0:
          pmemBank = regs[1];
          break;

        case 1:
          vmemBank = regs[1];
          break;

        case 2:
          
          break;

        case 3:
          break;

        case 4:
          break;

        case 5:
          Serial.print(regs[1]);
          break;

        case 6:
          Serial.print(int(regs[1]));
          break;

        case 7:
          break;
      }
      break;

    case 29:
      // CLF : clear bit in CR
      regs[7] = regs[7] & ~(1<<reg);
      break;

    case 30:
      // SEF : set bit in CR
      regs[7] = regs[7] | (1<<reg);
      break;

    case 31:
      // HLT : halt the system clock
      detachInterrupt(digitalPinToInterrupt(CLK));
      break;
}}

void loop() {
  // put your main code here, to run repeatedly:
}
