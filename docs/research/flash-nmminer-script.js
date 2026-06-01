let espStub;

const baudRates = [921600, 115200, 230400, 460800];

const bufferSize = 512;
const colors = ["#00a7e9", "#f89521", "#be1e2d"];
const measurementPeriodId = "0001";

const maxLogLength = 100;
const log = document.getElementById("log");
const butConnect = document.getElementById("butConnect");
const baudRate = document.getElementById("baudRate");
const butClear = document.getElementById("butClear");
// const butErase = document.getElementById("butErase");
// const butProgram = document.getElementById("butProgram");
const autoscroll = document.getElementById("autoscroll");
const lightSS = document.getElementById("light");
const darkSS = document.getElementById("dark");
const darkMode = document.getElementById("darkmode");
const firmware = document.querySelectorAll(".upload .firmware input");
const progress = document.querySelectorAll(".upload .progress-bar");
const offsets = document.querySelectorAll(".upload .offset");
const appDiv = document.getElementById("app");
const device = document.getElementById("devices");

const version = document.getElementById("version");

const devcode = document.getElementById("devcode");
const actcode = document.getElementById("actcode");
const butAct = document.getElementById("butAct");
const butGetCode = document.getElementById("butGetCode");
const actDiv = document.getElementById("act");
// WiFi Config
const configDiv = document.getElementById("wificonfig")
const wifi_ssid = document.getElementById("ssid")
const wifi_pwd = document.getElementById("password")
const btc_addr = document.getElementById("address")
const btnConfig = document.getElementById("btnConfig")
const btnClearInfo = document.getElementById("btnClearInfo")

const primary_pool_url = document.getElementById("primary_pool_url")
const primary_pool_password = document.getElementById("primary_pool_password")
const fallback_pool_url = document.getElementById("fallback_pool_url")
const fallback_pool_password = document.getElementById("fallback_pool_password")
const fallback_btc_addr = document.getElementById("address2")

// Console Only module
const consoleonly = document.getElementById("consoleonly");

const span = document.getElementById("span3rd");

const productDiv = document.getElementById("product");

let urls = ["bootloader.bin", "partitions.bin", "boot_app0.bin", "firmware.bin", "littlefs.bin"];

let flashUsage = {
  uid: '',
  license: '',
  dtype: '',
  dversion: '',
  toactive: 0, 
  flashed: 0
};

let flashToolUsed = false;

let bQuery = false;
let queryLicense = "failed";

document.addEventListener("DOMContentLoaded", () => {
  butConnect.addEventListener("click", () => {
    clickConnect().catch(async (e) => {
      console.error(e);
      errorMsg(e.message || e);
      if (espStub) {
        await espStub.disconnect();
      }
      toggleUIConnected(false);
    });
  });

  btnClearInfo.addEventListener("click", clickClearInfo);
  btnConfig.addEventListener("click", clickConfig);
  butGetCode.addEventListener("click", clickGetActCode);
  butAct.addEventListener("click", clickActivation);
  butClear.addEventListener("click", clickClear);
  // butErase.addEventListener("click", clickErase);
  // butProgram.addEventListener("click", clickProgram);
  for (let i = 0; i < firmware.length; i++) {
    firmware[i].addEventListener("change", checkFirmware);
  }
  for (let i = 0; i < offsets.length; i++) {
    offsets[i].addEventListener("change", checkProgrammable);
  }
  autoscroll.addEventListener("click", clickAutoscroll);
  baudRate.addEventListener("change", changeBaudRate);
  device.addEventListener("change", changeDevice);
  darkMode.addEventListener("click", clickDarkMode);
  consoleonly.addEventListener("click", clickConsoleMode);

  version.addEventListener("change", changeVersion);

  window.addEventListener("error", function (event) {
    console.log("Got an uncaught error: ", event.error);
  });
  if ("serial" in navigator) {
    const notSupported = document.getElementById("notSupported");
    notSupported.classList.add("hidden");
  }
  // baudRate.style.display = "none";
  
  initBaudRate();
  loadAllSettings();
  updateTheme();
  logMsg("ESP Web Flasher loaded.");
});

// version map
const vmap = new Map();

let allDevices = [];

