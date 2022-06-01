function bytes2hex(bytes) {
    var hexs = bytes.map(byte => ("00" + byte.toString(16)).slice(-2));
    return hexs.join('');
}

function color2number(color){
	var num = 0;
	num |= parseInt(color.substr(1, 2), 16) << 16;
	num |= parseInt(color.substr(3, 2), 16) << 8;
	num |= parseInt(color.substr(5, 2), 16);
	return num;
}

module.exports = function(RED) {
    function IrPixelsNode(config) {
        RED.nodes.createNode(this, config);
        var node = this;
        node.on('input', async (msg, send, done) => {
		// console.log("IrPixelsNode", msg, config);
		var temp = [
			config.btn00, config.btn01, config.btn02, config.btn03, config.btn04,
			config.btn05, config.btn06, config.btn07, config.btn08, config.btn09,
			config.btn10, config.btn11, config.btn12, config.btn13, config.btn14,
			config.btn15, config.btn16, config.btn17, config.btn18, config.btn19,
			config.btn20, config.btn21, config.btn22, config.btn23, config.btn24 ];

		var value = [];
		for( var i = 0 ; i < temp.length ; i++ ){
			if( (i % 8) == 0 )
				value[Math.floor(i / 8)] = 0x00;
			if( temp[i] )
				value[Math.floor(i / 8)] |= 0x01 << (7 - (i % 8));
		}
                msg.payload = JSON.stringify({
			type: "pixels_draw",
			bitmap: bytes2hex(value),
			fore_color: color2number(config.forecolor),
			back_color: color2number(config.backcolor),
                });
                node.send(msg);
        });
    }
    RED.nodes.registerType("ir-pixels", IrPixelsNode);
}
