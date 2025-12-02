/* RFID-RC522 모듈 사용 라이브러리 */
#include <MFRC522Constants.h>
#include <MFRC522Debug.h>
#include <MFRC522Driver.h>
#include <MFRC522DriverI2C.h>
#include <MFRC522DriverPin.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522Hack.h>
#include <MFRC522v2.h>
#include <require_cpp11.h>

/* RFID 모듈 관련 설정 */
// Learn more about using SPI/I2C or check the pin assigment for your board: https://github.com/OSSLibraries/Arduino_MFRC522v2#pin-layout
MFRC522DriverPinSimple ss_pin(15);

MFRC522DriverSPI driver{ss_pin}; // Create SPI driver
//MFRC522DriverI2C driver{};     // Create I2C driver
MFRC522 mfrc522{driver};         // Create MFRC522 instance

/* 터치 키패드 관련 설정 */
#define scl_pin 5
#define sdo_pin 4

/* 전역 변수 설정 */
byte current_key;
byte last_key;

/* 인증 키(키패드 비밀번호, RFID UID) 설정 */
String rfid_pass_key = "767bc6db";
String keypad_pass_key = "14116";

void setup() {
  Serial.begin(115200); while (!Serial); // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4).
  
  mfrc522.PCD_Init();    // Init MFRC522 board.
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);	// Show details of PCD - MFRC522 Card Reader details.
  Serial.println("Scan PICC to see UID, SAK, type, and data blocks...");

  pinMode(scl_pin, OUTPUT); pinMode(sdo_pin, INPUT);

  // 파란색 LED 켜기
}

void loop() {
  if (readKeypad() == 13) {
    Serial.println("키패드 입력이 인식되었습니다.");
    // LED 파란색 끄고 주황색 켜기
    String password_input;
    for (digit = 1; digit <= 4; digit ++) {
      while (readKeypad() != 0) {} // 키패드에서 손을 뗄 때까지 대기
      while (readKeypad() == 0) {} // 다시 누를 때까지 대기
      current_key = readKeypad();
      Serial.print(digit); Serial.print("번째 자리: "); Serial.println(current_key);
      password_input += String(current_key);
    }
    Serial.print("입력된 비밀번호: "); Serial.println(password_input);

    /* 패스워드가 등록된 패스워드인지 확인하고 문 열기 또는 무시하기 */
    if (password_input == keypad_pass_key) {
      Serial.println("인식된 패스워드는 등록되어 있는 패스워드입니다.");
      //doorOpen();
    } else {
      Serial.println("인식된 패스워드는 등록되어 있지 않은 패스워드입니다.");
    }
  }

  if (mfrc522.PICC_IsNewCardPresent()) {
    Serial.println("RFID 카드가 인식되었습니다.");
    // 파란색 LED 끄고 주황색 LED 켜기

    if (!mfrc522.PICC_ReadCardSerial()) return; // 카드 1개를 선택하고, 카드 1개가 선택되지 않았으면 loop 함수의 처음으로 돌아감
    //MFRC522Debug::PICC_DumpToSerial(mfrc522, Serial, &(mfrc522.uid)); // 덤프 디버그 정보 출력 (PICC_HaltA() 함수 자동 호출)
    //Serial.print("Card UID: "); MFRC522Debug::PrintUID(Serial, (mfrc522.uid)); Serial.println(); // 카드 UID 출력

    /* 카드의 UID를 uidString 변수에 String 형식으로 저장함 */
    String uidString = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (mfrc522.uid.uidByte[i] < 0x10) uidString += "0";
      uidString += String(mfrc522.uid.uidByte[i], HEX);
    }

    /* UID가 등록된 UID인지 확인하고 문 열기 또는 무시하기 */
    if (uidString == rfid_pass_key) {
      Serial.println("인식된 RFID 카드는 등록되어 있는 카드입니다.");
      //doorOpen();
    } else {
      Serial.println("인식된 RFID 카드는 등록되어 있지 않은 카드입니다.");
    }
  }
}

/* 키패드 입력을 인식하고, 입력된 키를 byte 형태로 반환 */
byte readKeypad(void) {
  byte key_count;
  byte key_state = 0;
  for (key_count = 1; key_count <= 16; key_count ++) {
    digitalWrite(scl_pin, LOW);
    if (!digitalRead(sdo_pin)) key_state = key_count;
    digitalWrite(scl_pin, HIGH);
  }
  return key_state;
}

/* 키패드 비밀번호 또는 RFID UID 인증에 성공하면 실행 */
void doorOpen() {
  // 초록색 LED 켜기
  // 서보모터 열기

  delay(5000); // 문 닫힘 상태를 인식하기 전에 5초 대기 (인증 통과 후 5초동안 문이 닫혀 있으면 자동으로 잠김)

  // while (문이 열린 상태) {}

  // 서보모터 닫기
  delay(100);
  // 초록색 LED 끄고 파란색 LED 켜기
}