window.onload = async function () {
  const vresponse = await fetch('/firmware/devices_version.json');
  const jsonObject = await vresponse.json();

  jsonObject.versions.forEach(version => {
    vmap.set(version.version, version.devices);
  });

  // get alldevices
  jsonObject.versions.forEach(version => {
    version.devices.forEach(dev => {
      if(!allDevices.includes(dev.device)){
        allDevices.push(dev.device);
      }
    });
  });

  // clear and add device
  while (device.firstChild) {
      device.removeChild(device.firstChild);
  }
  
  for(const dev of allDevices) {
      const option = document.createElement('option');
      option.value = dev;
      option.textContent = dev;
      device.appendChild(option);
  }
  device.value = loadSetting("device", "heltec-vision-master-t190");

  // clear and add version branch
  while(version.firstChild)
  {
    version.removeChild(version.firstChild);
  }

  for(const [key, value] of vmap)
  {
    value.forEach(dev => {
      if(dev.device === device.value)
      {
        const option = document.createElement('option');
        option.value = key;
        option.textContent = key;
        version.appendChild(option);
      }
    });
  }

  version.value = loadSetting("version", "v0.4.03");

  // for (const [key, value] of vmap)
  // {
  //   const option = document.createElement('option');
  //   option.value = key;
  //   option.textContent = key;
  //   version.appendChild(option);
  // }

  // version.value = loadSetting("version", "v0.4.02");

  // clear and add device
  // while (device.firstChild) {
  //   device.removeChild(device.firstChild);
  // }

  // for (const dev of vmap.get(version.value)) {
  //   const option = document.createElement('option');
  //   option.value = dev.device;
  //   option.textContent = dev.device;
  //   device.appendChild(option);
  // }
  // device.value = loadSetting("device", "heltec-vision-master-t190");
};

function initBaudRate() {
  for (let rate of baudRates) {
    var option = document.createElement("option");
    option.text = rate + " Baud";
    option.value = rate;
    baudRate.add(option);
  }
}

