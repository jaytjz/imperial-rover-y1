#include <WiFi.h>
#include <WebServer.h>

// Replace with your network credentials
const char* ssid = "JeremyTan";
const char* password = "plsconnect";

// Create an instance of the web server running on port 80
WebServer server(80);

const int en_left = 18;
const int en_right = 19;
const int dirleft = 32;
const int dirright = 33;

const int ultrasoundPin = 12;
const int radioPin = 25;
const int infraredPin = 27;
const int hallSensorPin = 34; 

// HTML content
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Team Basilisk</title>
    <meta name="viewport" content="user-scalable=no">
    <style>
        body {
            font-family: 'Gill Sans', 'Gill Sans MT', Calibri, 'Trebuchet MS', sans-serif;
            color: rgb(128, 128, 128);
            font-size: xx-large;
            display: flex;
            justify-content: space-between;
            align-items: center;
            height: 100vh;
            margin: 0;
        }
        #left {
            width: 40%;
            text-align: center;
            margin-left: 5%;
        }
        #right {
            width: 55%;
            text-align: center;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin-left: auto;
            margin-right: auto;
        }
        th, td {
            border: 1px solid black;
            padding: 8px;
            text-align: center;
        }
        h1 {
            text-align: center;
        }
        .curved-button {
            background-color: rgb(128, 128, 128);
            color: white;
            padding: 15px 30px;
            border: none;
            border-radius: 25px;
            font-size: 16px;
            cursor: pointer;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
            transition: background-color 0.3s, box-shadow 0.3s;
        }
        .curved-button:hover {
            background-color: rgb(85, 85, 85);
            box-shadow: 0 6px 12px rgba(0, 0, 0, 0.2);
        }
    </style>
