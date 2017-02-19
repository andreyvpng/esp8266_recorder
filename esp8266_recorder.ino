#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define EEPROM_MAX_ADDR 4096
#define EEPROM_MIN_ADDR 1

int buttonState = HIGH;
int lastButtonState;
long lastDebounceTime = 0;
long debounceDelay = 50;
int count = 0;
int reading;

String table;
String table_saved;

int class_9_criterion[] = {22, 24, 26};
int class_10_11_criterion[] = {28 , 30, 32};

const char *ssid = "ESPap";
const char *password = "123456789";

ESP8266WebServer server(80);

void countZero(){
  count = 0;
}

/*
 * Возвращает оценку ученика
 */

String evaluate(int count, String& class_){
  int* criterion;
  if(class_ == "9") 			 criterion = &class_9_criterion[0];
  if(class_ == "class_10") criterion = &class_10_11_criterion[0];
  if(class_ == "class_11") criterion = &class_10_11_criterion[0];

  if(class_ == "-") 					  return "-";  
  if(count >= *(criterion+2))   return "5";  
  if(count >= *(criterion+1))   return "4";  
  if(count >= *(criterion))     return "3";
  if(count <  *(criterion))  	  return "2";
}

/*
 * Считывает сохранненую таблицу из EEPROM и записывает
 * в переменную "table_saved" html-код таблицы
 */

void assign_saved(){
  String table_eeprom;
  table_eeprom = read_StringEE(EEPROM_MIN_ADDR, EEPROM.read(0));
  int length = EEPROM.read(0);

  Serial.println(length);

  table_saved = "";
  for(int i = 0; i <= length + 1; i++){
    if(i == 0){
      table_saved = table_saved + "<tr><td>";
    }
    if(table_eeprom[i] == ','){
      table_saved = table_saved + "</td><td>";
      i++;
    }
    if(table_eeprom[i] == '!'){ 
      table_saved = table_saved + "</td></tr>";
      i++;
    }
    if((table_eeprom[i-1] == '!') && (i != length)){
      table_saved = table_saved + "<tr><td>";
    }
    if(i == length) break;
    table_saved = table_saved + table_eeprom[i];
  }
}

/*
 * Страница принимающая Post-запрос для добавляения ученика в таблицу.
 * Агрументы: name, class_ю
 * Записывает данные для таблицы в переменную "table".
 */

void addToTable(){
  String name = server.arg("name");
  String class_ = server.arg("class_");

  if(name != ""){
    table = table + name + "," + count + "," + class_ + "," + evaluate(count, class_) + "!";
    countZero();
  }
  server.send ( 200, "text/html", "<script language='JavaScript'>window.location.href = '/'</script>");
}

/*
 * Страница принимающая Post-запрос для удаления содержимого временной таблицы.
 */

void delTable(){
  table = "";
  server.send ( 200, "text/html", "<script language='JavaScript'>window.location.href = '/'</script>");
}

/*
 * Страница принимающая Post-запрос для сохранения перменной "table" в EEPROM.
 */

void saveTable(){
  EEPROM.write(0, table.length());
  write_StringEE(EEPROM_MIN_ADDR, table);
  EEPROM.commit();
  assign_saved();
  server.send ( 200, "text/html", "<script language='JavaScript'>window.location.href = '/'</script>");
}

/*
 * Страница принимающая Post-запрос для удаления таблицы из EEPROM.
 */

void delTable_saved(){
  EEPROM.write(0, 0);
  table_saved = "";
  server.send ( 200, "text/html", "<script language='JavaScript'>window.location.href = '/'</script>");
}

/*
 * Главная страница
 */

