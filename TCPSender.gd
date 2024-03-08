extends Node

var port: int = 8080
var server_ip: String = "127.0.0.1"  # Defaulting to localhost
var connection: StreamPeerTCP

func _ready():
	connection = StreamPeerTCP.new()
	connect_to_robot()

func connect_to_robot():
	var error = connection.connect_to_host(server_ip, port)
	if error != OK:
		print("Failed to connect to robot command port on ", port)
	else:
		print("Connected to robot command port on ", port)

func send_command(command: String):
	# Check if the connection is active
	if connection.get_status() == StreamPeerTCP.STATUS_CONNECTED:
		var data = command.to_utf8_buffer()
		connection.put_data(data)
	else:
		print("Not connected. Reconnecting...")
		connect_to_robot()
