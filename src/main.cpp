/**
 * Author Teemu Mäntykallio
 *
 * Plot TMC2130 or TMC2660 motor load using the stallGuard value.
 * You can finetune the reading by changing the STALL_VALUE.
 * This will let you control at which load the value will read 0
 * and the stall flag will be triggered. This will also set pin DIAG1 high.
 * A higher STALL_VALUE will make the reading less sensitive and
 * a lower STALL_VALUE will make it more sensitive.
 *
 * You can control the rotation speed with
 * 0 Stop
 * 1 Resume
 * + Speed up
 * - Slow down
 */

#include "Arduino.h"
#include <TMCStepper.h>

#define MAX_SPEED 40 // In timer value
#define MIN_SPEED 1000

#define STALL_VALUE 20 // [0..255]
/* #define STALL_VALUE 100 // [0..255] */
/* #define STALL_VALUE 255 // [0..255] */

#define EN_PIN 32           // Enable
#define DIR_PIN 0          // Direction
#define STEP_PIN 4         // Step
/* #define SW_RX 63            // TMC2208/TMC2224 SoftwareSerial receive pin */
/* #define SW_TX 40            // TMC2208/TMC2224 SoftwareSerial transmit pin */
#define SERIAL_PORT Serial2 // TMC2208/TMC2224 HardwareSerial port
#define DRIVER_ADDRESS 0b00 // TMC2209 Driver address according to MS1 and MS2

#define R_SENSE                                                                \
  0.11f // Match to your driver
        // SilentStepStick series use 0.11
        // UltiMachine Einsy and Archim2 boards use 0.2
        // Panucatt BSD2660 uses 0.1
        // Watterott TMC5160 uses 0.075

// Select your stepper driver type
TMC2209Stepper driver(&SERIAL_PORT, R_SENSE, DRIVER_ADDRESS);
// TMC2209Stepper driver(SW_RX, SW_TX, R_SENSE, DRIVER_ADDRESS);

using namespace TMC2209_n;

// Using direct register manipulation can reach faster stepping times
#define STEP_PORT PORTF // Match with STEP_PIN
#define STEP_BIT_POS 0  // Match with STEP_PIN

/* ISR(){ */
/*   //STEP_PORT ^= 1 << STEP_BIT_POS; */
/*   digitalWrite(STEP_PIN, !digitalRead(STEP_PIN)); */
/* } */

int timeBetweenHalfSteps;

void makeHalfStep() { digitalWrite(STEP_PIN, !digitalRead(STEP_PIN)); }

void setup() {
  timeBetweenHalfSteps = 100;
  Serial.begin(250000); // Init serial port and set baudrate
  while (!Serial)
    ; // Wait for serial port to connect
  Serial.println("\nStart...");

  SERIAL_PORT.begin(115200);
  /* driver.beginSerial(115200); */

  pinMode(EN_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);

  driver.begin();
  driver.toff(4);
  driver.blank_time(24);
  // this might be too high
  driver.rms_current(400); // mA
  driver.microsteps(16);
  driver.TCOOLTHRS(0xFFFFF); // 20bit max
  driver.semin(0);
  driver.semax(3);
  driver.sedn(0b01);
  driver.SGTHRS(STALL_VALUE);

  /* // Set stepper interrupt */
  /* { */
  /*   cli();//stop interrupts */
  /*   TCCR1A = 0;// set entire TCCR1A register to 0 */
  /*   TCCR1B = 0;// same for TCCR1B */
  /*   TCNT1  = 0;//initialize counter value to 0 */
  /*   OCR1A = 256;// = (16*10^6) / (1*1024) - 1 (must be <65536) */
  /*   // turn on CTC mode */
  /*   TCCR1B |= (1 << WGM12); */
  /*   // Set CS11 bits for 8 prescaler */
  /*   TCCR1B |= (1 << CS11);// | (1 << CS10); */
  /*   // enable timer compare interrupt */
  /*   TIMSK1 |= (1 << OCIE1A); */
  /*   sei();//allow interrupts */
  /* } */
}

void loop() {
  static uint32_t last_time = 0;
  uint32_t ms = millis();

  makeHalfStep();
  delayMicroseconds(timeBetweenHalfSteps);

  while (Serial.available() > 0) {
    int8_t read_byte = Serial.read();
    if (read_byte == '0') {
      digitalWrite(EN_PIN, HIGH);
    } else if (read_byte == '1') {
      digitalWrite(EN_PIN, LOW);
    } else if (read_byte == '+') {
      if (timeBetweenHalfSteps > MAX_SPEED)
      timeBetweenHalfSteps -= 20;
    } else if (read_byte == '-') {
      if (timeBetweenHalfSteps < MIN_SPEED)
        timeBetweenHalfSteps += 20;
    }
  }

  if ((ms - last_time) > 5000) {
    last_time = ms;
    uint16_t sgResult = driver.SG_RESULT();
    uint16_t csActual = driver.cs2rms(driver.cs_actual());

    Serial.print("0 ");
    Serial.print(sgResult, DEC);
    Serial.print(" ");
    Serial.println(csActual, DEC);
    Serial.println();
  }
}
