/**@file Emhaette.ino */

#include <SPI.h>
#include <Ethernet.h>

/**
 * MAC adressen på vores Ethernet controller
 */
byte mac[] = {
        0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

/**
 * IP adressen på vores webserver
 */
IPAddress ip(192, 168, 1, 177);

/**
 * Porten på vores webserver. Port 80 er standard HTTP
 */
EthernetServer server(80);

/**
 * Indeholder den header som serveren modtager
 */
String header;

/**
 * Status variabel for emhætte blæser hastighed
 */
String output6State = "slow";

/**
 * Status variabel for om emhætten er tændt eller slukket
 */
String output5State = "off";

/**
 * Status variabel for om lyset på emhætten er tændt eller slukket
 */
String output4State = "off";

/**
 * Output PIN for blæseren på emhætten
 */
const int output6 = 6;

/**
 * Output PIN for emhætte
 */
const int output5 = 5;

/**
 * Output PIN for lyset på emhætten
 */
const int output4 = 4;

/**
 * Nuværende tidspunkt
 */
unsigned long currentTime = millis();

/**
 * Forrige tidspunkt
 */
unsigned long previousTime = 0;

/**
 * Timeout tid for webserveren
 */
const long timeoutTime = 2000;

/**
 * Første funktion som kører
 * Starter webserveren, indstiller pinModes og starter serial forbindelsen
 */
void setup() {
    // Åben serial forbindelsen og vent til den er åbnet
    Serial.begin(9600);
    while (!Serial) { ; // wait for serial port to connect. Needed for native USB port only
    }

    Serial.println("TEC emhætte, Morten og Virtus");

    // Sæt output variablerne til at være output
    pinMode(output5, OUTPUT);
    pinMode(output4, OUTPUT);
    pinMode(output6, OUTPUT);

    // Sæt outputs til at være LOW
    digitalWrite(output6, LOW);
    digitalWrite(output5, LOW);
    digitalWrite(output4, LOW);

    // Start for ethernet
    Ethernet.begin(mac, ip);

    // Tjek om Arduino'en har ethernet hardware
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println("Ethernet skjold blev ikke fundet");
        while (true) {
            delay(1); // Gør ingenting, vi kan ikke køre programmet uden et ethernet skjold
        }
    }

    if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("Ethernet kabel er ikke forbundet");
    }

    // Start for serveren
    server.begin();
    Serial.print("Serveren kører på ");
    Serial.println(Ethernet.localIP());
}

/**
 * Sætter LED hastighed for hvor hurtigt den skal blinke
 */
void handleSpeed() {
    if (output6State == "slow" && output5State == "on") {
        digitalWrite(output6, HIGH);
        delay(1000);
        digitalWrite(output6, LOW);
        delay(1000);
    } else if (output6State == "fast" && output5State == "on") {
        digitalWrite(output6, HIGH);
        delay(250);
        digitalWrite(output6, LOW);
        delay(250);
    }
}

/**
 * Håndterer HTTP headers, sætter output state og sender signal til Arduino
 */
void handleEndpoints() {
    // Her tjekker vi på hvilke GET requests klienten sender
    if (header.indexOf("GET /5/on") >= 0) {
        Serial.println("Emhætte tændt");
        output5State = "on";
        digitalWrite(output5, HIGH);
    } else if (header.indexOf("GET /5/off") >= 0) {
        Serial.println("Emhætte slukket");
        output5State = "off";
        digitalWrite(output5, LOW);
    } else if (header.indexOf("GET /4/on") >= 0) {
        Serial.println("Emhætte lys tændt");
        output4State = "on";
        digitalWrite(output4, HIGH);
    } else if (header.indexOf("GET /4/off") >= 0) {
        Serial.println("Emhætte lys slukket");
        output4State = "off";
        digitalWrite(output4, LOW);
    } else if (header.indexOf("GET /6/fast") >= 0) {
        Serial.println("Blæser hurtig");
        output6State = "fast";
    } else if (header.indexOf("GET /6/slow") >= 0) {
        Serial.println("Blæser langsom");
        output6State = "slow";
    }
}

/**
 * Genererer knapper til HTML-siden afhængig af output states
 */
