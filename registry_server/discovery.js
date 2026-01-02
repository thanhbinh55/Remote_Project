// const dgram = require("dgram");
// const server = dgram.createSocket("udp4");

// server.on("listening", () => {
//     const address = server.address();
//     console.log(`[UDP] Discovery server listening on ${address.address}:${address.port}`);
// });

// server.on("message", (msg, rinfo) => {
//     const text = msg.toString();

//     console.log(`[UDP] Message from ${rinfo.address}:`, text);

//     if (text === "DISCOVER_REGISTRY") {
//         const response = `REGISTRY_IP:${rinfo.address}`;
//         server.send(response, rinfo.port, rinfo.address);
//         console.log("[UDP] Sent:", response);
//     }
// });

// // Lắng nghe UDP 8888
// server.bind(8888);
const dgram = require("dgram");
const os = require("os");

const server = dgram.createSocket("udp4");

// Hàm lấy IP thông minh: Ưu tiên Wifi/LAN, bỏ qua VMware/VirtualBox
function getRealIP() {
    const nets = os.networkInterfaces();

    const WIFI_NAMES = ["Wi-Fi", "WiFi", "Wireless LAN adapter Wi-Fi"];

    // Ưu tiên lấy IP từ Wi-Fi
    for (const name of Object.keys(nets)) {
        if (!WIFI_NAMES.includes(name)) continue;

        for (const net of nets[name]) {
            if (net.family === 'IPv4' && !net.internal) {
                return net.address;
            }
        }
    }

    // Nếu không có Wi-Fi thì fallback: bỏ VMware, VirtualBox
    for (const name of Object.keys(nets)) {
        for (const net of nets[name]) {
            if (net.family === 'IPv4' && !net.internal) {

                // BỎ các adapter ảo
                if (name.includes("VMware")) continue;
                if (name.includes("VirtualBox")) continue;
                if (name.includes("Hyper-V")) continue;

                return net.address;
            }
        }
    }

    return "127.0.0.1";
}

const REGISTRY_IP = getRealIP();

server.on("listening", () => {
    console.log(`[UDP] Discovery server listening on 0.0.0.0:8888`);
    console.log(`[UDP] Registry IP = ${REGISTRY_IP}`);
});

server.on("message", (msg, rinfo) => {
    console.log(`[UDP] Message from ${rinfo.address}:`, msg.toString());

    if (msg.toString() === "DISCOVER_REGISTRY")  {
        const reply = "REGISTRY_IP:" + REGISTRY_IP;
        server.send(reply, rinfo.port, rinfo.address);
        console.log(`[UDP] Sent: ${reply} → to ${rinfo.address}:${rinfo.port}`);
    }
});

server.bind(8888, "0.0.0.0");

