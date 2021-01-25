# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c"
					"Sd_card.c"
					"Server.c"
					"Ota.c"
					"Uc_Sim.c"
					"Ethernet.c"
					"socket.c"
					"w5500.c"
					"wizchip_conf.c"
					"Wifi_station.c"
					"ModbusMaster.c"
					"Sensor.c"
					"power.c"
					"Wifiap.c"
					"swing_socket_app.c"
					"Kbvison_cam.c")
set(COMPONENT_ADD_INCLUDEDIRS "")
register_component()