void printButtons(EthernetClient client) {
    String msgstatus5 = output5State == "on" ? "tændt" : "slukket";
    client.println("<p>Emhætte: " + msgstatus5 + "</p>");
    if (output5State == "off") {
        client.println("<p><a href=\"/5/on\"><button class=\"button\">TÆND</button></a></p>");
    } else {
        client.println("<p><a href=\"/5/off\"><button class=\"button button2\">SLUK</button></a></p>");
    }

    // Vis nuværende status for blæseren
    String msgstatus6 = output6State == "fast" ? "hurtig" : "langsom";
    client.println("<p>Udsugning: " + msgstatus6 + "</p>");
    if (output6State == "slow") {
        client.println("<p><a href=\"/6/fast\"><button class=\"button\">HURTIG</button></a></p>");
    } else {
        client.println("<p><a href=\"/6/slow\"><button class=\"button button2\">LANGSOM</button></a></p>");
    }

    // Vis nuværende status for lyset på emhætten
    String msgstatus4 = output4State == "on" ? "tændt" : "slukket";
    client.println("<p>Lys: " + msgstatus4 + "</p>");
    if (output4State == "off") {
        client.println("<p><a href=\"/4/on\"><button class=\"button\">TÆND</button></a></p>");
    } else {
        client.println("<p><a href=\"/4/off\"><button class=\"button button2\">SLUK</button></a></p>");
    }
}

/**
 * loop() metoden løber konstant, lige så hurtigt som CPU'en tillader
 */