</head>
<body>
    <div id="left">
        <h1>Basilisk</h1>
        <table id="data-table">
            <tr>
                <th>Name</th>
                <th>Infrared</th>
                <th>Radio</th>
                <th>Magnetic</th>
            </tr>
        </table>
        <p>
            <button class="curved-button" id="upload-button">Upload</button>
            <button class="curved-button" id="delete-button">Delete</button>
        </p>
    </div>
    <div id="right">
        <p>
            X: <span id="x_coordinate">0</span>
            Y: <span id="y_coordinate">0</span>
            Angle: <span id="angle">0</span>&deg
        </p>
        <p>
            Motor State: <span id="motor_state"> Off</span>
            <span style="margin-left: 20px;">Speed: <span id="speed">0</span></span>
        </p>
        <canvas id="canvas" name="game"></canvas>
        <p>
            Name: <span id="ultrasound"> - </span>
            Infrared: <span id="infrared_wave"> - </span>
            Radio: <span id="radio_wave"> - </span>
            Magnetic: <span id="magnetic_field"> - </span>
        </p>
        <p>
            <button class="curved-button" id="ultrasound-button">Ultrasound</button>
            <button class="curved-button" id="infrared-button">Infrared</button>
            <button class="curved-button" id="radio-button">Radio</button>
            <button class="curved-button" id="magnet-button">Magnetic</button>
        </p>
    </div>
    <script>
    
	    var canvas, ctx, width, height, radius, x_orig, y_orig;
        let coord = { x: 0, y: 0 };
        let paint = false;
        let lastCommand = { action: null, speed: null };
	    
        window.addEventListener('load', () => {
            canvas = document.getElementById('canvas');
            ctx = canvas.getContext('2d');
            resize();

            document.addEventListener('mousedown', startDrawing);
            document.addEventListener('mouseup', stopDrawing);
            document.addEventListener('mousemove', draw);
            document.addEventListener('touchstart', startDrawing);
            document.addEventListener('touchend', stopDrawing);
            document.addEventListener('touchcancel', stopDrawing);
            document.addEventListener('touchmove', draw);
            document.addEventListener('keydown', handleKeyDown);
            document.addEventListener('keyup', handleKeyUp);

            setInterval(() => {
                let currentAction = document.getElementById("motor_state").innerText.toLowerCase();
                let currentSpeed = document.getElementById("speed").innerText;
                if (lastCommand.action !== currentAction || lastCommand.speed !== currentSpeed) {
                    sendMotorCommand(currentAction, currentSpeed);
                    lastCommand = { action: currentAction, speed: currentSpeed };
                }
            }, 500);

            document.getElementById('upload-button').addEventListener('click', uploadData);
            document.getElementById('delete-button').addEventListener('click', deleteData);
            document.getElementById('ultrasound-button').addEventListener('click', ultrasoundData);
            document.getElementById('infrared-button').addEventListener('click', infraredData);
            document.getElementById('radio-button').addEventListener('click', radioData);
            document.getElementById('magnet-button').addEventListener('click', magnetData);
        });

        function resize() {
            width = document.getElementById('right').offsetWidth;
            radius = 100;
            height = radius * 5;
            ctx.canvas.width = width;
            ctx.canvas.height = height;
            background();
            joystick(width / 2, height / 2);
        }

        function background() {
            x_orig = width / 2;
            y_orig = height / 2;

            ctx.beginPath();
            ctx.arc(x_orig, y_orig, radius + 20, 0, Math.PI * 2, true);
            ctx.fillStyle = '#ECE5E5';
            ctx.fill();
        }

        function joystick(width, height) {
            ctx.beginPath();
            ctx.arc(width, height, radius, 0, Math.PI * 2, true);
            ctx.fillStyle = '#F08080';
            ctx.fill();
            ctx.strokeStyle = '#F6ABAB';
            ctx.lineWidth = 8;
            ctx.stroke();
        }

        function getPosition(event) {
            var rect = canvas.getBoundingClientRect();
            var mouse_x = event.clientX || event.touches[0].clientX;
            var mouse_y = event.clientY || event.touches[0].clientY;
            coord.x = mouse_x - rect.left;
            coord.y = mouse_y - rect.top;
        }

        function isInCircle() {
            var current_radius = Math.sqrt(Math.pow(coord.x - x_orig, 2) + Math.pow(coord.y - y_orig, 2));
            return radius >= current_radius;
        }

        function startDrawing(event) {
            paint = true;
            getPosition(event);
            if (isInCircle()) {
                updateCanvas();
                draw(event);
            }
        }

        function stopDrawing() {
            paint = false;
            resetCanvas();
            updateDisplay(0, 0, 0, 0);
            updateMotorState(0);
        }

        function draw(event) {
            if (paint) {
                getPosition(event);
                updateCanvas();
                var angle = Math.atan2((coord.y - y_orig), (coord.x - x_orig));
                var angle_in_degrees = (Math.sign(angle) == -1) ? Math.round(-angle * 180 / Math.PI) : Math.round(360 - angle * 180 / Math.PI);
                var x = isInCircle() ? coord.x : radius * Math.cos(angle) + x_orig;
                var y = isInCircle() ? coord.y : radius * Math.sin(angle) + y_orig;
                joystick(x, y);

                var x_relative = Math.round(x - x_orig);
                var y_relative = Math.round(y_orig - y);
                var current_radius = Math.sqrt(Math.pow(coord.x - x_orig, 2) + Math.pow(coord.y - y_orig, 2));
                var speed = Math.min(Math.round((current_radius / radius) * 100), 100);

                updateDisplay(x_relative, y_relative, angle_in_degrees, speed);
                updateMotorState(angle_in_degrees);
            }
        }

        function handleKeyDown(event) {
            let keyMap = {
                "ArrowUp": [0, 70, "forward", 70],
                "ArrowDown": [0, -70, "reverse", 70],
                "ArrowLeft": [-70, 0, "left", 70],
                "ArrowRight": [70, 0, "right", 70],
                " ": [0, 0, "off", 0]
            };
            if (keyMap[event.key]) {
                let [x, y, action, speed] = keyMap[event.key];
                coord.x = x + x_orig;
                coord.y = y_orig - y;
                updateJoystickAndMotor();
                if (lastCommand.action !== action || lastCommand.speed !== speed) {
                    sendMotorCommand(action, speed);
                    lastCommand = { action, speed };
                }
                document.getElementById("speed").innerText = speed;
            }
        }

        function handleKeyUp() {
            resetCanvas();
            updateDisplay(0, 0, 0, 0);
            updateMotorState(0);
            if (lastCommand.action !== "off" || lastCommand.speed !== 0) {
                sendMotorCommand("off", 0);
                lastCommand = { action: "off", speed: 0 };
            }
        }

        function updateJoystickAndMotor() {
            updateCanvas();
            let angle = Math.atan2((coord.y - y_orig), (coord.x - x_orig));
            joystick(coord.x, coord.y);
            let angle_in_degrees = (angle >= 0 ? 360 : 0) - Math.round(angle * 180 / Math.PI);
            updateDisplay(Math.round(coord.x - x_orig), Math.round(y_orig - coord.y), angle_in_degrees);
            updateMotorState(angle_in_degrees);
        }

        function updateCanvas() {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            background();
        }

        function resetCanvas() {
            updateCanvas();
            joystick(width / 2, height / 2);
        }

        function updateDisplay(x, y, angle, speed = 0) {
            document.getElementById("x_coordinate").innerText = x;
            document.getElementById("y_coordinate").innerText = y;
            document.getElementById("angle").innerText = angle;
            document.getElementById("speed").innerText = speed;
        }

        function updateMotorState(angle) {
            let motorState;
            if (angle == 0) {
                motorState = "Off";
            } else if (angle > 0 && angle <= 45) {
                motorState = "Right";
            } else if(angle > 315 && angle <= 360){
                motorState = "Right";
            } else if (angle > 45 && angle <= 135) {
                motorState = "Forward";
            } else if (angle > 215 && angle <= 315) {
                motorState = "Reverse";
            } else {
                motorState = "Left";
            }
            document.getElementById("motor_state").innerText = motorState;
        }

        function sendMotorState(state) {
            fetch(`/${state.toLowerCase()}`)
                .then(response => response.text())
                .then(data => console.log(data))
                .catch(error => console.error('Error:', error));
        }

        function sendMotorCommand(action, speed) {
            fetch(`/${action}?speed=${speed}`)
                .then(response => response.text())
                .then(data => console.log(data))
                .catch(error => console.error('Error:', error));
        }
        
        function uploadData(){
            let table = document.getElementById('data-table');
            let row = table.insertRow();
            let cell1 = row.insertCell(0);
            let cell2 = row.insertCell(1);
            let cell3 = row.insertCell(2);
            let cell4 = row.insertCell(3);
            cell1.innerHTML = document.getElementById("ultrasound").innerText;
            cell2.innerHTML = document.getElementById("infrared_wave").innerText;
            cell3.innerHTML = document.getElementById("radio_wave").innerText;
            cell4.innerHTML = document.getElementById("magnetic_field").innerText;
            document.getElementById("ultrasound").innerText = "-";
            document.getElementById("infrared_wave").innerText = "-";
            document.getElementById("radio_wave").innerText = "-";
            document.getElementById("magnetic_field").innerText = "-";
        }
        
        function deleteData() {
            let table = document.getElementById('data-table');
            if (table.rows.length > 1) table.deleteRow(table.rows.length - 1);
        }
        
        function ultrasoundData(){
		        fetch('/ultrasound').then(response => response.text()).then(data => {
                document.getElementById('ultrasound').textContent = data;
            });
        }
        
        function infraredData(){
            fetch('/infrared').then(response => response.text()).then(data => {
                document.getElementById('infrared_wave').textContent = data;
            });
        }

        function radioData() {
            fetch('/radio').then(response => response.text()).then(data => {
                document.getElementById('radio_wave').textContent = data;
            });          
        }
        
         function magnetData() {
            fetch('/magnetic').then(response => response.text()).then(data => {
                document.getElementById('magnetic_field').textContent = data;
            });            
        }   
    </script>
