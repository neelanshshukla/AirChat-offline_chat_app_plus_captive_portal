// --- LIBRARIES ---
#include <WiFi.h>        // Includes the library needed to use the ESP32's Wi-Fi hardware
#include <WebServer.h>   // Includes the library that turns the ESP32 into a web server
#include <DNSServer.h>   // Includes the library for DNS, which allows us to create a "Captive Portal"

// --- GLOBAL OBJECTS ---
DNSServer dnsServer;     // Creates a DNS server object to handle network redirects
WebServer server(80);    // Creates a web server object listening on port 80 (the default HTTP port)

// --- DATA STRUCTURE ---
// Creates a custom template to hold two pieces of text: a name and a message
struct GuestMessage 
{
  String name;           // Variable to store the sender's name
  String text;           // Variable to store the sender's message
};

GuestMessage airchat[20]; // Creates an array (a list) that can hold up to 20 GuestMessage objects
int messageCount = 0;     // Keeps track of how many messages have been saved (starts at 0)

// --- THE FRONTEND (HTML/CSS/JS) ---
// The R"rawliteral(...) syntax allows us to write a massive block of text (HTML) 
// without needing to put quotes around every single line or use \n for new lines.
String HTML = R"rawliteral(

<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0"> <!-- Makes the page scale correctly on phones -->
  <title>AirChat</title>
  <style>
    body { background: #000; color: #fff; font-family: sans-serif; text-align: center; padding: 10px; }
    h1 { color: #58a6ff; }
    #name-tag{font-size: 12px;;}
    input, textarea { width: 90%; padding: 11px; margin: 9px 0; background: #262626; color: #fff; border:2px solid #5c7789;}
    button { width: 50%; padding: 10px; background: #58a6ff; color: #000; font-weight: bold; border: 2px outset grey; margin-top: 10px; }
    #chat { text-align: left; width: 90%; margin: 20px auto; border-top: 1px solid #1e415d; padding-top: 10px; }
    p { margin: 8px 0; word-wrap: break-word; }
    b { color: #96d3e6; }
  </style>
</head>
<body>
  <h1>AirChat</h1>
  <p id = "name-tag">Made by Neelansh Shukla</p>
  <!-- HTML-->
  <input type="text" id="name" placeholder="Name"><br>
  <input type="text" id="msg" placeholder="Message"><br>
  <button onclick="send()">Send</button>

  <!-- This empty div is where the JavaScript will inject the messages -->
  <div id="chat"></div>

  <script>
    // JAVASCRIPT: Runs in the user's browser
    
    function send() {
      // 1. Get the values the user typed into the input boxes
      let n = document.getElementById("name").value;
      let m = document.getElementById("msg").value;
      
      // 2. If either box is empty, do nothing (return)
      if(!n || !m) return;
      
      // 3. Send the data to the ESP32's "/submit" route using a URL
      // encodeURIComponent ensures special characters (like & or spaces) don't break the URL
      fetch(`/submit?name=${encodeURIComponent(n)}&msg=${encodeURIComponent(m)}`)
        .then(() => {
          document.getElementById("msg").value = ""; // Clear the message input box
          load();                                     // Immediately reload the messages
        });
    }

    function load() {
      // 1. Ask the ESP32 for the list of messages
      fetch('/get_messages')
        .then(res => res.json()) // Convert the ESP32's response into a JavaScript JSON object
        .then(data => {
          let html = "";
          // 2. Loop through the messages backwards (newest at the top)
          for(let i = data.length - 1; i >= 0; i--) {
            // 3. Format the message into HTML paragraphs
            html += `<p><b>@${data[i].name}:</b> ${data[i].text}</p>`;
          }
          // 4. Insert the formatted HTML into the "chat" div
          document.getElementById("chat").innerHTML = html;
        });
    }

    // Run the load() function every 2000 milliseconds (2 seconds) to check for new messages
    setInterval(load, 2000);
    load(); // Run it once immediately when the page loads
  </script>
</body>
</html>
)rawliteral";

// --- BACKEND ROUTE HANDLERS ---

// 1. Runs when someone visits the main page (http://192.168.4.1/)
void handleRoot() 
{
  // Send HTTP status 200 (OK), tell the browser it's HTML, and send the HTML string
  server.send(200, "text/html", HTML); 
}

// 2. Runs when the JavaScript fetches "/submit"
void handleSubmit()
{
  // Check if the URL actually contains the "name" and "msg" parameters
  if (server.hasArg("name") && server.hasArg("msg"))
   {
    // Check if we haven't reached the 20 message limit
    if (messageCount < 20) 
    { 
      // Save the data into the array at the current messageCount index
      airchat[messageCount].name = server.arg("name");
      airchat[messageCount].text = server.arg("msg");
      // Increase the counter by 1 so the next message goes into the next slot
      messageCount++;
    }
    // Tell the browser everything went fine
    server.send(200, "text/plain", "OK");
  } 
  else 
  {
    // If the parameters were missing, send an error code
    server.send(400, "text/plain", "Bad Request");
  }
}

// 3. Runs when the JavaScript fetches "/get_messages"
void handleGetMessages() 
{
  String json = "["; // Start building a JSON array string
  
  // Loop through the array of messages we have saved
  for (int i = 0; i < messageCount; i++) 
  {
    // If it's not the first item, add a comma to separate the JSON objects
    if (i > 0) json += ",";
    
    // Manually build the JSON string, e.g., {"name":"John","text":"Hello"}
    json += "{\"name\":\"" + airchat[i].name + "\",\"text\":\"" + airchat[i].text + "\"}";
  }
  
  json += "]"; // Close the JSON array string
  
  // Send the JSON string to the browser
  server.send(200, "application/json", json);
}

// --- SETUP (Runs once when the ESP32 boots up) ---
void setup() 
{
  Serial.begin(115200); // Start the serial monitor at a fast speed for debugging
  
  // 1. Create the Wi-Fi Network
  IPAddress apIP(192, 168, 4, 1); // Set the IP address for the ESP32
  WiFi.mode(WIFI_AP);             // Set the Wi-Fi chip to Access Point mode (acts like a router)
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); // Apply the IP settings
  WiFi.softAP("AirChat");         // Create an open Wi-Fi network named "AirChat"

  // 2. Start the DNS Server for the Captive Portal
  // This intercepts ANY web request (the "*") on port 53 and redirects it to the ESP32's IP
  dnsServer.start(53, "*", apIP);

  // 3. Tell the Web Server which function to run for which URL
  server.on("/", handleRoot);                   // Main page
  server.on("/submit", handleSubmit);           // Receiving new messages
  server.on("/get_messages", handleGetMessages); // Sending messages to the browser

  // 4. Handle unknown requests (Captive Portal Redirect)
  server.onNotFound([]() {
    // If the user tries to go to google.com, send a 302 Redirect back to the ESP32's IP
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
  });

  server.begin(); // Start the web server
  Serial.println("AirChat Server Started! Connect to 'AirChat' WiFi.");
}

// --- LOOP (Runs forever) ---
void loop() 
{
  dnsServer.processNextRequest(); // Constantly check for incoming Wi-Fi connection attempts
  server.handleClient();          // Constantly check for incoming web page requests
}