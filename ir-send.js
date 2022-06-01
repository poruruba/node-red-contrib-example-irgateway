module.exports = function(RED) {
    function IrSendNode(config) {
        RED.nodes.createNode(this, config);
        var node = this;
        node.on('input', async (msg, send, done) => {
                msg.payload = JSON.stringify({
			type: "ir_send",
			address: parseInt(config.address),
			command: parseInt(config.command)
                });
                node.send(msg);
        });
    }
    RED.nodes.registerType("ir-send", IrSendNode);
}
