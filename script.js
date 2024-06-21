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
