const express = require("express");//framework để tạo web API
const cors = require("cors");//Cross-Origin Resource Sharing → cho phép Angular gọi API
const bodyParser = require("body-parser");//dùng để đọc JSON trong request
const agentRepo = require("./agents");

const app = express(); //Khởi tạo app server
// app lúc này là Web server tương đương:
// accept request
// route đến handler
// // trả response

// Kích hoạt middlewares
// Là một lớp chạy giữa:
// Client Request → Middleware → Express Route → Response
app.use(cors()); //cors() Cho phép Angular (localhost:4200) gọi server (localhost:3000)
app.use(bodyParser.json()); //Đọc JSON trong POST request.

// test route
// Khi browser gửi GET /
// Express gọi hàm callback (req, res)
// res.json() trả về JSON
app.get("/", (req, res) => {
    res.json({ message: "Registry Server is running!" });
});

// start server
const PORT = 3000;
app.listen(PORT, () => console.log("Registry server running on port", PORT));

// Tạo API register
// POST /api/agents/register
app.post("/api/agents/register", (req, res) => {
    const info = req.body;

    if (!info.machineId || !info.ip || !info.wsPort) {
        return res.status(400).json({ error: "Missing required fields" });
    }

    agentRepo.registerAgent(info);
    res.json({ status: "registered" });
});


// Tạo API heartbeat
// POST /api/agents/heartbeat
app.post("/api/agents/heartbeat", (req, res) => {
    const { machineId } = req.body;

    if (!machineId) {
        return res.status(400).json({ error: "machineId is required" });
    }

    agentRepo.updateHeartbeat(machineId);
    res.json({ status: "heartbeat ok" });
});


// Cuối cùng thêm API trả danh sách máy cho Angular
// Get all agents từ agents.js
// Tính online/offline dựa trên lastSeen
// GET /api/agents
app.get("/api/agents", (req, res) => {
    const list = agentRepo.getAllAgents();

    // kiểm tra online (lastSeen trong 60 giây)
    const now = Date.now();
    list.forEach(a => {
        a.online = (now - a.lastSeen < 60000);
    });

    res.json(list);
});

