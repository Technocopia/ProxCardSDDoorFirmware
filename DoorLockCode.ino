//green
#define W0 27
//white
#define W1 14
#define DoorP 25
#define DoorE 26
#define button 32
#include <Arduino.h>
//#include "codes.h" // also where #define sitecode is
//#define NUM_STATIC_CARDS (sizeof(cards) / sizeof(unsigned long int))
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#if defined ARDUINO_ARCH_ESP32
#include <FS.h>
#endif
#define numCards 300
// Configuration that we'll store on disk

#define sitecode 17

char bits[80];
int bitcnt = 0;
unsigned long long bitw = 0;
unsigned int timeout = 1000;
boolean valid = true;
int parity(unsigned long int x);
const char *filename = "/db.json";  // <- SD library uses 8.3 filenames
const size_t CAPACITY = JSON_ARRAY_SIZE(300);
// allocate the memory for the document
StaticJsonDocument<CAPACITY> doc_read;
StaticJsonDocument<CAPACITY> doc_write;
JsonArray array_read = doc_read.to<JsonArray>();
JsonArray array_write = doc_write.to<JsonArray>();
//portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
// Wiegand 0 bit ISR. Triggered by wiegand 0 wire.
bool loadCardMode = false;
long startLoadCardMode = 0;
#if defined ARDUINO_ARCH_ESP32
void IRAM_ATTR W0ISR() {
#else
	void W0ISR() {
#endif

	if (digitalRead(W0))
		return;
	//portEXIT_CRITICAL(&mux);
	bitw = (bitw << 1) | 0x0; // shift in a 0 bit.
	bitcnt++;               // Increment bit count
	timeout = millis();         // Reset timeout
	//portEXIT_CRITICAL(&mux);

}

// Wiegand 1 bit ISR. Triggered by wiegand 1 wire.
#if defined ARDUINO_ARCH_ESP32
void IRAM_ATTR W1ISR() {
#else
	void W1ISR() {
#endif
	if (digitalRead(W1))
		return;
	//portEXIT_CRITICAL(&mux);
	bitw = (bitw << 1) | 0x1; // shift in a 1 bit
	bitcnt++;               // Increment bit count
	timeout = millis();         // Reset Timeout
	//portEXIT_CRITICAL(&mux);

}

// Prints the content of a file to the Serial
void printFile(const char *filename) {
	// Open file for reading
	File file = (File) SD.open(filename, FILE_READ);
	if (!file) {
		Serial.println(F("Failed to read file"));
		return;
	}

	// Extract each characters by one by one
	while (file.available()) {
		Serial.print((char) file.read());
	}
	Serial.println();

	// Close the file (File's destructor doesn't close the file)
	file.close();
}

void setup() {
	// put your setup code here, to run once:
	Serial.begin(115200);
	// create an empty array
	pinMode(SS, INPUT_PULLUP);
	pinMode(SCK, INPUT_PULLUP);
	pinMode(MOSI, INPUT_PULLUP);
	pinMode(MISO, INPUT_PULLUP);
	// Initialize SD library
#if defined ARDUINO_ARCH_ESP32
	while (!SD.begin(SS, SPI, 4000000, "/sd", 5)) {
#else
		while (!SD.begin(SS)) {
#endif
		Serial.println(F("Failed to initialize SD library"));
		delay(1000);
	}
	if (!SD.exists(filename)) {
		File fileTest = (File) SD.open(filename, FILE_WRITE);
		Serial.println(F("Database missing"));
		fileTest.close();
		//ESP.restart();
	}

	DeserializationError err;
	do {
		File f_l = (File) SD.open(filename, FILE_READ);
		if (f_l) {
			Serial.println(
					"File " + String(filename) + " timeout: "
							+ String(f_l.getTimeout()));
//		// parse a JSON array
			err = deserializeJson(doc_read, f_l);
			f_l.close();
			Serial.println("deserialize result " + String(err.c_str()));

			if (err == DeserializationError::Ok) {
				Serial.println(F("on boot Print config file..."));
				printFile(filename);
				Serial.println("Number of cards " + String(array_read.size()));
			}
		} else {
			Serial.println("File open failed!");
		}
	} while (err != DeserializationError::Ok);
	// Dump config file
//	Serial.println(F("Print config file..."));
//	printFile(filename);

	pinMode(W0, INPUT_PULLUP);
	pinMode(W1, INPUT_PULLUP);
	pinMode(button, INPUT_PULLUP);
	pinMode(0, INPUT_PULLUP);
	pinMode(DoorP, OUTPUT);
	pinMode(DoorE, OUTPUT);
	digitalWrite(DoorP, LOW);
	digitalWrite(DoorE, HIGH);

	// Set up edge interrupts on wiegand pins.

	// Wiegand is used by many rfid card readers and mag stripe readers.

	// It has two wires. a '1' wire and a '0' wire. It will output a short pulse on
	//    one of these wires to indicate to you that it has read a 1 or a 0 bit.
	//    you need to count the pulses. It's a lot like rotary phone wheels
	//    only in binary and with 2 wires.

	// make sure W0 ans W1 trigger interrupts on a 1->0 transition.
	attachInterrupt(digitalPinToInterrupt(W0), W0ISR, FALLING);
	attachInterrupt(digitalPinToInterrupt(W1), W1ISR, FALLING);
	Serial.println("Hello World!");

	for (int i = 0; i < sizeof(bits); i++)
		bits[i] = 0;
	digitalWrite(DoorP, LOW);
	digitalWrite(DoorE, HIGH);
}

void open() {
	Serial.println("Opening door");
	digitalWrite(DoorP, HIGH); // Open door.
	delay(100);
	digitalWrite(DoorE, LOW);
	delay(200);
	digitalWrite(DoorE, HIGH);
	delay(100);
	digitalWrite(DoorE, LOW);
	delay(200);
	digitalWrite(DoorE, HIGH);
	delay(100);
	digitalWrite(DoorE, LOW);
	delay(200);
	digitalWrite(DoorE, HIGH);
	delay(5000);
	digitalWrite(DoorE, LOW);
	delay(200);
	digitalWrite(DoorE, HIGH);
	digitalWrite(DoorP, LOW); // close

	Serial.println("Locking door");
}

bool haveCard() {
	return (((millis() - timeout) > 500) && bitcnt > 30);
}

long int getIDOfCurrentCard() {
	unsigned long long bitwtmp = bitw;
	int bitcnttmp = bitcnt;
	bitcnt = 0;
	bitw = 0;
	//portEXIT_CRITICAL(&mux);
	for (int i = bitcnttmp; i != 0; i--)
		Serial.print((unsigned int) (bitwtmp >> (i - 1) & 0x00000001)); // Print the card number in binary
	Serial.print(" (");
	Serial.print(bitcnttmp);
	Serial.println(")");
	boolean ep, op;
	unsigned int site;
	unsigned long int card;

	// Pick apart card.
	site = (bitwtmp >> 25) & 0x00007f;
	card = (bitwtmp >> 1) & 0xffffff;
	op = (bitwtmp >> 0) & 0x000001;
	ep = (bitwtmp >> 32) & 0x000001;

	// Check parity
	if (parity(site) != ep)
		valid = false;
	if (parity(card) == op)
		valid = false;

	// Print card info
	Serial.print("Got " + String());
	Serial.print("Site: ");
	Serial.println(site);
	Serial.print("Card: ");
	Serial.println(card);
	Serial.print("ep: ");
	Serial.print(parity(site));
	Serial.println(ep);
	Serial.print("op: ");
	Serial.print(parity(card));
	Serial.println(op);

	valid = true;
	return card;
}
#if defined ARDUINO_ARCH_ESP32
void IRAM_ATTR loop() {
#else
	void loop() {
#endif
	if (!digitalRead(button) && !haveCard()) {
		open();
		if (haveCard()) {

		}
		return;
	}
	if (!digitalRead(0) && loadCardMode == false) {
		// load card mode
		loadCardMode = true;
		startLoadCardMode = millis();
		Serial.println("Start load card mode");
		digitalWrite(DoorE, LOW);
		delay(200);
		digitalWrite(DoorE, HIGH);
		delay(100);
		digitalWrite(DoorE, LOW);
		delay(200);
		digitalWrite(DoorE, HIGH);
		delay(100);
	}
	if (loadCardMode && (millis() - startLoadCardMode) > 20000) {
		loadCardMode = false;
		Serial.println("End load Card Mode, writing");
		// serialize the array and send the result to Serial
		File file = (File) SD.open(filename, FILE_WRITE);
		serializeJson(doc_read, file);
		file.flush();
		file.close();
		printFile(filename);
		digitalWrite(DoorE, LOW);
		delay(200);
		digitalWrite(DoorE, HIGH);
		delay(100);
		digitalWrite(DoorE, LOW);
		delay(200);
		digitalWrite(DoorE, HIGH);
		delay(100);
	}
	if (haveCard()) { // The reader hasn't sent a bit in 2000 units of time. Process card.
		unsigned long int card = getIDOfCurrentCard();
		for (JsonVariant v : array_read) {
			//Serial.println("Checking "+String(cards[i]));
			if (v.as<int>() == card) { // Is it in the DB?
				Serial.println(
						"\t\tMatch! " + String(card) + " to card form list "
								+ String(v.as<int>()));
				if (!loadCardMode) open();
				return;
			}
		}
		if (loadCardMode) {
			Serial.println("Adding card! " + String(card));
			array_read.add(card);
			startLoadCardMode = millis();
		} else {
			Serial.println("Error! " + String(card));
			//digitalWrite(DoorE, HIGH);
			digitalWrite(DoorE, LOW);
			delay(500);
			digitalWrite(DoorE, HIGH);
			delay(500);
			digitalWrite(DoorE, LOW);
			delay(500);
			digitalWrite(DoorE, HIGH);
		}
	} else {
		delay(30);
	}

}

int parity(unsigned long int x) {
	unsigned long int y;
	y = x ^ (x >> 1);
	y = y ^ (y >> 2);
	y = y ^ (y >> 4);
	y = y ^ (y >> 8);
	y = y ^ (y >> 16);
	return y & 1;
}