</body>
</html>
)rawliteral";

// Function to handle root path
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void moveMotors(int leftSpeed, int rightSpeed, int leftDir, int rightDir) {
    analogWrite(en_left, leftSpeed);
    analogWrite(en_right, rightSpeed);
    analogWrite(dirleft, leftDir);
    analogWrite(dirright, rightDir);
}

void forward() {
    int speed = server.arg("speed").toInt();
    int pwmValue = map(speed, 0, 100, 0, 255);
    moveMotors(pwmValue, pwmValue, 1000, 1000);
    server.send(200, F("text/plain"), F("forward"));
}

void reverse() {
    int speed = server.arg("speed").toInt();
    int pwmValue = map(speed, 0, 100, 0, 255);
    moveMotors(pwmValue, pwmValue, 0, 0);
    server.send(200, F("text/plain"), F("reverse"));
}

void left() {
    int speed = server.arg("speed").toInt();
    int pwmValue = map(speed, 0, 100, 0, 255);
    moveMotors(0, pwmValue, 1000, 1000);
    server.send(200, F("text/plain"), F("left"));
}

void right() {
    int speed = server.arg("speed").toInt();
    int pwmValue = map(speed, 0, 100, 0, 255);
    moveMotors(pwmValue, 0, 1000, 1000);
    server.send(200, F("text/plain"), F("right"));
}

void off() {
    moveMotors(0, 0, 0, 0);
    server.send(200, F("text/plain"), F("off"));
}

