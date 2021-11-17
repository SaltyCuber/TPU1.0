byte instrReg;
byte regs[8] = [0,0,0,0,0,0,0xFF,0];
// pin 2 will be cpu clock pin
const byte CLK = 2;
byte imm;
byte instr;
byte reg;
unsigned int temp;
unsigned int temp1;
byte oldCarry;
byte pmemBank = 0;
byte vmemBank = 1;

void setup() {
  attachInterrupt(digitalPinToInterrupt(CLK), onClock, RISING);
  DDRA = 255;
  DDRC = 255;
  DDRL = 0;
  }

byte readMem(byte lower, byte upper) {
  DDRL = 0;
  PORTC = lower;
  PORTA = upper;
  byte data = PINL;
  return data;
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
  instrReg = readMem(++regs[6], pmemBank);
  instr = (instrReg & 0b11111000) >> 3;
  reg = instrReg & 0b00000111;

  switch (instr) {
    
    case 0:
      // NOP
      break;
    
    case 1:
      // ADC : acc = acc + imm + carry
      imm = readMem(++regs[6], pmemBank);
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
      temp = regs[1] + (~readMem(++regs[6], pmemBank));
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
      imm = readMem(++regs[6], pmemBank);
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
      imm = readMem(++regs[6], pmemBank);
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
      imm = readMem(++regs[6], pmemBank);
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
      imm = readMem(++regs[6],pmemBank);
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
      regs[1] = readMem(++regs[6], pmemBank);
      regs[7] = calcFlags(regs[1]) | (regs[7] & 0b10000000);
      break;

    case 14:
      // MTA : acc = reg
      regs[1] = regs[reg];
      regs[7] = calcFlags(regs[1]) | (regs[7] & 0b10000000);
      break;

    case 15:
      // MTR : reg = imm
      regs[reg] = readMem(++regs[6],pmemBank);
      regs[7] = calcFlags(regs[reg]) | (regs[7] & 0b10000000);
      break;

    case 16:
      // MTR : reg = acc
      regs[reg] = regs[1];
      regs[7] = calcFlags(regs[reg]) | (regs[7] & 0b10000000);
      break;

    case 17:
      // JRI : jump to 16 bit address (reg = upper word, imm = lower word)
      break;

    case 18:
      // JRI : jump to 16 bit address (imm = upper word, reg = lower word)
      break;

    case 19:
      // BRS : branch to imm if bit set
      break;

    case 20:
      // BRS : branch to acc if bit set
      break;

    case 21:
      // BNS : branch to imm if bit not set
      break;

    case 22:
      // BNS : branch to acc if bit not set
      break;

    case 23:
      // LDA : acc = vmem[imm]
      break;

    case 24:
      // LDA : acc = vmem[reg]
      break;

    case 25:
      // STA : vmem[imm] = acc
      break;

    case 26:
      // STA : vmem[reg] = acc
      break;

    case 27:
      // INP : acc = data from port number imm
      break;

    case 28:
      // OUT : send acc to port number imm
      break;

    case 29:
      // CLF : clear bit in CR
      break;

    case 30:
      // SEF : set bit in CR
      break;

    case 31:
      // HLT : halt the system clock (tie clock pin high)
      break;
}}

void loop() {
  // put your main code here, to run repeatedly:

}