function logMsg(text) {

  const colorMap = {
    "\033[0m": "black",
    "\033[31m": "red",
    "\033[32m": "green",
    "\033[33m": "yellow",
    "\033[34m": "blue",
    "\033[35m": "magenta",
    "\033[36m": "cyan",
    "\033[37m": "white"
  };

  // log.innerHTML += text + "<br>";

  let htmlText = text.replace(/\033\[\d+m/g, (match) => {
    return `</span><span style="color: ${colorMap[match] || 'black'};">`;
  });

  // Ensure the text starts with a span
  htmlText = `<span>${htmlText}</span>`;

  // log.innerHTML += `<pre style="font-family: monospace;">${htmlText}</pre>`;
  // log.innerHTML += htmlText + "<br>";
  log.innerHTML += `<div class="log-entry">${htmlText}</div>`;

  // Remove old log content
  if (log.textContent.split("\n").length > maxLogLength + 1) {
    let logLines = log.innerHTML.replace(/(\n)/gm, "").split("<br>");
    log.innerHTML = logLines.splice(-maxLogLength).join("<br>\n");
  }

  if (autoscroll.checked) {
    log.scrollTop = log.scrollHeight;
  }
}

function debugMsg(...args) {
  function getStackTrace() {
    let stack = new Error().stack;
    //console.log(stack);
    stack = stack.split("\n").map((v) => v.trim());
    stack.shift();
    stack.shift();

    let trace = [];
    for (let line of stack) {
      line = line.replace("at ", "");
      trace.push({
        func: line.substr(0, line.indexOf("(") - 1),
        pos: line.substring(line.indexOf(".js:") + 4, line.lastIndexOf(":")),
      });
    }

    return trace;
  }

  let stack = getStackTrace();
  stack.shift();
  let top = stack.shift();
  let prefix =
    '<span class="debug-function">[' + top.func + ":" + top.pos + "]</span> ";
  for (let arg of args) {
    if (arg === undefined) {
      logMsg(prefix + "undefined");
    } else if (arg === null) {
      logMsg(prefix + "null");
    } else if (typeof arg == "string") {
      logMsg(prefix + arg);
    } else if (typeof arg == "number") {
      logMsg(prefix + arg);
    } else if (typeof arg == "boolean") {
      logMsg(prefix + (arg ? "true" : "false"));
    } else if (Array.isArray(arg)) {
      logMsg(prefix + "[" + arg.map((value) => toHex(value)).join(", ") + "]");
    } else if (typeof arg == "object" && arg instanceof Uint8Array) {
      logMsg(
        prefix +
        "[" +
        Array.from(arg)
          .map((value) => toHex(value))
          .join(", ") +
        "]"
      );
    } else {
      logMsg(prefix + "Unhandled type of argument:" + typeof arg);
      console.log(arg);
    }
    prefix = ""; // Only show for first argument
  }
}

function errorMsg(text) {
  logMsg('<span class="error-message">Error:</span> ' + text);
  console.error(text);
}

/**
 * @name updateTheme
 * Sets the theme to  Adafruit (dark) mode. Can be refactored later for more themes
 */
function updateTheme() {
  // Disable all themes
  document
    .querySelectorAll("link[rel=stylesheet].alternate")
    .forEach((styleSheet) => {
      enableStyleSheet(styleSheet, false);
    });

  if (darkMode.checked) {
    enableStyleSheet(darkSS, true);
  } else {
    enableStyleSheet(lightSS, true);
  }
}

function enableStyleSheet(node, enabled) {
  node.disabled = !enabled;
}

function formatMacAddr(macAddr) {
  return macAddr
    .map((value) => value.toString(16).toUpperCase().padStart(2, "0"))
    .join(":");
}

/**
 * @name clickConnect
 * Click handler for the connect/disconnect button.
 */
async function clickConnect() {

  const readRemoteFileAsArrayBuffer = async (url) => {
    const response = await fetch(url);
    if (!response.ok) {
      throw new Error("Failed to fetch remote file: " + url);
    }
    const arrayBuffer = await response.arrayBuffer();
    let size = arrayBuffer.byteLength;
    logMsg("load file:" + url + " ok, length: " + size);
    return arrayBuffer;
  };

  if (espStub) {
    await espStub.disconnect();
    await espStub.port.close();
    toggleUIConnected(false);
    espStub = undefined;
    return;
  }

  const esploaderMod = await window.esptoolPackage;

  const esploader = await esploaderMod.connect({
    log: (...args) => logMsg(...args),
    debug: (...args) => debugMsg(...args),
    error: (...args) => errorMsg(...args),
  });

  // esploader.addEventListener('disconnect', function (event) {
  //   errorMsg("Device disconnected, reload page 3 seconds later");
  //   if(flashToolUsed)
  //   {
  //     storeRecord(flashUsage);
  //   }
  //   setTimeout(function () {
  //     location.reload();
  //   }, 3000);
  // });

  let baud = parseInt(baudRate.value);

  try {
    await esploader.initialize();

    // flash tool usage info
    flashToolUsed = true;
    flashUsage.dtype = device.value;
    bQuery = false;

    logMsg("Connected to " + esploader.chipName);
    logMsg("MAC Address: " + formatMacAddr(esploader.macAddr()));

    espStub = await esploader.runStub();
    if((otherBootDevice.includes(device.value) || esp32c5BootDevice.includes(device.value)) && (baud != 115200))
    {
      await espStub.setBaudrate(baud);
    }

    // await espStub.setBaudrate(921600);
    toggleUIConnected(true);

    // espStub.addEventListener("disconnect", () => {
    //   toggleUIConnected(false);
    //   espStub = false;
    // });

    if (consoleonly.checked) {
      logMsg("Reset Device, Enter Console Only mode!");
    }
    else {
      productDiv.style.display = "none";
      toggleUIToolbar(true);

      for (let i = 0; i < 4; i++) {
        let contents = await readRemoteFileAsArrayBuffer(urls[i]);
        try {
          let offset = parseInt(offsets[i].value, 16);
          progress[i].classList.remove("hidden");
          const progressBar = progress[i].querySelector("div");

          // write bootloader
          if(offsets[i].value === "f90000")
          {
            let axe_bootloader_url = "/firmware/" + version.value + "/" + device.value + "/bootloader.bin";
            let axe_content = await readRemoteFileAsArrayBuffer(axe_bootloader_url);
            await espStub.flashData(
              axe_content,
              (bytesWritten, totalBytes) => {
                progressBar.style.width =
                  Math.floor((bytesWritten / totalBytes) * 100) + "%";
              },
              0
            );
            await sleep(10);
          }

          if(offsets[i].value === "390000")
          {
            let new_ui = await readRemoteFileAsArrayBuffer(urls[4]);
            let ui_offset = parseInt("10000", 16);
            await espStub.flashData(
              new_ui,
              (bytesWritten, totalBytes) => {
                progressBar.style.width =
                  Math.floor((bytesWritten / totalBytes) * 100) + "%";
              },
              ui_offset,
              true
            );
            await sleep(10);
          }

          let compress = false;
          if(otherBootDevice.includes(device.value) || esp32c5BootDevice.includes(device.value))
          {
            compress = true;
          }

          await espStub.flashData(
            contents,
            (bytesWritten, totalBytes) => {
              progressBar.style.width =
                Math.floor((bytesWritten / totalBytes) * 100) + "%";
            },
            offset,
            compress
          );
          await sleep(100);
        } catch (e) {
          errorMsg(e);
        }
      }
      flashUsage.flashed = 1;
    }

  } catch (err) {
    await esploader.disconnect();
    throw err;
  }

  await sleep(1000);
  toggleUIToolbar(false);
  productDiv.style.display = "block";
  configDiv.style.display = "block";
  // reset the device
  if(baud != 115200)
  {
    await espStub.setBaudrate(115200);
  }
  await esploader.hardReset(false);
  butAct.disabled = false;
  butGetCode.disabled = false;

  esploader.addEventListener('disconnect', function (event) {
    errorMsg("Device disconnected, reload page 3 seconds later");
    if(flashToolUsed)
    {
      storeRecord(flashUsage);
    }
    setTimeout(function () {
      location.reload();
    }, 3000);
  });

  // Axe framework
  const axeDevice = [
    'NMAxe-v1x',
    'NMAxe-Gamma',
    'NMQAxe++'
  ];

  const newLicenseVersion = [
    'v1.7.01',
    'v1.7.02',
    'v1.7.03',
    'v1.7.04',
    'v1.7.05',
    'v1.7.06',
    'v1.8.10',
    'v1.8.20',
    'v1.8.22',
    'v1.8.23',
    'v1.8.24',
    'v1.8.25',
    'v1.8.26',
    'v1.8.27',
    'v1.8.28',
    'v2.0.01',
    'v2.0.02'
  ];
  
  actDiv.style.display = "none";

  // if(axeDevice.includes(device.value))
  // {
  //   actDiv.style.display = "none";
  // }
  // else
  // {
  //   actDiv.style.display = "block";
  // }

  await sleep(1000);
  // Console
  while (true) {
    try {
      let line = await esploader.readLine(100);
      // Remove ANSI escape sequences
      data = line.replace(/\x1B[[(?);]{0,2}(;?\d)*./g, '');
      logMsg(line);

      // Check if the line contains "Licence not found in nvs"
      if (data.includes('Licence not found in nvs') || data.includes('License verification failed')) {
        const match = data.match(/\[([^\]]+)\]/);
        if (match) {
          devcode.value = match[1];
          flashUsage.uid = match[1];
          if(!bQuery)
          {
            bQuery = true;
            queryLicense = await getLicense(match[1]);
          }
          if(queryLicense!='failed')
          {
            logMsg("try to active device, license:" + queryLicense);
            // new version check
            if(newLicenseVersion.includes(version.value))
            // if(version.value === "v1.7.01" || version.value === "v1.7.02" || version.value === "v1.7.03")
            {
              queryLicense = await getNewLicense(match[1]);
              logMsg("try to active device, new license:" + queryLicense);
              flashUsage.license = queryLicense;
              await espStub.writeToString(queryLicense);
            }
            else
            {
              flashUsage.license = queryLicense;
              await espStub.writeToString(queryLicense);
            }
          }
          else
          {
            actDiv.style.display = "block";
          }
          // license = await getLicense(match[1]);
          // if(license!='failed')
          // {
          //   logMsg("device license code: " + license);
          //   flashUsage.license = license;
          //   await espStub.writeToString(license);
          // }
        }
      }

      const extractInfo = (msg) => {
        const match = msg.match(/\[([^\]]+)\]/);
        return match ? match[1] : null;
      };

      if(data.includes('NMMiner version'))
      {
        const parts = data.split('NMMiner version');
        const match = extractInfo(parts[parts.length -1]);
        if (match) {
          flashUsage.dversion = match;
        }
      }

      if(data.includes('NMMiner Device code'))
      {
        const parts = data.split('NMMiner Device code');
        const match = extractInfo(parts[parts.length -1]);
        if (match) {
          flashUsage.uid = match;
        }
      }

      if(data.includes('NMMiner Device licence'))
      {
        const parts = data.split('NMMiner Device licence');
        const match = extractInfo(parts[parts.length -1]);
        if (match) {
          flashUsage.license = match;
        }
      }

      if (data.includes('License verification passed')) {
        actDiv.style.display = "none";
      }

      if (data.includes('Force wifi configuration triggered')) {
        configDiv.style.display = "block"
      }

      if (data.includes('WiFi connect timeout')) {
        configDiv.style.display = "block"
      }

      // if (data.includes('Connected to access point')) {
      //   configDiv.style.display = "none";
      // }

      if(data.includes('share accepted,'))
      {
        configDiv.style.display = "none";
      }

    }
    catch (e) {
      if (e.message === "Read line timed out") {
        // If the error is a timeout error, ignore it and continue the loop
        continue;
      } else {
        // If it's another kind of error, log it and break the loop
        errorMsg(e);
        break;
      }
    }
  }

}