void loop() {
    // Tjek om der er indkommende klienter
    EthernetClient client = server.available();

    // Tjek blæser hastighed og blink LED'en alt efter hvilken hastighed den er sat til, men start kun for LED'en hvis emhætten er tændt
    handleSpeed();

    if (client) {
        Serial.println("Ny klient");

        // Gem data der kommer fra nuværende klient
        String currentLine = "";

        currentTime = millis();
        previousTime = currentTime;

        // Loop mens klienten er forbundet
        while (client.connected() && currentTime - previousTime <= timeoutTime) {
            currentTime = millis();

            // Hvis der er bytes som kan læses fra klienten
            if (client.available()) {
                char c = client.read();
                Serial.write(c);
                header += c;

                // Hvis nuværende byte er en newline karakter
                if (c == '\n') {
                    // Hvis der bliver sendt 2 newline karakterer i streg, så er det enden af HTTP requesten
                    if (currentLine.length() == 0) {
                        // Send en HTTP respons til klienten
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();

                        // Slukker og tænder for outputs via headers
                        handleEndpoints();

                        // Vis HTML websiden
                        client.println("<!DOCTYPE html><html lang=\"da\">");
                        client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                        client.println("<link rel=\"icon\" href=\"data:,\">");
                        client.println("<meta charset=\"UTF-8\"></head>");

                        client.println("<body><div class=\"container\">");
                        client.println("<div>");
                        client.println("<h1>TEC Emhætte Controller</h1>");
                        client.println("<button class=\"button\" id='voice-btn'>Stemmestyring</button>");
                        client.println("<h5>Klik på Stemmestyring knap og sig en af følgende kommandoer</h5>");

                        // region CSS start
                        client.println("<style> html {");
                        client.println("font-family: Helvetica;");
                        client.println("display: inline-block;");
                        client.println("margin: 0px auto;");
                        client.println("text-align: center;");
                        client.println("}");
                        client.println(".button {");
                        client.println("background-color: #195B6A;");
                        client.println("border: none;");
                        client.println("color: white;");
                        client.println("padding: 16px 40px;");
                        client.println("text-decoration: none;");
                        client.println("font-size: 30px;");
                        client.println("margin: 2px;");
                        client.println("cursor: pointer;");
                        client.println("}");
                        client.println(".button2 {");
                        client.println("background-color: #77878A;");
                        client.println("}");
                        client.println("table td {");
                        client.println("padding: 10px;");
                        client.println("}");
                        client.println(".container {");
                        client.println("padding: 150px;");
                        client.println("width: fit-content;");
                        client.println("text-align: center;");
                        client.println("margin: 0 auto;");
                        client.println("position: relative;");
                        client.println("}");
                        client.println(".container::before {");
                        client.println("position: absolute;");
                        client.println("top: 0;");
                        client.println("left: 0;");
                        client.println("bottom: 0;");
                        client.println("right: 0;");
                        client.println("background-image: url(data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA1MTIgNTEyIiB4bWxuczp2PSJodHRwczovL3ZlY3RhLmlvL25hbm8iPjxwYXRoIGQ9Ik0xNDEuMjQgNC40MTRoMjI5LjUxN3YzNzAuNzZIMTQxLjI0eiIgZmlsbD0iIzkwYTRhZSIvPjxwYXRoIGQ9Ik01MDMuMDg0IDQyNi45M0wzNzAuNzU4IDI3OC4wN0gxNDEuMjRMOC45MTYgNDI2LjkzYy0yLjY0OCAyLjk4NC00LjY4NyA2LjQxLTYuMTg4IDEwLjAzN2g1MDYuNTM2Yy0xLjQ5Mi0zLjYzLTMuNTMyLTcuMDU0LTYuMTgtMTAuMDM3eiIgZmlsbD0iI2IwYmVjNSIvPjxwYXRoIGQ9Ik0yLjcyOCA0MzYuOTY2Qy45OSA0NDEuMjAzIDAgNDQ1LjczIDAgNDUwLjM5MnY0OC4zNjZjMCA0Ljg3MyAzLjk1NSA4LjgyOCA4LjgyOCA4LjgyOGg0OTQuMzQ1YzQuODczIDAgOC44MjgtMy45NTUgOC44MjgtOC44Mjh2LTQ4LjM2NmMwLTQuNjUyLS45OS05LjItMi43MzctMTMuNDI3SDIuNzI4eiIgZmlsbD0iIzc4OTA5YyIvPjxwYXRoIGQ9Ik0xNzYuNTUyIDQ1NC42MkgzMzUuNDV2MzUuM0gxNzYuNTUyeiIgZmlsbD0iI2FmYjQyYiIvPjxnIGZpbGw9IiNlY2VmZjEiPjxjaXJjbGUgY3g9IjM3MC43NTkiIGN5PSI0NzIuMjc2IiByPSI4LjgyOCIvPjxjaXJjbGUgY3g9IjQwNi4wNjkiIGN5PSI0NzIuMjc2IiByPSI4LjgyOCIvPjwvZz48L3N2Zz4=);");
                        client.println("background-position: center;");
                        client.println("background-repeat: no-repeat;");
                        client.println("opacity: .4;");
                        client.println("content: '';");
                        client.println("z-index: -1;");
                        client.println("background-size: cover;");
                        client.println("}");
                        client.println("p {");
                        client.println("font-weight: 600;");
                        client.println("} </style>");
                        // endregion CSS slut

                        // Kommandoer HTML start
                        client.println("<table style=\"text-align: center;margin: 0 auto;\">");
                        client.println("<tbody>");
                        client.println("<tr>");
                        client.println("<td>Turn On Extractor Hood</td>");
                        client.println("<td>Turn Off Extractor Hood</td>");
                        client.println("</tr>");
                        client.println("<tr>");
                        client.println("<td>Exhaust Speed Slow</td>");
                        client.println("<td>Exhaust Speed Fast</td>");
                        client.println("</tr>");
                        client.println("<tr>");
                        client.println("<td>Turn On Light</td>");
                        client.println("<td>Turn Off Light</td>");
                        client.println("</tr>");
                        client.println("</tbody>");
                        client.println("</table>");
                        // Kommandoer slut

                        // Vis nuværende status for emhætten
                        printButtons(client);

                        client.println("<div>");
                        client.println("<p class=\"output\"></p>");
                        client.println("</div>");

                        // region JS start
                        // Udskriv alt vores Javascript kode til stemmestyring

                        // Web Speech API https://developer.mozilla.org/en-US/docs/Web/API/Web_Speech_API
                        // SpeechRecognition Interface https://developer.mozilla.org/en-US/docs/Web/API/SpeechRecognition
                        // JSpeech Grammar Format, bruges til at priotere vores kommandoer højere, "weight", https://www.w3.org/TR/2000/NOTE-jsgf-20000605/
                        client.println("<script>var SpeechRecognition = SpeechRecognition || webkitSpeechRecognition;");
                        client.println("var SpeechGrammarList = SpeechGrammarList || webkitSpeechGrammarList;");
                        client.println("var SpeechRecognitionEvent = SpeechRecognitionEvent || webkitSpeechRecognitionEvent;");
                        client.println("var commands = ['turn on extractor hood', 'turn on light', 'exhaust speed slow', 'exhaust speed fast', 'turn off extractor hood', 'turn off light'];");
                        client.println("var grammar = '#JSGF V1.0; grammar extractorHoodCommands; public <command> = ' + commands.join(' | ') + ' ;';");
                        client.println("var recognition = new SpeechRecognition();");
                        client.println("var speechRecognitionList = new SpeechGrammarList();");
                        client.println("speechRecognitionList.addFromString(grammar, 1);");
                        client.println("recognition.grammars = speechRecognitionList;");
                        client.println("recognition.continuous = false;");
                        client.println("recognition.lang = 'en-US';");
                        client.println("recognition.interimResults = false;");
                        client.println("recognition.maxAlternatives = 1;");
                        client.println("var diagnostic = document.querySelector('.output');");
                        client.println("document.getElementById('voice-btn').addEventListener('click', function (event) {");
                        client.println("recognition.start();");
                        client.println("console.log('Ready to receive a color command.');});");
                        client.println("recognition.onresult = function(event) {");
                        client.println("var command = event.results[0][0].transcript;");
                        //  client.println("console.log('Command: ' + command);");
                        client.println("diagnostic.textContent = 'Result received: ' + command + '.';");
                        client.println("console.log('Confidence: ' + event.results[0][0].confidence);");
                        client.println("command = command.toLowerCase();");
                        client.println("switch (command) {");
                        client.println("case 'turn on extractor hood':");
                        // client.println("console.log('tænd for emhætten');");
                        client.println("if (confirm('Tænd for emhætten?')) {");
                        client.println("location.pathname = '/5/on';}");
                        client.println("break;");
                        client.println("case 'turn on light':");
                        // client.println("console.log('tænd for lyset på emhætten');");
                        client.println("if (confirm('Tænd for lyset på emhætten?')) {");
                        client.println("location.pathname = '/4/on';}");
                        client.println("break;");
                        client.println("case 'exhaust speed slow':");
                        //client.println("console.log('sæt blæser hastighed til langsom');");
                        client.println("if (confirm('Sæt blæser hastighed til langsom?')) {");
                        client.println("location.pathname = '/6/slow';}");
                        client.println("break;");
                        client.println("case 'exhaust speed fast':");
                        client.println("console.log('sæt blæser hastighed til hurtig');");
                        client.println("if (confirm('Sæt blæser hastighed til hurtig?')) {");
                        client.println("location.pathname = '/6/fast';}");
                        client.println("break;");
                        client.println("case 'turn off extractor hood':");
                        // client.println("console.log('sluk for emhætten');");
                        client.println("if (confirm('Sluk for emhætten?')) {");
                        client.println("location.pathname = '/5/off';}");
                        client.println("break;");
                        client.println("case 'turn off light':");
                        // client.println("console.log('sluk for lyset');");
                        client.println("if (confirm('Sluk for lyset?')) {");
                        client.println("location.pathname = '/4/off';}");
                        client.println("break;");
                        client.println("default:");
                        client.println("let found = tryToFindCommandBetter(command);");
                        client.println("if (found !== false) {");
                        // client.println("console.log('Fandt kommando ved at søge commands igennem');");
                        // client.println("console.log('Kommando fundet:' + found);");
                        client.println("} else {");
                        //client.println("console.log('Kunne ikke finde kommando')");
                        client.println("}");
                        client.println("break;");
                        client.println("}");
                        client.println("};");
                        client.println("function tryToFindCommandBetter(speechCommand) {");
                        client.println("let found = false;");
                        client.println("commands.forEach(cmd => {");
                        client.println("let splitSpeechCommand = speechCommand.split(' ');");
                        client.println("let splitCommand = cmd.split(' ');");
                        client.println("");
                        client.println("let containsEverySubString = splitCommand.every(val => {");
                        // client.println("console.log('val: ' + val);");
                        // client.println("console.log('indexOf: ' + splitCommand.indexOf(val));");
                        // client.println("console.log(splitCommand);");
                        client.println("return splitSpeechCommand.indexOf(val) >= 0;");
                        client.println("});");
                        // client.println("console.log(containsEverySubString);");
                        client.println("if (containsEverySubString) {");
                        client.println("found = cmd;");
                        client.println("}");
                        client.println("});");
                        client.println("return found;");
                        client.println("}");
                        client.println("recognition.onspeechend = function() {");
                        client.println("recognition.stop();");
                        client.println("};");
                        client.println("recognition.onnomatch = function(event) {");
                        client.println("diagnostic.textContent = 'Ingen kommando fundet, prøv igen';");
                        client.println("};");
                        client.println("recognition.onerror = function(event) {");
                        client.println("diagnostic.textContent = 'Fejl i stemmestyring: ' + event.error;");
                        client.println("};</script>");
                        // endregion JS slut

                        client.println("</div></div></body></html>");

                        // HTTP responsen ender med endnu en newline karakter
                        client.println();

                        // Break ud af loopet
                        break;
                    } else {
                        // Hvis vi fik en newline karakter, så clear den nuværende linje
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    // Hvis det vi har fået IKKE er en "carriage return" karakter, så tilføjes det til nuværende linje
                    currentLine += c;      // add it to the end of the currentLine
                }
            }
        }

        // Tøm header variablen
        header = "";

        // Luk for forbindelsen
        client.stop();
        Serial.println("Klient forbindelse afsluttet");
        Serial.println("");
    }
}
