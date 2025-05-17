const button1 = document.getElementById("algo1");
const button2 = document.getElementById("algo2");

const holdTime = 500;
let timer1, wasHeldEnough1, timer2, wasHeldEnough2;

button1.addEventListener('mousedown', handleHold);
button1.addEventListener('mouseup', handleRelease);
button1.addEventListener('touchstart', handleHold);
button1.addEventListener('touchend', handleRelease);

button2.addEventListener('mousedown', handleStart);
button2.addEventListener('mouseup', handleStop);
button2.addEventListener('touchstart', handleStart);
button2.addEventListener('touchend', handleStop);

const ESP32_IP = "http://192.168.4.2";


function handleHold() {
  wasHeldEnough1 = false;
  timer1 = setTimeout(() => {
    wasHeldEnough1 = true;
    fetch(`${ESP32_IP}/hold`);
  }, holdTime);
}

function handleRelease() {
  if (wasHeldEnough1) {
    fetch(`${ESP32_IP}/release`);
  }
  clearTimeout(timer1);
  wasHeldEnough1 = false;
}

function handleStart() {
  wasHeldEnough2 = false;
  timer2 = setTimeout(() => {
    wasHeldEnough2 = true;
    fetch(`${ESP32_IP}/start`);
  }, holdTime);
}

function handleStop() {
  if (wasHeldEnough2) {
    fetch(`${ESP32_IP}/stop`);
  }
  clearTimeout(timer2);
  wasHeldEnough2 = false;
}

const socket = new WebSocket("ws://192.168.4.2/ws");

socket.onmessage = (event) => {
  let data = event.data;
  // Clear previous active LED classes.
  document.querySelectorAll('.led').forEach(led => led.classList.remove('active'));

  // Check if we received a held-mode message.
  if (data.startsWith("held:")) {
    // Remove the prefix and split the two colors.
    let colors = data.substring(5).split(",");
    colors.forEach(color => {
      let ledEl = document.getElementById(color);
      if (ledEl) {
        ledEl.classList.add("active");
      }
    });
  } else {
    // Otherwise, work as normal (activate a single LED).
    if (data === "red") document.getElementById("red").classList.add("active");
    if (data === "yellow") document.getElementById("yellow").classList.add("active");
    if (data === "green") document.getElementById("green").classList.add("active");
  }
}