void handleRoot() {
  if(EEPROM.read(0) != 0){
    assign_saved();
  }
  String temp =
  "<html>\
  <head>\
  <title>ESP8266</title>\
  <meta charset='utf-8'>\
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>\
  <style>\
	  body {\
	    font-family: Sans-Serif;\
	    Color: #333;\
	    max-width: 750px;\
	    margin: 0 auto;\
	   }\
	  table {\
	    border-collapse: collapse;\
	    width: 100%;\
	  }\
	  th, td {\
	    text-align: left;\
	    padding: 8px;\
	  }\
	  tr:nth-child(even){background-color: #f2f2f2}\
	  input[type=submit]{\
	    background-color: #4CAF50;\
	    border: none;\
	    color: white;\
	    padding: 12px 20px;\
	    text-align: center;\
	    text-decoration: none;\
	    display: inline-block;\
	    font-size: 16px;\
	  }\
	  input.reset{background-color: #f44336;  }\
	  input[type=text] {\
		  padding: 12px 20px;\
		  margin: 8px 0;\
		  box-sizing: border-box;\
	  }\
	  select {\
	    padding: 12px 20px;\
	    margin: 8px 0;\
	    box-sizing: border-box;\
	  }\
  </style>\
  </head>\
  <body><br>\
  <h3>Количество: " + String(count) + "</h3>\
  <form action='/addToTable'>\
  <input type='text'maxlength='40' name='name'>\
  <select size='1' name='class_'>\
  <option value='-'>Выберите класс</option>\
  <option value='9'>9 класс</option>\
  <option value='10'>10 класс</option>\
  <option value='11'>11 класс</option>\
  </select>\
  <input class='submit' type='submit' value='Добавить'>\
  </form>\
  <h3 align='center'>Таблица:</h3>";

  if(table.length() > 0){
    temp = temp + "<table><tr>\
    <th>Фамилия Имя</th><th>Количество</th>\
    <th>Класс</th><th>Оценка</th></tr>";

    int length = table.length();
    for(int i = 0; i < length; i++){
      if(i == 0){
        temp = temp + "<tr><td>";
      }
      if(table[i] == ','){
        temp = temp + "</td><td>";
        i++;
      }
      if(table[i] == '!'){ 
        temp = temp + "</td></tr>";
        i++;
      }
      if((table[i-1] == '!') && (i != length)){
        temp = temp + "<tr><td>";
      }
      if(i == length) break;
      temp = temp + table[i];
    }
    temp = temp + "</table>";
    temp = temp +  "<br><form action='/delTable'><input class='reset' type='submit' value='Очистить таблицу'></form>";
    temp = temp +  "<form action='/saveTable'><input class='submit' type='submit' value='Сохранить таблицу'></form>";
  } else{
    temp = temp + "<p align='center'>Нет данных</p>";
  }
  temp = temp + "<h3 align='center'>Сохраненная таблица:</h3>";
  if(table_saved.length() > 0){
          temp = temp + "<table>\
                <tr>\
                <th>Фамилия Имя</th>\
                <th>Количество</th>\
                <th>Класс</th>\
                <th>Оценка</th>\
                </tr>";
    temp = temp + table_saved;
    temp = temp + "</table>";
    temp = temp + "<br><form action='/delTable_saved'><input class='reset' type='submit' value='Удалить таблицу'></form>";
  } else {
    temp = temp + "<p align='center'>Нет данных</p>";
  }
  temp = temp + "<hr>\
  							<h3>Оценивание:</h3>\
  							<p><b>10-11 класс:</b></p>\
  							<p>'3' - 28; '4' - 30; '5' - 32;</p>\
  							<p><b>9 класс:</b></p>\
  							<p>'3' - 22; '4' - 24; '5' - 26;</p>";
  temp = temp + "</body></html>";
  server.send ( 200, "text/html", temp);
}


void setup() {
	delay(1000);
	Serial.begin(115200);
	EEPROM.begin(4096);
	Serial.println();
	Serial.print("Configuring access point...");
	WiFi.softAP(ssid, password);

	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
	server.on("/addToTable", addToTable);
	server.on("/delTable", delTable);
	server.on("/saveTable", saveTable);
	server.on("/delTable_saved", delTable_saved);
	server.on("/", handleRoot);
	server.begin();
	Serial.println("HTTP server started");
	assign_saved();
}

void loop() {
	reading = digitalRead(2);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == HIGH) {
        count++;
      }
    }
  }
  lastButtonState = reading;
  server.handleClient();
}

/*
 * Для работы с EEPROM
 */
bool write_StringEE(int Addr, String input) {
  char cbuff[input.length()+1];
  input.toCharArray(cbuff,input.length()+1);
  return eeprom_write_string(Addr,cbuff);
}
String read_StringEE(int Addr, int length) {
  char cbuff[length+1];
  eeprom_read_string(Addr, cbuff, length+1);
  
  String stemp(cbuff);
  return stemp;
}
boolean eeprom_is_addr_ok(int addr) {
  return ((addr >= EEPROM_MIN_ADDR) && (addr <= EEPROM_MAX_ADDR));
}
boolean eeprom_write_bytes(int startAddr, const byte* array, int numBytes) {

  int i;

  if (!eeprom_is_addr_ok(startAddr) || !eeprom_is_addr_ok(startAddr + numBytes)) {
    return false;
  }

  for (i = 0; i < numBytes; i++) {
    EEPROM.write(startAddr + i, array[i]);
  }

  return true;
}
boolean eeprom_write_string(int addr, const char* string) {

  int numBytes;
  numBytes = strlen(string) + 1;

  return eeprom_write_bytes(addr, (const byte*)string, numBytes);
}
boolean eeprom_read_string(int addr, char* buffer, int bufSize) {
  byte ch;
  int bytesRead;

  if (!eeprom_is_addr_ok(addr)) {
    return false;
  }

  if (bufSize == 0) {
    return false;
  }
  if (bufSize == 1) {
    buffer[0] = 0;
    return true;
  }

  bytesRead = 0;
  ch = EEPROM.read(addr + bytesRead);
  buffer[bytesRead] = ch;
  bytesRead++;

  while ( (ch != 0x00) && (bytesRead < bufSize) && ((addr + bytesRead) <= EEPROM_MAX_ADDR) ) {
    ch = EEPROM.read(addr + bytesRead);
    buffer[bytesRead] = ch;
    bytesRead++;
  }

  if ((ch != 0x00) && (bytesRead >= 1)) {
    buffer[bytesRead - 1] = 0;
  }

  return true;
}
