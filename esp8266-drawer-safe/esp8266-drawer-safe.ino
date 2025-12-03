#include <SPI.h> // SPI 통신 라이브러리(RFID 모듈)
#include <MFRC522.h> // RFID-RC522 모듈 사용 라이브러리
#include <Servo.h> // 서보모터 사용 라이브러리

/* RFID 모듈 관련 설정 */
#define ss_pin 15 // 보드의 D8
#define rst_pin 0 // 보드의 D3
MFRC522 mfrc522(ss_pin, rst_pin); // RFID 모듈 객체 선언

/* 터치 키패드 관련 설정 */
#define scl_pin 5 // 보드의 D1
#define sdo_pin 4 // 보드의 D2

/* 전역 변수 설정 */
byte current_key;
byte last_key;

/* 인증 키(키패드 비밀번호, RFID UID) 설정 */
String rfid_pass_key = "767bc6db";
String keypad_pass_key = "14116";

/* LED 핀 번호 설정 */
#define blue_led 16 // D0
#define orange_led 2 // D4
#define green_led 14 // D5
#define red_led 12 // D6

/* 서보모터 사용 설정 */
Servo servo; // 서보모터 객체 선언
int servo_angle_lock = 90; // 잠글 때의 서보모터 각도 설정
int servo_angle_open = 180; // 열 때의 서보모터 각도 설정
#define servo_pin 0 // 서보모터 핀 설정

#define door_pin 13 // 자석 스위치(도어 센서) 핀 번호 지정 (보드의 D7)

void setup() {
  Serial.begin(115200); while (!Serial); // 시리얼 통신 시작, 통신 시작될 때까지 대기
  
  pinMode(scl_pin, OUTPUT); pinMode(sdo_pin, INPUT);
  digitalWrite(scl_pin, HIGH);
  Serial.println("키패드 설정 완료");

  SPI.begin(); // SPI 통신 시작
  mfrc522.PCD_Init(); // RFID 모듈 초기화
  Serial.println("RFID 모듈 초기화 완료");

  pinMode(blue_led, OUTPUT); pinMode(orange_led, OUTPUT); pinMode(green_led, OUTPUT); pinMode(red_led, OUTPUT);
  pinMode(door_pin, INPUT_PULLUP); // 자석 스위치 설정

  servo.attach(servo_pin); servo.write(servo_angle_lock); // 서보모터 작동 시작
}

void loop() {
  digitalWrite(blue_led, HIGH); // 파란색 LED 켜기

  /* 키패드 입력 인식, 입력된 키가 13(*)이라면 */
  if (readKeypad() == 13) {
    Serial.println("키패드 입력(*)이 인식되었습니다.");
    digitalWrite(blue_led, LOW); digitalWrite(orange_led, HIGH); // LED 파란색 끄고 주황색 켜기

    String password_input = ""; // 입력받을 비밀번호 String 초기화

    /* 자리수를 1부터 4까지 반복 */
    for (int digit = 1; digit <= 4; digit ++) {
      while (readKeypad() != 0) { yield(); } // 키패드에서 손을 뗄 때까지 대기
      while (readKeypad() == 0) { yield(); } // 다시 누를 때까지 대기
      current_key = readKeypad();
      Serial.print(digit); Serial.print("번째 자리 입력받은 키: "); Serial.println(current_key);
      password_input += String(current_key); // 입력된 비밀번호 String에 방금 입력받은 키 byte를 추가
    }

    Serial.print("최종 입력된 비밀번호: "); Serial.println(password_input);
    digitalWrite(orange_led, LOW); //주황색 LED 끄기

    /* 비밀번호가 등록된 비밀번호인지 확인하고 문 열기 또는 무시하기 */
    if (password_input == keypad_pass_key) {
      Serial.println("인식된 비밀번호는 등록되어 있는 비밀번호입니다.");
      doorOpen(); // 잠금 풀고, 사용자가 문을 닫으면 다시 잠그는 함수
    } else {
      Serial.println("인식된 비밀번호는 등록되어 있지 않은 비밀번호입니다.");
      digitalWrite(red_led, HIGH); delay(5000); digitalWrite(red_led, LOW); // 빨간색 LED 5초동안 켰다가 끄기
    }

    mfrc522.PICC_HaltA(); mfrc522.PCD_StopCrypto1(); // 다음 통신을 위해 통신 종료
    /* doorOpen 함수가 종료되면 if문 밖으로 빠져나감 */
  }

  /* RFID 카드가 인식되고 데이터를 읽는 데에 성공하면 */
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.println("RFID 카드가 인식되었습니다.");
    digitalWrite(blue_led, LOW); digitalWrite(orange_led, HIGH); // LED 파란색 끄고 주황색 켜기

    /* 카드의 UID를 uidString 변수에 String 형식으로 저장함 */
    String uidString = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (mfrc522.uid.uidByte[i] < 0x10) uidString += "0";
      uidString += String(mfrc522.uid.uidByte[i], HEX);
    }

    Serial.print("인식된 RFID 카드의 UID: "); Serial.println(uidString);
    digitalWrite(orange_led, LOW); //주황색 LED 끄기

    /* UID가 등록된 UID인지 확인하고 문 열기 또는 무시하기 */
    if (uidString == rfid_pass_key) {
      Serial.println("인식된 RFID 카드는 등록되어 있는 카드입니다.");
      doorOpen(); // 잠금 풀고, 사용자가 문을 닫으면 다시 잠그는 함수
    } else {
      Serial.println("인식된 RFID 카드는 등록되어 있지 않은 카드입니다.");
      digitalWrite(red_led, HIGH); delay(5000); digitalWrite(red_led, LOW); // 빨간색 LED 5초동안 켰다가 끄기
    }

    /* doorOpen 함수가 종료되면 if문 밖으로 빠져나감 */
  }

  /* loop문 끝 */
}

/* 키패드에 입력된 키를 byte 형태로 반환 (키패드의 어떠한 키도 누르지 않았으면 0(byte)을 반환) */
byte readKeypad() {
  byte key_state = 0;
  for (byte key_count = 1; key_count <= 16; key_count ++) {
    digitalWrite(scl_pin, LOW);
    if (!digitalRead(sdo_pin)) key_state = key_count;
    digitalWrite(scl_pin, HIGH);
  }
  return key_state;
}

/* 키패드 비밀번호 또는 RFID UID 인증에 성공하면 실행 */
void doorOpen() {
  digitalWrite(green_led, HIGH); // 초록색 LED 켜기
  servo.write(servo_angle_open); // 서보모터 잠금 풀기
  Serial.println("잠금을 해제합니다.");

  delay(5000); // 문 닫힘 상태를 인식하기 전에 5초 대기 (인증 통과 후 5초동안 문이 닫혀 있으면 자동으로 잠김)
  while (digitalRead(door_pin) == HIGH) { yield(); } // 문 열려 있는 동안 대기 (문 다시 닫힐 떄까지)
  Serial.print("문이 닫혔습니다. ");

  servo.write(servo_angle_lock); // 서보모터 다시 잠그기
  delay(100); // 서보모터 작동 대기
  digitalWrite(green_led, LOW); // 초록색 LED 끄기
  Serial.println("다시 잠급니다.");
}