async function changeDevice() {
  saveSetting("device", device.value);
  // clear and add version branch
  while(version.firstChild)
  {
    version.removeChild(version.firstChild);
  }

  for(const [key, value] of vmap)
  {
    value.forEach(dev => {
      if(dev.device === device.value)
      {
        const option = document.createElement('option');
        option.value = key;
        option.textContent = key;
        version.appendChild(option);
      }
    });
  }
  saveSetting("version", version.value)
}

async function changeVersion() {

  saveSetting("version", version.value);

  // // clear and load the devices supported by current version
  // while(device.firstChild)
  // {
  //   device.removeChild(device.firstChild);
  // }

  // for(const dev of vmap.get(version.value))
  // {
  //   const option = document.createElement('option');
  //   option.value = dev.device;
  //   option.textContent = dev.device;
  //   device.appendChild(option);
  // }
  // saveSetting("device", device.value);
}

/**
 * @name changeBaudRate
 * Change handler for the Baud Rate selector.
 */
async function changeBaudRate() {
  saveSetting("baudrate", baudRate.value);
  if (espStub) {
    let baud = parseInt(baudRate.value);
    if (baudRates.includes(baud)) {
      // await espStub.setBaudrate(baud);
    }
  }
}

/**
 * @name clickAutoscroll
 * Change handler for the Autoscroll checkbox.
 */
