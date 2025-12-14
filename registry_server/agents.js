// agents.js

const agents = {};   // Lưu danh sách agent theo dạng key-value

function registerAgent(info) {
    agents[info.machineId] = {
        machineId: info.machineId,
        ip: info.ip,
        os: info.os,
        wsPort: info.wsPort,
        tags: info.tags || [],
        lastSeen: Date.now()
    };
}

function updateHeartbeat(machineId) {
    if (agents[machineId]) {
        agents[machineId].lastSeen = Date.now();
    }
}

function unregisterAgent(machineId) {
    delete agents[machineId];
}

function getAllAgents() {
    return Object.values(agents);
}

function getAgent(machineId) {
    return agents[machineId] || null;
}

module.exports = {
    registerAgent,
    updateHeartbeat,
    unregisterAgent,
    getAllAgents,
    getAgent
};
