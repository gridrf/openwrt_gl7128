module("luci.controller.lora-station", package.seeall)

function index()
	-- no config create an empty one
	if not nixio.fs.access("/etc/config/lora-station") then
		return
	end
        entry({"admin", "services", "lora-station"}, cbi("lora-station"), _("LoRa Station"), 105)
end