async function clickAutoscroll() {
  saveSetting("autoscroll", autoscroll.checked);
}

/**
 * @name clickDarkMode
 * Change handler for the Dark Mode checkbox.
 */
async function clickDarkMode() {
  updateTheme();
  saveSetting("darkmode", darkMode.checked);
}

async function clickGetActCode() {
  flashUsage.toactive = 1;
  url = "https://license.nmminer.shop?dev=" + devcode.value;
  window.open(url, "_blank");
}

async function clickConsoleMode() {
  // saveSetting("consoleonly", consoleonly.checked);
  if (butConnect.textContent != "Disconnect") {
    if (consoleonly.checked) {
      butConnect.textContent = "Connect";
    }
    else {
      butConnect.textContent = "Connect and Program";
    }
  }
}

/**
 * @name clickActivation
 * Click handler for the activation button.
 */
async function clickActivation() {
  logMsg("activation code: " + actcode.value);
  flashUsage.license = actcode.value;
  // await espStub.writeToStream(actcode.value);
  await espStub.writeToString(actcode.value);
}

async function clickClearInfo() {

  wifi_ssid.value = '';
  wifi_pwd.value = '';
  btc_addr.value = '';
  fallback_btc_addr.value = '';
  primary_pool_url.value = '';
  fallback_pool_url.value = '';

  saveSetting("wifi_ssid", wifi_ssid.value);
  saveSetting("wifi_password", wifi_pwd.value);
  saveSetting("address", btc_addr.value);  
  saveSetting("address2", fallback_btc_addr.value);
  saveSetting("primary_pool_url", primary_pool_url.value);
  saveSetting("fallback_pool_url", fallback_pool_url.value);
}