// Function to handle ultrasound data
void handleUltrasound() {
	if (Serial1.available()) {
    String name_received = Serial1.readStringUntil('#');
    Serial.print("Name received: ");
    Serial.println(name_received);
    server.send(200, "text/plain", name_received);
  }
  else{
    server.send(200, "text/plain", "error");
  }
}

void handleInfrared() {
  // Variables to store the number of pulses and the frequency
  unsigned long pulseCount = 0;
  unsigned long previousMillis = 0;
  unsigned long interval = 1000; // Interval for measuring frequency (in milliseconds)
  float frequency = 0.0;
  unsigned long duration = 500;

  // Variables to store the state of the input pin
  int lastState = LOW;
  int currentState;
  // Get the current state of the input pin
  unsigned long startMillis = millis();
  pulseCount = 0; // Reset pulse count at the start

  while (millis() - startMillis < duration) {
    // Get the current state of the input pin
    currentState = digitalRead(infraredPin);

    // Check for a rising edge
    if (currentState == HIGH && lastState == LOW) {
      pulseCount++;
    }

    // Update the last state
    lastState = currentState;
  }

  // Calculate the frequency
  frequency = pulseCount * (1000.0 / duration);
  
  // Send the frequency as a response
  Serial.println("Infrared Frequency: " + String(frequency));
  server.send(200, "text/plain", String(frequency));
}

// Function to handle radio frequency data
void handleRadio() {
  // Variables to store the number of pulses and the frequency
  unsigned long pulseCount = 0;
  unsigned long previousMillis = 0;
  unsigned long interval = 1000; // Interval for measuring frequency (in milliseconds)
  float frequency = 0.0;
  unsigned long duration = 500;

  // Variables to store the state of the input pin
  int lastState = LOW;
  int currentState;
  // Get the current state of the input pin
  unsigned long startMillis = millis();
  pulseCount = 0; // Reset pulse count at the start

  while (millis() - startMillis < duration) {
    // Get the current state of the input pin
    currentState = digitalRead(radioPin);

    // Check for a rising edge
    if (currentState == HIGH && lastState == LOW) {
      pulseCount++;
    }

    // Update the last state
    lastState = currentState;
  }

  // Calculate the frequency
  frequency = pulseCount * (1000.0 / duration);
  
  // Send the frequency as a response
  Serial.println("Radio Frequency: " + String(frequency));
  server.send(200, "text/plain", String(frequency));
}

// Function to handle magnetic field data
void handleMagnetic() {
  const float centerVoltage = 2.02; // Center voltage
  const float tolerance = 0.05; // Tolerance range for the center voltage
  // Read the analog voltage from the Hall effect sensor
  int sensorValue = analogRead(hallSensorPin);
  // Convert the analog reading (0-1023) to a voltage (0-5V)
  float voltage = sensorValue * (3.3 / 4095.0);

  Serial.print("Magnet Voltage: ");
  Serial.println(voltage, 2); // Print the voltage with 2 decimal places

  String magneticField;

  // Determine the magnetic field direction
  if (voltage < (centerVoltage - tolerance)) {
    magneticField = "N";
  } else if (voltage > (centerVoltage + tolerance)) {
    magneticField = "S";
  } else {
    magneticField = "-";
  }

  Serial.println(magneticField);
  server.send(200, "text/plain", magneticField);
}

void setup() {
  // Start serial communication for debugging purposes
  Serial.begin(9600);

  Serial1.begin(600, SERIAL_8N1, ultrasoundPin, 13); // Initialize UART:RX pin 12, TX pin 13, baud rate 600

  // Set the input pin mode
  pinMode(radioPin, INPUT);
  pinMode(infraredPin, INPUT);
  pinMode(hallSensorPin,INPUT);

  // Connect to Wi-Fi network
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }
  Serial.println("Connected to the WiFi network");

  // Print the ESP32 IP address
  Serial.println(WiFi.localIP());

  // Define the root path and other paths
  server.on("/", handleRoot);
  server.on(F("/forward"), forward);
  server.on(F("/reverse"), reverse);
  server.on(F("/right"), right);
  server.on(F("/left"), left);
  server.on(F("/off"), off);
  server.on(F("/ultrasound"), handleUltrasound);
  server.on(F("/infrared"), handleInfrared);
  server.on(F("/radio"), handleRadio);
  server.on(F("/magnetic"), handleMagnetic);

  // Start the server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Handle client requests
  server.handleClient();
}