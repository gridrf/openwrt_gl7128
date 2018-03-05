m = Map("lora-station", "LoRa-Station", translate("LoRa-station is a LoRa station service implementation, here you can configure the settings."))

s = m:section(TypedSection, "lora-station", "LoRa Parameters")
s.addremove = false
s.anonymous = true

s:tab("server", translate("Server Settings"))
s:tab("radio", translate("Radio Settings"))
s:tab("websocket", translate("Websocket Settings"))

gateway_id = s:taboption("server", Value,"gateway_id",translate("Gateway ID"), "mac in hex")
gateway_id.optional = false;
gateway_id.rmempty = false;
gateway_id.default = ''
gateway_id.datatype = "string"

server_address = s:taboption("server", Value,"server_address",translate("Server Address"), "ip address")
server_address.optional = false;
server_address.rmempty = false;
server_address.default = ''
server_address.datatype = "string"

serv_port_up = s:taboption("server", Value,"serv_port_up",translate("Server Port (Up)"), "uplink port")
serv_port_up.optional = false;
serv_port_up.rmempty = false;
serv_port_up.default = '1680'
serv_port_up.datatype = "uinteger"

serv_port_down = s:taboption("server", Value,"serv_port_down",translate("Server Port (Down)"), "downlink port")
serv_port_down.optional = false;
serv_port_down.rmempty = false;
serv_port_down.default = '1780'
serv_port_down.datatype = "uinteger"

is_public_network = s:taboption("server", Flag,"is_public_network",translate("Public Network"), "set radio to public network")
is_public_network.optional = false;
is_public_network.rmempty = false;
is_public_network.default = '0'

keepalive_interval = s:taboption("server", Value,"keepalive_interval",translate("Keep Alive Interval"), "in ms")
keepalive_interval.optional = false;
keepalive_interval.rmempty = false;
keepalive_interval.default = '30000'
keepalive_interval.datatype = "uinteger"


--
-- Radio Configuration Mode
--
mode_select= s:taboption("radio", ListValue,"mode",translate("Configuration Mode"))
mode_select.optional = false;
mode_select.rmempty = false;
mode_select.default = "LORA"
mode_select.datatype = "string"
mode_select:value("LORA", translate("LoRa"))
mode_select:value("FSK", translate("Fsk"))
--
-- channel frequency
--
frequency = s:taboption("radio", Value,"frequency",translate("Channel Frequency"), "in Hz")
frequency.optional = false;
frequency.rmempty = false;
frequency.default = '433175000'
frequency.datatype = "uinteger"

--
-- signal strength
--
power = s:taboption("radio", Value,"power",translate("Signal Strength"), "in dBm")
power.optional = false;
power.rmempty = false;
power.default = '10'
power.datatype = "uinteger"


--
-- signal PREAMBLE_LENGTH
--
preamble = s:taboption("radio", Value,"preamble",translate("Preamble Length"), "Same for Tx and Rx")
preamble.optional = false;
preamble.rmempty = false;
preamble.default = '8'
preamble.datatype = "uinteger"

--
-- fsk channel datarate
--
fsk_datarate = s:taboption("radio", Value,"datarate",translate("Channel Datarate"), "in bps")
fsk_datarate.optional = true;
fsk_datarate.rmempty = false;
fsk_datarate.default = '50000'
fsk_datarate.datatype = "uinteger"
fsk_datarate:depends("mode", "FSK")

--
-- fsk channel FDEV
--
fdev = s:taboption("radio", Value,"fdev",translate("Fdev"), "in Hz")
fdev.optional = true;
fdev.rmempty = false;
fdev.default = '25000'
fdev.datatype = "uinteger"
fdev:depends("mode", "FSK")

--
-- fsk channel BANDWIDTH
--
fsk_bandwidth = s:taboption("radio", Value,"fsk_bandwidth",translate("Channel Bandwidth"), "in Hz")
fsk_bandwidth.optional = true;
fsk_bandwidth.rmempty = false;
fsk_bandwidth.default = '50000'
fsk_bandwidth.datatype = "uinteger"
fsk_bandwidth:depends("mode", "FSK")

--
-- fsk channel AFC_BANDWIDTH
--
fsk_afc_bandwidth = s:taboption("radio", Value,"afc_bandwidth",translate("Channel AFC Bandwidth"), "in Hz")
fsk_afc_bandwidth.optional = true;
fsk_afc_bandwidth.rmempty = false;
fsk_afc_bandwidth.default = '83333'
fsk_afc_bandwidth.datatype = "uinteger"
fsk_afc_bandwidth:depends("mode", "FSK")

--
-- channel bandwidth
--
bandwidth = s:taboption("radio", Value,"bandwidth",translate("Channel Bandwidth"), "0:125khz,1:250khz,2:500khz")
bandwidth.optional = true;
bandwidth.rmempty = false;
bandwidth.default = '0'
bandwidth.datatype = "uinteger"
bandwidth:depends("mode", "LORA")

--
-- spreading factor
--
spreading_factor = s:taboption("radio", Value,"spreading_factor",translate("Spreading Factor"), "SF7-SF12")
spreading_factor.optional = true;
spreading_factor.rmempty = false;
spreading_factor.default = '7'
spreading_factor.datatype = "uinteger"
spreading_factor:depends("mode", "LORA")


--
-- CODINGRATE
--
codingrate = s:taboption("radio", Value,"codingrate",translate("Coding Rate"), "1:4/5,2:4/6,3:4/7,4:4/8")
codingrate.optional = true;
codingrate.rmempty = false;
codingrate.default = '1'
codingrate.datatype = "uinteger"
codingrate:depends("mode", "LORA")

tx_iqInverted = s:taboption("radio", Flag, "tx_iqInverted", translate("Tx Inversion"), "ipol")
tx_iqInverted.rmempty = false
tx_iqInverted.default = '1'

s:taboption("websocket", Flag, "websocket_open", translate("Listen Websocket")).rmempty = false


websocket_port = s:taboption("websocket", Value,"websocket_port",translate("Websocket port"), "websocket port")
websocket_port.optional = false;
websocket_port.rmempty = false;
websocket_port.default = '7681'
websocket_port.datatype = "uinteger"


tmpl = s:taboption("websocket", Value, "_tmpl",
	translate("Edit the template that is used for generating the index.html."), 
	translate("This is the content of the file '/usr/share/websocket/index.html' from which your websocket configuration will be generated. "))

tmpl.template = "cbi/tvalue"
tmpl.rows = 20

function tmpl.cfgvalue(self, section)
	return nixio.fs.readfile("/usr/share/websocket/index.html")
end

function tmpl.write(self, section, value)
	value = value:gsub("\r\n?", "\n")
	nixio.fs.writefile("//usr/share/websocket/index.html", value)
end



m.on_after_commit = function(self)

	io.popen("/etc/init.d/lora-station restart")
end

return m