async function clickConfig() {
  const config_data = {
    ssid: wifi_ssid.value,
    password: wifi_pwd.value,
    address: btc_addr.value,
    address2: fallback_btc_addr.value,
    primary_pool_url: primary_pool_url.value,
    primary_pool_password: primary_pool_password.value,
    fallback_pool_url: fallback_pool_url.value,
    fallback_pool_password: fallback_pool_password.value
  };

  saveSetting("wifi_ssid", wifi_ssid.value);
  saveSetting("wifi_password", wifi_pwd.value);
  saveSetting("primary_pool_url", primary_pool_url.value);
  saveSetting("fallback_pool_url", fallback_pool_url.value);

  if(btc_addr.value)
  {
    saveSetting("address", btc_addr.value);
    if(fallback_btc_addr.value)
    {
      saveSetting("address2", fallback_btc_addr.value);
    }
    else
    {
      fallback_btc_addr.value = btc_addr.value;
      saveSetting("address2", fallback_btc_addr.value);
    }
    const jsonString = JSON.stringify(config_data);
    logMsg("Config WiFi: " + jsonString);
    await espStub.writeToString(jsonString);
  }
  else
  {
    errorMsg("Not Config. Enter btc address, it's importance for Lottery device working!!!")
  }
}

/**
 * @name clickErase
 * Click handler for the erase button.
 */
async function clickErase() {
  if (
    window.confirm("This will erase the entire flash. Click OK to continue.")
  ) {
    baudRate.disabled = true;
    // butErase.disabled = true;
    // butProgram.disabled = true;
    try {
      logMsg("Erasing flash memory. Please wait...");
      let stamp = Date.now();
      await espStub.eraseFlash();
      logMsg("Finished. Took " + (Date.now() - stamp) + "ms to erase.");
    } catch (e) {
      errorMsg(e);
    } finally {
      // butErase.disabled = false;
      baudRate.disabled = false;
      // butProgram.disabled = getValidFiles().length == 0;
    }
  }
}

/**
 * @name clickProgram
 * Click handler for the program button.
 */
async function clickProgram() {
  const readUploadedFileAsArrayBuffer = (inputFile) => {
    const reader = new FileReader();

    return new Promise((resolve, reject) => {
      reader.onerror = () => {
        reader.abort();
        reject(new DOMException("Problem parsing input file."));
      };

      reader.onload = () => {
        resolve(reader.result);
      };
      reader.readAsArrayBuffer(inputFile);
    });
  };

  baudRate.disabled = true;
  // butErase.disabled = true;
  // butProgram.disabled = true;
  for (let i = 0; i < 4; i++) {
    firmware[i].disabled = true;
    offsets[i].disabled = true;
  }
  for (let file of getValidFiles()) {
    progress[file].classList.remove("hidden");
    let binfile = firmware[file].files[0];
    let contents = await readUploadedFileAsArrayBuffer(binfile);
    try {
      let offset = parseInt(offsets[file].value, 16);
      const progressBar = progress[file].querySelector("div");
      await espStub.flashData(
        contents,
        (bytesWritten, totalBytes) => {
          progressBar.style.width =
            Math.floor((bytesWritten / totalBytes) * 100) + "%";
        },
        offset
      );
      await sleep(100);
    } catch (e) {
      errorMsg(e);
    }
  }
  for (let i = 0; i < 4; i++) {
    firmware[i].disabled = false;
    offsets[i].disabled = false;
    progress[i].classList.add("hidden");
    progress[i].querySelector("div").style.width = "0";
  }
  // butErase.disabled = false;
  baudRate.disabled = false;
  // butProgram.disabled = getValidFiles().length == 0;
  logMsg("To run the new firmware, please reset your device.");
}

function getValidFiles() {
  // Get a list of file and offsets
  // This will be used to check if we have valid stuff
  // and will also return a list of files to program
  let validFiles = [];
  let offsetVals = [];
  for (let i = 0; i < 4; i++) {
    let offs = parseInt(offsets[i].value, 16);
    if (firmware[i].files.length > 0 && !offsetVals.includes(offs)) {
      validFiles.push(i);
      offsetVals.push(offs);
    }
  }
  return validFiles;
}

/**
 * @name checkProgrammable
 * Check if the conditions to program the device are sufficient
 */
async function checkProgrammable() {
  // butProgram.disabled = getValidFiles().length == 0;
}

