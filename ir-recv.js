module.exports = function(RED) {
    function IrRecvNode(config) {
        RED.nodes.createNode(this, config);
        var node = this;
        node.on('input', async (msg, send, done) => {
		var json = JSON.parse(msg.payload);
		msg.payload = json;
		msg.topic = json.type;
                node.send(msg);
        });
    }
    RED.nodes.registerType("ir-recv", IrRecvNode);
}
