#include <WiFi.h> 


const char* ssid = "Tenda_44AC00";     
const char* password = ""; //xoa truoc khi dua len github 

void setup() {
    Serial.begin(115200); 
    WiFi.begin(ssid, password);  

    Serial.print("Connecting...");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nSuccess!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void loop() {
}