/**
 * @name checkFirmware
 * Handler for firmware upload changes
 */
async function checkFirmware(event) {
  let filename = event.target.value.split("\\").pop();
  let label = event.target.parentNode.querySelector("span");
  let icon = event.target.parentNode.querySelector("svg");
  if (filename != "") {
    if (filename.length > 17) {
      label.innerHTML = filename.substring(0, 14) + "&hellip;";
    } else {
      label.innerHTML = filename;
    }
    icon.classList.add("hidden");
  } else {
    label.innerHTML = "Choose a file&hellip;";
    icon.classList.remove("hidden");
  }

  await checkProgrammable();
}

/**
 * @name clickClear
 * Click handler for the clear button.
 */
async function clickClear() {
  // reset();     Reset function wasnt declared.
  log.innerHTML = "";
}

function convertJSON(chunk) {
  try {
    let jsonObj = JSON.parse(chunk);
    return jsonObj;
  } catch (e) {
    return chunk;
  }
}

function toggleUIToolbar(show) {
  isConnected = show;
  for (let i = 0; i < 4; i++) {
    progress[i].classList.add("hidden");
    progress[i].querySelector("div").style.width = "0";
  }
  if (show) {
    appDiv.classList.add("connected");
  } else {
    appDiv.classList.remove("connected");
  }
  // butErase.disabled = !show;
}

// ESP32-D0 framework
const otherBootDevice = [
  'esp32-2432s028r-ili9341',
  'esp32-2432s028r-st7789',
  'heltec-wifi-lora32-v2',
  'ttgo-t-display',
  'esp32-dev-kitc-32',
  'esp32-2432s024',
  'esp32-3248s035',
  'esp32cam',
  'wt32-sc01',
  'nm-tv-154',
  'elecrow-esp32-display-24inch',
  'elecrow-esp32-display-28inch',
  'esp32-32e-lcd-display-40inch',
  'esp32-32e-lcd-display-32inch'
];

const esp32c5BootDevice = [
  'esp32-c5-devkitc-1',
  'nm-cyd-c5'
];
// Axe framework
const axeDevice = [
  'NMAxe-v1x',
  'NMAxe-Gamma',
  'NMQAxe++'
];

const newUIVersion = [
  'v2.0.01',
  'v2.0.02'
];

function toggleUIConnected(connected) {
  let lbl = "Connect";
  if (connected) {
    lbl = "Disconnect";
    if(axeDevice.includes(device.value))
    {
      offsets[0].value = "f90000";
      offsets[1].value = "8000";
      offsets[2].value = "410000";
      offsets[3].value = "10000";

      urls[0] = "/firmware/" + version.value + "/" + device.value + "/ota_data_initial.bin";
      urls[1] = "/firmware/" + version.value + "/" + device.value + "/partitions.bin";
      urls[2] = "/firmware/" + version.value + "/" + device.value + "/spiffs.bin";
      urls[3] = "/firmware/" + version.value + "/" + device.value + "/firmware.bin";
      // urls[0] = "/firmware/" + device.value + "/ota_data_initial.bin";
      // urls[1] = "/firmware/" + device.value + "/partitions.bin";
      // urls[2] = "/firmware/" + device.value + "/spiffs.bin";
      // urls[3] = "/firmware/" + device.value + "/firmware.bin";

      span.textContent = "spiffs.bin";
    }
    else
    {
      span.textContent = "boot_app0.bin";
      if(otherBootDevice.includes(device.value))
      {
        offsets[0].value = "1000";
      }
      else if(esp32c5BootDevice.includes(device.value))
      {
        offsets[0].value = '2000';
      }
      else{
        offsets[0].value = "0";
      }

    offsets[1].value = "8000";
    offsets[2].value = "e000";
    if(newUIVersion.includes(version.value))
    {
      offsets[3].value = "390000"; // littlefs
    }
    else
    {
      offsets[3].value = "10000";
    }

    // urls[0] = "https://raw.githubusercontent.com/NMminer1024/NMMiner/refs/heads/main/firmware/" + device.value + "/bootloader.bin";
    // urls[1] = "https://raw.githubusercontent.com/NMminer1024/NMMiner/refs/heads/main/firmware/" + device.value + "/partitions.bin";
    // urls[2] = "https://raw.githubusercontent.com/NMminer1024/NMMiner/refs/heads/main/firmware/" + device.value + "/boot_app0.bin";
    // urls[3] = "https://raw.githubusercontent.com/NMminer1024/NMMiner/refs/heads/main/firmware/" + device.value + "/firmware.bin";

    // urls[0] = "/firmware/" + device.value + "/bootloader.bin";
    // urls[1] = "/firmware/" + device.value + "/partitions.bin";
    // urls[2] = "/firmware/" + device.value + "/boot_app0.bin";
    // urls[3] = "/firmware/" + device.value + "/firmware.bin";

    urls[0] = "/firmware/" + version.value + "/" + device.value + "/bootloader.bin";
    urls[1] = "/firmware/" + version.value + "/" + device.value + "/partitions.bin";
    urls[2] = "/firmware/" + version.value + "/" + device.value + "/boot_app0.bin";
    if(newUIVersion.includes(version.value))
    {
      urls[4] = "/firmware/" + version.value + "/" + device.value + "/firmware.bin";
      urls[3] = "/firmware/" + version.value + "/" + device.value + "/littlefs.bin";
    }
    else
    {
      urls[3] = "/firmware/" + version.value + "/" + device.value + "/firmware.bin";
    }
          
    }

    // logMsg("firmware: " + urls[3]);

  } else {
    toggleUIToolbar(false);
  }
  butConnect.textContent = lbl;
}

function loadAllSettings() {
  // Load all saved settings or defaults
  autoscroll.checked = loadSetting("autoscroll", true);
  baudRate.value = loadSetting("baudrate", 921600);
  darkMode.checked = loadSetting("darkmode", false);
  // consoleonly.checked = loadSetting("consoleonly", false);

  wifi_ssid.value = loadSetting("wifi_ssid", "");
  wifi_pwd.value = loadSetting("wifi_password", "");
  btc_addr.value = loadSetting("address", "");
  fallback_btc_addr.value = loadSetting("address2", "");
  primary_pool_url.value = loadSetting("primary_pool_url", "stratum+tcp://solobtc.nmminer.com:3333");
  fallback_pool_url.value = loadSetting("fallback_pool_url", "stratum+tcp://au.solobtc.nmminer.com:3333");
}

function loadSetting(setting, defaultValue) {
  let value = JSON.parse(window.localStorage.getItem(setting));
  if (value == null) {
    return defaultValue;
  }

  return value;
}

function saveSetting(setting, value) {
  window.localStorage.setItem(setting, JSON.stringify(value));
}

function ucWords(text) {
  return text
    .replace("_", " ")
    .toLowerCase()
    .replace(/(?<= )[^\s]|^./g, (a) => a.toUpperCase());
}

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

async function getNewLicense(uid) {
  try {
    const url = `https://license.nmminer.shop:19999/get_new_license?uid=${uid}`;
    const response = await fetch(url, {method: 'GET', mode: 'cors', headers:{ 'Content-Type' : 'application/json'}});
    if (!response.ok) {
      return 'failed';
    }
    const data = await response.json();
    if (data.status !== 'success') {
      return 'failed';
    }
    return data.license;
  } catch (error) {
    console.error(error);
    return 'failed';
  }
}

async function getLicense(uid) {
  try {
    const url = `https://license.nmminer.shop:19999/get_uid_license?uid=${uid}`;
    const response = await fetch(url, {method: 'GET', mode: 'cors', headers:{ 'Content-Type' : 'application/json'}});
    if (!response.ok) {
      return 'failed';
    }
    const data = await response.json();
    if (data.status !== 'success') {
      return 'failed';
    }
    return data.license;
  } catch (error) {
    console.error(error);
    return 'failed';
  }
}

async function storeRecord(record){
  try{
    const url = 'https://pool.nmminer.com:9527/v1/flashrecord/';
    // logMsg(JSON.stringify(record))
    const response = await fetch(url, {
      method: 'POST', 
      mode: 'cors', 
      headers:{ 'Content-Type' : 'application/json'},
      body: JSON.stringify(record)
    });
    if(!response.ok)
    {
      console.error('Failed to sotre ${response.statusText}')
    }
    const responseData = await response.json();
    console.log('Store ok:', responseData);
  }
  catch(error)
  {
    console.error(error);
  }
}
